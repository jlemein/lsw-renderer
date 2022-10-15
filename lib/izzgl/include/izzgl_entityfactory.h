#pragma once

#include <izzgl_rendersystem.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include "geo_effect.h"
#include "izz_renderablecomponentfactory.h"
#include "izzgl_meshsystem.h"

namespace izz {
namespace geo {
struct Mesh;
struct Curve;
class Scene;
struct SceneNode;
struct Light;
struct Camera;
};  // namespace geo

namespace ecs {
struct Transform;
struct PointLight;
}  // namespace ecs
}  // namespace izz

namespace izz {
namespace gl {
class MaterialSystem;
}
class SceneGraphEntity;

/**
 * @brief Specify what scene information to use for scene instantiation.
 * @details It is possible to skip loading
 */
struct SceneLoaderFlags {
  bool animations{true};
  bool cameras{false};
  bool geometry{true};
  bool lights{false};
  bool materials{true};

  static inline SceneLoaderFlags All() {
    return SceneLoaderFlags{true, true, true, true, true};
  }
};

/**!
 * Entity factory creates entities.
 *
 */
class EntityFactory {
  static inline const char* ID = "SceneGraphHelper";

 public:
  EntityFactory(entt::registry& registry, std::shared_ptr<gl::RenderSystem> renderSystem, std::shared_ptr<izz::gl::MaterialSystem> materialSystem,
                std::shared_ptr<izz::gl::MeshSystem> meshSystem);

  void setDefaultMaterial(std::shared_ptr<izz::gl::Material> material);

  // TODO: represent the active camera in the scene graph,
  //  probably by flagging the entity with ActiveCamera component.
  //  or maybe a centralized registry.
  void setActiveCamera(EntityFactory camera);

  entt::registry& getRegistry();

  /**!
   * @brief Creates a geometry
   * @return
   */
  // TODO: currently assigning a material to a geometry disconnects the relationship with the creator.
  //  this does not make it possible to share materials
  SceneGraphEntity addGeometry(izz::geo::Mesh mesh, int materialId);
  //  SceneGraphEntity addGeometry(lsw::geo::Mesh&& mesh, izz::gl::Material&& material);
//  SceneGraphEntity addGeometry(const izz::geo::Mesh& mesh);
//  SceneGraphEntity addGeometry(izz::geo::Mesh mesh, geo::cEffect effect);

  void setActiveCamera(const SceneGraphEntity* activeCamera);

  SceneGraphEntity makeEntity(std::string name = "");

  /// @brief Creates a simple barebone entity containing minimum components
  /// Minum components are: Transform, Name, Relationship
  SceneGraphEntity makeMoveableEntity(std::string name = "", glm::vec3 position = glm::vec3(0.0F));

  /// @brief Creates a camera at a Z distance of 5 meter from the origin,
  /// looking at the origin
  SceneGraphEntity makeCamera(std::string name, float zDistance = 5.0F, float fovx = 120.0F, float aspect = 1.0F, float zNear = 0.5F, float zFar = 1000.0F);

  SceneGraphEntity makeCamera(const izz::geo::Camera& geoCamera);

  SceneGraphEntity makeLight(const izz::geo::Light& light);

  SceneGraphEntity makeAmbientLight(std::string name, glm::vec3 color = {.1F, .1F, .1F}, float intensity = 1.0F);
  SceneGraphEntity makePointLight(std::string name, glm::vec3 position, izz::ecs::PointLight pointLight);
  SceneGraphEntity makePointLight(std::string name, glm::vec3 position);

  /**
   * Creates a spotlight specified using a lookat configuration, i.e. by specifying an eye, center and up vector.
   * @warning  Make sure that a orthogonal basis can be formed with the three vectors.
   * @param name    The readable name for this scene graph entity.
   * @param eye     The position of the spotlight.
   * @param center  Location the spotlight is targeted at. Note that the spotlight orientation is created once.
   *                Transformations applied to the spotlight will not reorient the spotlight to the center.
   * @param up      Up vector, pointing upwards, usually taken to be (0, 1, 0).
   * @return a scene graph entity containing, among others, the SpotLight and Transform component.
   */
  SceneGraphEntity makeSpotLightFromLookAt(std::string name, glm::vec3 eye = glm::vec3(.0F, .0F, 5.F), glm::vec3 center = glm::vec3(.0F, .0F, .0F),
                                           glm::vec3 up = glm::vec3(.0F, 1.F, .0F));

  /**
   * Creates a directional light source towards the specified direction
   * @param name        [in] A human readable name for the light source.
   * @param direction   [in] The direction of the light source, as seen from the
   *                         origin of the scene.
   * @return A new scene graph entity with the directional light source.
   */
  SceneGraphEntity makeDirectionalLight(std::string name, glm::vec3 direction = {0.10976F, 0.98787834F, 0.10976F});

  SceneGraphEntity makeMesh(const izz::geo::Mesh& mesh);
  SceneGraphEntity makeEmptyMesh(const izz::geo::Mesh& mesh);
  SceneGraphEntity makeCurve(std::string name);

  SceneGraphEntity makeRenderable(izz::gl::MeshBuffer&& mesh, MaterialId materialId, gl::RenderStrategy type = gl::RenderStrategy::UNDEFINED);
  SceneGraphEntity makeRenderable(std::string name, const izz::gl::MeshBuffer& mesh, glm::mat4 transform, MaterialId materialId, gl::RenderStrategy type = gl::RenderStrategy::UNDEFINED);
  SceneGraphEntity makeRenderable(izz::geo::Curve&& curve, MaterialId materialId, gl::RenderStrategy type = gl::RenderStrategy::UNDEFINED);

  /**!
   * Loads a complete scene and adds it to the scene graph.
   * A scene consists of meshes, a node hierarchy with instances referring to
   * the meshes.
   * Also it consists of cameras, and lights, which can optionally be loaded
   * as well. By default everything is loaded.
   * @return
   */
  SceneGraphEntity makeScene(const izz::geo::Scene&, SceneLoaderFlags flags = SceneLoaderFlags{}, gl::RenderStrategy renderableType = gl::RenderStrategy::UNDEFINED);

  SceneGraphEntity makeTexture();
  SceneGraphEntity makeRectangularGrid(float size = 10.0F, float spacing = 1.0F);
  SceneGraphEntity makeDebugVisualization(entt::entity target);

  SceneGraphEntity makePosteffect(const std::string name, int materialId);

  void setDefaultRenderStrategy(gl::RenderStrategy renderStrategy);

 private:
  /// Uses EnTT in the background for scene management
  entt::registry& m_registry;
  std::shared_ptr<gl::RenderSystem> m_renderSystem {nullptr};
  std::shared_ptr<izz::gl::MaterialSystem> m_materialSystem{nullptr};
  std::shared_ptr<izz::gl::MeshSystem> m_meshSystem{nullptr};

  /// Default render strategy to use if not specified.
  gl::RenderStrategy m_defaultRenderStrategy = gl::RenderStrategy::UNDEFINED;

  std::shared_ptr<izz::gl::Material> m_defaultMaterial{nullptr};
  const SceneGraphEntity* m_activeCamera{nullptr};

  void processChildren(const izz::geo::Scene& scene, std::shared_ptr<const izz::geo::SceneNode> node, SceneLoaderFlags flags,
                       SceneGraphEntity* parent_p = nullptr, gl::RenderStrategy type = gl::RenderStrategy::UNDEFINED);
};

//====================================
// INLINE DEFINITIONS
//====================================

inline entt::registry& EntityFactory::getRegistry() {
  return m_registry;
}

}  // namespace izz
