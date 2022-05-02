//
// Created by jlemein on 22-02-21.
//
#pragma once

#include <GL/glew.h>
#include <fmt/format.h>
#include <gl_renderable.h>
#include <izz.h>
#include <izzgl_texture.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <variant>
#include "geo_materialdescription.h"

namespace izz {
namespace gl {

enum class UType : uint32_t { FLOAT, FLOAT2, FLOAT3, FLOAT4, FLOAT_ARRAY, BOOL, INT, INT2, INT3, INT4, INT_ARRAY, MAT2, MAT3, MAT4 };

class UniformProperty {
 public:
  UniformProperty(GLint location = 0, unsigned char* data = nullptr, int length = 1)
    : m_location{location}
    , m_data{data}
    , m_length{length} {}

  GLint m_location;  /// @brief the location of the uniform parameter in the GLSL shader.
                     //  UType type;      /// @brief Type of the uniform property stored. @see UType.
  void* m_data;      /// @brief Offset in the UnscopedUniforms::pData attribute where the value is stored. Offset is expressed in bytes (uint8_t).
  int m_length;      /// @brief length of the property. This is expressed in number of elements (i.e. the length of the array irrespective of data type)
};

class UniformProperties {
 private:
  UniformProperty* m_properties;
  std::vector<uint8_t> m_data {};
  uint32_t m_usedBytes = 0U;
  uint32_t m_numProperties = 0;

 public:
  UniformProperties() {}

  UniformProperties(std::size_t size, int numProperties) {
    m_properties = new UniformProperty[numProperties];
    m_numProperties = 0;
    m_data = std::vector<uint8_t>();
    m_data.resize(size);
  }

  ~UniformProperties() {
    delete[] m_properties;
  }

  const unsigned char* data() const {
    return m_data.data();
  }

  UniformProperty* addBoolean(std::string name, bool value, int location) {
    GLint* data = reinterpret_cast<GLint*>(m_data.data() + m_usedBytes);
    *data = static_cast<GLint>(value);

    if (location != GL_INVALID_INDEX) {
      glUniform1i(location, *reinterpret_cast<GLint*>(data));
    }

    UniformProperty* p = &m_properties[m_numProperties++];
    p->m_data = data;
    p->m_location = location;
    p->m_length = 1;

    booleanValues[name] = p;
    m_usedBytes += sizeof(GLint);

    return p;
  }

  UniformProperty* addInt(std::string name, int value, int location) {
    GLint* data = reinterpret_cast<GLint*>(m_data.data() + m_usedBytes);
    *data = static_cast<GLint>(value);

    if (location != GL_INVALID_INDEX) {
      glUniform1i(location, *reinterpret_cast<GLint*>(data));
    }

    UniformProperty* p = &m_properties[m_numProperties++];
    p->m_data = data;
    p->m_location = location;
    p->m_length = 1;

    intValues[name] = p;
    m_usedBytes += sizeof(GLint);

    return p;
  }

  UniformProperty* addFloat(std::string name, float value, int location) {
    GLfloat* data = reinterpret_cast<GLfloat*>(m_data.data() + m_usedBytes);
    *data = static_cast<GLfloat>(value);

    if (location != GL_INVALID_INDEX) {
      glUniform1f(location, *data);
    }

    UniformProperty* p = &m_properties[m_numProperties++];
    p->m_data = data;
    p->m_location = location;
    p->m_length = 1;

    floatValues[name] = p;
    m_usedBytes += sizeof(GLfloat);

    return p;
  }

  UniformProperty* addFloatArray(std::string name, std::vector<float> values, int location) {
    GLfloat* data = reinterpret_cast<GLfloat*>(m_data.data() + m_usedBytes);
    memcpy(reinterpret_cast<GLfloat*>(data), values.data(), sizeof(float) * values.size());

    if (location != GL_INVALID_INDEX) {
      glUniform1fv(location, values.size(), reinterpret_cast<GLfloat*>(data));
    }

    UniformProperty* p = &m_properties[m_numProperties++];
    p->m_data = data;
    p->m_location = location;
    p->m_length = static_cast<int>(values.size());

    floatArrayValues[name] = p;
    m_usedBytes += sizeof(GLfloat) * values.size();

    return p;
  }

  /// @brief maps the name of a uniform property to a float property in the m_properties vector.
  std::unordered_map<std::string, UniformProperty*> floatValues;
  /// @brief maps the name of a uniform property to a integer property in the m_properties vector.
  std::unordered_map<std::string, UniformProperty*> intValues;
  /// @brief maps the name of a uniform property to a float array property in the m_properties vector.
  std::unordered_map<std::string, UniformProperty*> floatArrayValues;
  /// @brief maps the name of a uniform property to a boolean property in the m_properties vector.
  std::unordered_map<std::string, UniformProperty*> booleanValues;
};

/**
 * @brief Describes a uniform buffer object in OpenGL. Uniform buffer objects are used for named uniform blocks.
 */
struct UniformBuffer {
  GLuint bufferId{0};  // generated by glGenBuffers(1, &id).
  int blockIndex{-1};
  int blockBind{0};
  size_t size{0U};
  void* data{nullptr};
};

/**
 * A mapping of data location and size to keep the uniform blocks registered.
 */
struct UniformBlockInfo {
  std::string name;
  void* data;
  std::size_t size;
};

struct LightingInfo {
  // name of UBO struct in shader
  std::string ubo_struct_name;
};

enum class ColorBuffer { UNUSED, CUBEMAP, BUFFER_1D, BUFFER_2D, TEXTURE_2D, TEXTURE_2D_MULTISAMPLE };

enum class FramebufferFormat { UNUSED, RGBA_FLOAT32, RGBA_UINT8 };
class FramebufferConfiguration {
 public:
  FramebufferFormat colorBuffers[4] = {FramebufferFormat::UNUSED, FramebufferFormat::UNUSED, FramebufferFormat::UNUSED, FramebufferFormat::UNUSED};
  FramebufferFormat depthBuffer;
  FramebufferFormat outColorBuffers[4] = {FramebufferFormat::UNUSED, FramebufferFormat::UNUSED, FramebufferFormat::UNUSED, FramebufferFormat::UNUSED};
};

struct TextureBuffer {
  GLuint textureId{0};  // as obtained via glGenTextures(, &id)
  GLint location{-1};   // as obtained via glGetUniformLocation
};

class Material {
 public:
  static inline const char* ID = "Material";

  // ==== PART OF MATERIAL DEFINITION ======================
  MaterialId id{-1};
  std::string name;

  //  MaterialProperty& operator[](const std::string& name) {
  //    return textures[name];
  //  }

  void addUniformBool(std::string paramName, bool value);

  void addUniformInt(std::string paramName, int value);

  void addUniformFloat(std::string paramName, float value);

  void addUniformFloatArray(std::string paramName, const std::vector<float>& value);

  void useTextures() const;

  void pushUnscopedUniforms() const;

  void pushUniforms() const;

  void setTexture(const std::string& name, Texture* pTexture) {
    if (!textures.contains(name)) {
      auto location = glGetUniformLocation(programId, name.c_str());
      if (location == GL_INVALID_INDEX) {
        spdlog::warn("Material: texture sampler '{}' could not be found in material {}: {}", name, programId, name);
      }
      textures[name] = TextureBuffer{pTexture != nullptr ? pTexture->bufferId : 0, location};
    } else {
      textures.at(name).textureId = pTexture->bufferId;
    }
  }

  GLint getTextureBuffer(const std::string& key) const {
    if (textures.count(key) > 0) {
      return textures.at(key).textureId;
    } else {
      return -1;
      //      throw std::runtime_error(fmt::format("Property {} (texture) does not exist for material {}", key, name));
    }
  }

  LightingInfo lighting;

  std::string shaderLayout{""};

  // special set of attributes
  glm::vec3 diffuse;
  glm::vec3 emissive;
  glm::vec3 ambient;
  glm::vec3 specular;
  glm::vec3 transparent;

  /// @brief Globally declared uniforms. It's not adviceable to use many global uniforms. Use uniform buffers instead.
  std::shared_ptr<UniformProperties> globalUniforms{nullptr};
  std::shared_ptr<UniformProperties> scopedUniforms{nullptr};

  /// @brief collection of uniform name to uniform property. This map contains both scoped as well as unscoped uniforms.
  std::unordered_map<std::string, UniformProperty*> m_allUniforms;  // Refers to all the uniform properties (unscoped and scoped).

  // contains map from name to property type
  std::unordered_map<std::string, izz::geo::PropertyType> propertyTypes;

  std::filesystem::path diffuseTexturePath{""};
  std::filesystem::path specularTexturePath{""};
  std::filesystem::path normalTexturePath{""};
  std::filesystem::path roughnessTexturePath{""};

  // ==== PROPERTIES PART OF DEFINITION =======

  // ==== PROPERTIES BELOW ARE PART OF GPU ================================
  int programId{0};  /// @brief Program id as obtained via glCreateProgram()
  std::unordered_map<std::string, izz::gl::TextureBuffer> textures{};
  std::unordered_map<std::string, UniformBuffer> uniformBuffers{};

  template <typename T>
  void setProperty(const T& data) {
    setProperty(T::PARAM_NAME, data);
  }

  void setUniformFloat(std::string name, float value) {
    // dangerous cast here
    if (m_allUniforms.count(name) > 0) {
      auto pValue = reinterpret_cast<GLfloat*>(m_allUniforms.at(name)->m_data);
      *pValue = value;
    }
  }

  void setUniformVec4(std::string name, const glm::vec4& value) {
    // dangerous cast here
    if (m_allUniforms.count(name) > 0) {
      memcpy(m_allUniforms.at(name)->m_data, glm::value_ptr(value), sizeof(GLfloat) * 4);
    }
  }

  void setUniformInt(std::string name, int value) {
    if (m_allUniforms.count(name) > 0) {
      // dangerous cast here
      auto pValue = reinterpret_cast<GLint*>(m_allUniforms.at(name)->m_data);
      *pValue = value;
    }
  }

  void setUniformBool(std::string name, bool value) {
    if (m_allUniforms.count(name) > 0) {
      // dangerous cast here
      auto pValue = reinterpret_cast<GLint*>(m_allUniforms.at(name)->m_data);
      *pValue = static_cast<GLint>(value);
    }
  }

  void setUniformFloatArray(std::string name, const std::vector<float>& value) {
    if (m_allUniforms.count(name) > 0) {
      // dangerous cast here
      memcpy(m_allUniforms.at(name)->m_data, value.data(), sizeof(GLfloat) * value.size());
    }
  }

  glm::vec4 getUniformVec4(std::string name) const {
    auto pArr = reinterpret_cast<float*>(m_allUniforms.at(name)->m_data);
    return glm::vec4(pArr[0], pArr[1], pArr[2], pArr[3]);
  }

  float& getUniformFloat(std::string name) const {
    return *reinterpret_cast<float*>(m_allUniforms.at(name)->m_data);
  }

  std::vector<float> getUniformFloatArray(std::string name) const {
    auto prop = m_allUniforms.at(name);
    float* pStart = reinterpret_cast<float*>(prop->m_data);
    return std::vector<float>{pStart, pStart + prop->m_length};
  }

  // TODO: deprecated, remove it in favor of the register call below.
  template <typename T>
  [[deprecated("Use a uniform block manager instead")]] void setProperty(const char* name, const T& data) {
    if (uniformBuffers.count(name) > 0) {
      memcpy(uniformBuffers.at(name).data, &data, sizeof(T));

    } else {
      //      properties[name].data = std::unique_ptr<T>{new T,
      //      std::default_delete<T>()};
      uniformBuffers[name].data = new T();
      memcpy(uniformBuffers[name].data, &data, sizeof(T));
    }
    uniformBuffers.at(name).size = sizeof(T);
  }

  /**
   * @brief Assigns a uniform buffer object to a material. A material will imediately render using the new assigned uniform buffer object.
   * @param name The name of the UBO struct in the shader.
   * @param buffer The uniform buffer object. Uniform buffers can be created via @see MaterialSystem.
   */
  void setUniformBuffer(const char* name, UniformBuffer buffer) {
    if (uniformBuffers.count(name) > 0) {
      throw std::runtime_error("Cannot add a uniform block {} to material {} that already exists.");
    } else {
      spdlog::info("Material {}: registered UBO {}", id, name);
      uniformBuffers[name] = buffer;
    }
  }
  //
  //  void* getProperty(const char* name) {
  //    try {
  //      return uniformBlocks.at(name).data;
  //    } catch (std::out_of_range&) {
  //      throw std::runtime_error(fmt::format("Cannot find shader property {} in shader.", name));
  //    }
  //  }

  template <typename T>
  T* getProperty() {
    return reinterpret_cast<T*>(getProperty(T::PARAM_NAME));
  }
};

}  // namespace gl
}  // namespace izz
