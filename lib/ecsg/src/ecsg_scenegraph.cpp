#include <ecsg_scenegraph.h>
#include <ecsg_scenegraphentity.h>

#include <ecs_camera.h>
#include <ecs_light.h>
#include <ecs_name.h>
#include <ecs_relationship.h>
#include <ecs_texture.h>
#include <ecs_transform.h>

#include <glrs_renderable.h>

#include <geo_camera.h>
#include <geo_curve.h>
#include <geo_light.h>
#include <geo_material.h>
#include <geo_mesh.h>
#include <geo_meshinstance.h>
#include <geo_meshtransform.h>
#include <geo_scene.h>

#include <spdlog/spdlog.h>

using namespace lsw;
using namespace lsw::ecsg;
using namespace lsw::ecs;

SceneGraph::SceneGraph() {}

void SceneGraph::setDefaultMaterial(std::shared_ptr<geo::Material> material) {
  m_defaultMaterial = material;
}

SceneGraphEntity SceneGraph::addGeometry(geo::Mesh mesh, geo::Material mat) {
  auto e = makeEntity(mesh.name);
  e.add<glrs::Renderable>();
  e.add<geo::Mesh>(mesh);
  e.add<geo::Material>(mat);

  return e;
}

SceneGraphEntity SceneGraph::addGeometry(geo::Mesh mesh, std::shared_ptr<geo::Material> mat) {
  auto e = makeEntity(mesh.name);
  e.add<glrs::Renderable>();
  e.add<geo::Mesh>(mesh);
  // TODO: make sure we store a shared ptr instead of a copy.
  //  shared materials offer option to share materials.
  e.add<geo::Material>(*mat);

  return e;
}

// SceneGraphEntity SceneGraph::addGeometry(geo::Mesh&& mesh, geo::Material&& material) {
//   auto e = makeEntity(mesh.name);
//   e.add<glrs::Renderable>();
//   e.add<geo::Mesh>(std::forward<geo::Mesh>(mesh));
//   e.add<geo::Material>(std::forward<geo::Material>(material));
//
//   return e;
// }

SceneGraphEntity SceneGraph::makeEntity(std::string name) {
  auto e{m_registry.create()};
  m_registry.emplace<Name>(e, name);
  m_registry.emplace<Transform>(e);
  m_registry.emplace<ecs::Relationship>(e);
  return SceneGraphEntity{m_registry, e};
}

SceneGraphEntity SceneGraph::makeCamera(std::string name, float zDistance, float fovx, float aspect, float zNear, float zFar) {
  auto cameraEntity = makeEntity(std::move(name));
  ecs::Camera camera{.fovx = fovx, .aspect = aspect, .zNear = zNear, .zFar = zFar};
  cameraEntity.add<ecs::Camera>(std::move(camera));
  cameraEntity.get<ecs::Transform>().localTransform[3] = glm::vec4(0.0F, 0.0F, zDistance, 1.0F);

  return cameraEntity;
}

SceneGraphEntity SceneGraph::makeCamera(const geo::Camera& geoCamera) {
  auto cameraEntity = makeEntity(geoCamera.name);
  ecs::Camera camera{.fovx = geoCamera.fovx, .aspect = geoCamera.aspect, .zNear = geoCamera.near, .zFar = geoCamera.far};
  cameraEntity.add<ecs::Camera>(std::move(camera));

  auto& transform = cameraEntity.get<Transform>();
  transform.localTransform = glm::lookAt(geoCamera.position, geoCamera.lookAt, geoCamera.up);

  return cameraEntity;
}

SceneGraphEntity SceneGraph::makeLight(const geo::Light& light) {
  auto lightEntity = makeEntity(light.name);

  auto& transform = lightEntity.get<Transform>();
  transform.localTransform[3] = glm::vec4(light.position, 1.0F);

  if (light.type == geo::Light::Type::DIRECTIONAL_LIGHT) {
    transform.localTransform[3].w = 0.0F;
    lightEntity.add<ecs::DirectionalLight>({light.intensity, light.diffuseColor});
  } else if (light.type == geo::Light::Type::AMBIENT_LIGHT) {
    lightEntity.add<ecs::AmbientLight>({light.intensity, light.diffuseColor});
  } else if (light.type == geo::Light::Type::POINT_LIGHT) {
    lightEntity.add<ecs::PointLight>({.intensity = light.intensity,
                                      .linearAttenuation = light.attenuationLinear,
                                      .quadraticAttenuation = light.attenuationQuadratic,
                                      .radius = light.radius.x,
                                      .color = light.diffuseColor});
  } else {
    throw std::runtime_error("Cannot make light source for unknown geo::Light type.");
  }

  return lightEntity;
}

SceneGraphEntity SceneGraph::makeAmbientLight(std::string name, glm::vec3 color, float intensity) {
  auto lightEntity = makeEntity(std::move(name));
  lightEntity.add<ecs::AmbientLight>({.intensity = 1.0F, .color = color});
  return lightEntity;
}

SceneGraphEntity SceneGraph::makePointLight(std::string name, glm::vec3 position) {
  auto lightEntity = makeEntity(std::move(name));
  lightEntity.add<ecs::PointLight>();

  // different between point and directional light is in the w component.
  lightEntity.get<Transform>().localTransform[3] = glm::vec4(position, 1.0F);
  return lightEntity;
}

SceneGraphEntity SceneGraph::makePointLight(std::string name, glm::vec3 position, ecs::PointLight pointLight) {
  auto lightEntity = makeEntity(std::move(name));
  lightEntity.add<ecs::PointLight>(std::move(pointLight));

  // different between point and directional light is in the w component.
  lightEntity.get<Transform>().localTransform[3] = glm::vec4(position, 1.0F);
  return lightEntity;
}

SceneGraphEntity SceneGraph::makeDirectionalLight(std::string name, glm::vec3 direction) {
  auto lightEntity = makeEntity(std::move(name));
  lightEntity.add<ecs::DirectionalLight>();

  // different between point and directional light is in the w component.
  lightEntity.get<Transform>().localTransform[3] = glm::normalize(glm::vec4(direction, 1.0F));
  return lightEntity;
}

SceneGraphEntity SceneGraph::makeMesh(const geo::Mesh& mesh) {
  auto meshEntity = makeEntity(mesh.name);

  meshEntity.add<glrs::Renderable>();
  meshEntity.add<geo::Mesh>(mesh);

  // Watch out here, geo::Material is a value type so we can do this.
  // It is very tricky, because we ignore the resource itself here, possibly
  // causing dangling pointers in the end
  if (m_defaultMaterial == nullptr) {
    throw std::runtime_error("No default material set, cannot create mesh");
  }
  meshEntity.add<geo::Material>(*m_defaultMaterial);

  //  auto& shader = meshEntity.add<ecs::Shader>(
  //      {"assets/shaders/diffuse.vert.spv",
  //      "assets/shaders/diffuse.frag.spv"});
  //  shader.textures.emplace_back(Texture{
  //    .diffuseTextureFilepath = "assets/textures/textures/castle_normal.jpg"
  //  };

  return meshEntity;
}

SceneGraphEntity SceneGraph::makeEmptyMesh(const geo::Mesh& mesh) {
  auto sge = makeEntity(mesh.name);
  auto e = sge.handle();

  auto& renderable = m_registry.emplace<glrs::Renderable>(e);

  m_registry.emplace<geo::Mesh>(e, mesh);
  return sge;
}

// SceneGraphEntity SceneGraph::makeMeshInstance(const geo::Mesh &mesh) {
//  auto sge = makeEntity(mesh.name);
//  auto e = sge.id();
//
//  auto &renderable = m_registry.emplace<ecs::Renderable>(e);
//  renderable.name = mesh.name;
//
//  m_registry.emplace<geo::Mesh>(e, mesh);
//  m_registry.emplace<ecs::Shader>(
//      e, ecs::Shader{"assets/shaders/diffuse.vert.spv",
//                     "assets/shaders/diffuse.frag.spv"});
//
//  Texture &texture = m_registry.emplace<Texture>(e);
//  texture.diffuseTextureFilepath =
//  "assets/textures/textures/castle_normal.jpg";
//
//  return sge;
//}

SceneGraphEntity SceneGraph::makeCurve(std::string name) {
  auto curve = makeEntity(std::move(name));

  curve.add<geo::Curve>();
  curve.add<glrs::Renderable>();
  auto& s = curve.add<geo::Material>({.name = "default curve material",
                                      .vertexShader = "assets/shaders/default_curve.vert.spv",
                                      .fragmentShader = "assets/shaders/default_curve.frag.spv"});

  auto block = new glrs::ColorBlock;
  block->color = glm::vec4(0.45F, 0.52F, 0.68F, 0.0F);
  s.setProperty("ColorBlock", block);
  // TODO: Add color block manager to material system,
  //  OR make a separate list of constantUniformBlocks that dont need management,
  //  OR whenever you look for ColorBlock manager and you cannot find one, then ignore.
  //  s.registerUniformBlock("ColorBlock", block, sizeof(ecs::ColorBlock));

  return curve;
}

void SceneGraph::processChildren(std::shared_ptr<const geo::SceneNode> node, SceneLoaderFlags flags, SceneGraphEntity* parent_p) {
  auto root = makeEntity(node->name);
  root.setTransform(node->transform);

  // if current processed scene node is not a root node.
  // then add it to it's parent.
  if (parent_p != nullptr) {
    parent_p->addChild(root);
  }

  // Add lights
  if (flags.lights) {
    for (auto& light : node->lights) {
      auto lightEntity = makeLight(*light);
      root.addChild(lightEntity);
    }
  }

  // Add cameras
  if (flags.cameras) {
    for (auto& camera : node->cameras) {
      auto cameraEntity = makeCamera(*camera);
      root.addChild(cameraEntity);
    }
  }

  if (flags.geometry) {
    // add mesh instances for this root
    for (auto& instance : node->meshInstances) {
      // make shader from material
      // TODO: make a material system that loads a default
      //    auto material =
      //        m_resourceManager->getResource<geo::Material>("UberMaterial");
      //
      //    (*material)->diffuseTexture = (*instance->material)->diffuseTexture;
      //
      //    (*material)->setProperty<geo::UberMaterialData>(
      //        {.diffuse = glm::vec4((*instance->material)->diffuse, 1.0F),
      //         .specular = glm::vec4((*instance->material)->specular, 1.0F),
      //         .ambient = glm::vec4((*instance->material)->ambient, 1.0F),
      //         .hasDiffuseTex = (*instance->material)->diffuseTexture !=
      //         nullptr});

      // create mesh instance mesh data
      // TODO: make mesh instance instead of copy mesh
      //    auto e = makeRenderable(*instance->mesh, instance->transform,
      //    **material);
      auto e = makeRenderable(*instance->mesh, instance->transform, *instance->material);

      root.addChild(e);
    }
  }

  for (auto& child : node->children) {
    processChildren(child, flags, &root);
  }
}

SceneGraphEntity SceneGraph::makeScene(const geo::Scene& scene, SceneLoaderFlags flags) {
  auto rootScene = makeEntity();

  // for geometry and mesh data
  processChildren(scene.rootNode(), flags, &rootScene);

  return rootScene;
}

SceneGraphEntity SceneGraph::makeRenderable(const geo::Mesh& mesh, glm::mat4 transform, geo::Material& material) {
  auto e = makeEntity(mesh.name);
  e.setTransform(std::move(transform));

  //  auto materialId =
  //  m_resourceManager->getResource<geo::Material>(materialName)->id();
  e.add<glrs::Renderable>();
  e.add<geo::Mesh>(mesh);
  e.add<geo::Material>(material);

  return e;
}

SceneGraphEntity SceneGraph::makeRenderable(geo::Mesh&& mesh, const geo::Material& material) {
  auto e = m_registry.create();
  m_registry.emplace<ecs::Transform>(e);
  m_registry.emplace<glrs::Renderable>(e);
  m_registry.emplace<geo::Mesh>(e, std::move(mesh));
  m_registry.emplace<geo::Material>(e, material);

  return SceneGraphEntity{m_registry, e};
}

SceneGraphEntity SceneGraph::makeRenderable(geo::Curve&& curve, const geo::Material& material) {
  auto e = m_registry.create();
  m_registry.emplace<ecs::Transform>(e);
  m_registry.emplace<glrs::Renderable>(e);
  m_registry.emplace<geo::Curve>(e, std::move(curve));
  m_registry.emplace<geo::Material>(e, material);

  return SceneGraphEntity{m_registry, e};
}

entt::entity makeTexture();

SceneGraphEntity SceneGraph::makeRectangularGrid(float size, float spacing) {
  auto sge = makeCurve("RectangularGrid");
  auto& curve = sge.get<geo::Curve>();

  float z = 0.0F;

  // initial center line axis
  curve.vertices.push_back(-size);
  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(z);

  curve.vertices.push_back(size);
  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(z);

  // initial center line axis
  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(-size);

  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(0.0F);
  curve.vertices.push_back(size);

  while (z - spacing < size) {
    // positive side horizontal
    curve.vertices.push_back(-size);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(z);

    curve.vertices.push_back(size);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(z);

    curve.vertices.push_back(-size);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(-z);

    curve.vertices.push_back(size);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(-z);

    // positive side vertical
    curve.vertices.push_back(z);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(-size);

    curve.vertices.push_back(z);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(size);

    curve.vertices.push_back(-z);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(-size);

    curve.vertices.push_back(-z);
    curve.vertices.push_back(0.0F);
    curve.vertices.push_back(size);

    z += spacing * 2.0F;
  }

  return sge;
}
