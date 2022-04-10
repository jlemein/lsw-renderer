//
// Created by jeffrey on 19-01-22.
//
#include <GL/glew.h>
#include <ecs_camera.h>
#include <gl_deferredrenderer.h>
#include <gl_rendersystem.h>
#include <gl_renderutils.h>
#include <izz_scenegraphhelper.h>
#include <izzgl_materialsystem.h>
#include <entt/entt.hpp>
using namespace izz::gl;

DeferredRenderer::DeferredRenderer(izz::gl::RenderSystem& renderSystem, entt::registry& registry)
  : m_renderSystem{renderSystem}
  , m_registry{registry} {
//  m_registry.on_construct<gl::DeferredRenderable>().connect<&DeferredRenderer::onConstruct>(this);
}

void DeferredRenderer::onConstruct(entt::registry& registry, entt::entity e) {
//  DeferredRenderable& r = registry.get<DeferredRenderable>(e);
//
//  // if render state exists
//  std::cout << "Hello On construct - renderstateid = " << r.renderStateId << std::endl;
//  RenderState& rs = r.renderStateId == -1 ? m_renderSystem.createRenderState() : m_renderSystem.getRenderState(r.renderStateId);
//  r.renderStateId = rs.id;
//
//
//  if (r.meshEntity != entt::null) {
//    auto pCurve = m_registry.try_get<lsw::geo::Curve>(r.meshEntity);
//    if (pCurve != nullptr) {
//      RenderUtils::FillBufferedMeshData(*pCurve, rs.meshData);
//    }
//
//    auto pMesh = m_registry.try_get<lsw::geo::Curve>(r.meshEntity);
//    if (pMesh != nullptr) {
//      RenderUtils::FillBufferedMeshData(*pMesh, rs.meshData);
//    }
//  }
//
//  if (r.materialId != -1) {
//    auto& material = m_renderSystem.getMaterialSystem().getMaterialById(r.materialId);
//    RenderUtils::LoadMaterial(material, rs);
//    // MVP
//    r.mvp = RenderUtils::GetUniformBufferLocation(rs, "UniformBufferBlock");
//    //    r.lights = RenderUtils::GetUniformBufferLocation(rs, "ForwardLighting");
//  } else {
//    spdlog::warn("DeferredRenderable is incomplete. Does not contain material properties.");
//  }
}

// void DeferredRenderer::resize(int width, int height) {
//   m_screenWidth = width;
//   m_screenHeight = height;
// }


void DeferredRenderer::init(int width, int height) {
  m_screenWidth = width;
  m_screenHeight = height;

  // Create a framebuffer for the gbuffer (geometry pass).
  glGenFramebuffers(1, &m_gBufferFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFbo);

  // GBuffer: position texture
  glGenTextures(1, &m_gPosition);
  glBindTexture(GL_TEXTURE_2D, m_gPosition);  // so that all subsequent calls will affect position texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

  // GBuffer: normal texture
  glGenTextures(1, &m_gNormal);
  glBindTexture(GL_TEXTURE_2D, m_gNormal);  // so that all subsequent calls will affect normal texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

  // GBuffer: albedoTexture
  glGenTextures(1, &m_gAlbedoSpec);
  glBindTexture(GL_TEXTURE_2D, m_gAlbedoSpec);  // so that all subsequent calls will affect albedoSpec texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gAlbedoSpec, 0);

  // handling curves
  for (auto [entity, curve, r] : m_registry.view<lsw::geo::Curve, DeferredRenderable>().each()) {
    try {
      RenderUtils::FillBufferedMeshData(curve, m_renderSystem.getRenderState(r.renderStateId).meshData);
    } catch (std::exception& e) {
      auto name = m_registry.all_of<lsw::ecs::Name>(entity) ? m_registry.get<lsw::ecs::Name>(entity).name : "Unnamed";
      throw std::runtime_error(fmt::format("Failed initializing curve '{}': {}", name, e.what()));
    }
  }

  // handling meshes
  for (auto [entity, mesh, r] : m_registry.view<lsw::geo::Mesh, DeferredRenderable>().each()) {
    try {
      RenderUtils::FillBufferedMeshData(mesh, m_renderSystem.getRenderState(r.renderStateId).meshData);
    } catch (std::exception& e) {
      auto name = m_registry.all_of<lsw::ecs::Name>(entity) ? m_registry.get<lsw::ecs::Name>(entity).name : "Unnamed";
      throw std::runtime_error(fmt::format("Failed initializing mesh '{}': {}", name, e.what()));
    }
  }

  spdlog::info("=== Deferred Renderer Initialization Complete ===");
}

void DeferredRenderer::update() {
  // Updates the Render system updates the model view projection matrix for each of the
  if (m_activeCamera == entt::null) {
    throw std::runtime_error("No active camera in scene");
  }

  auto& cameraTransform = m_registry.get<lsw::ecs::Transform>(m_activeCamera);
  glm::mat3 view = glm::inverse(cameraTransform.worldTransform);

  auto& activeCamera = m_registry.get<lsw::ecs::Camera>(m_activeCamera);
  glm::mat3 proj = glm::perspective(activeCamera.fovx, activeCamera.aspect, activeCamera.zNear, activeCamera.zFar);

  for (auto [e, r] : m_registry.view<DeferredRenderable>().each()) {
    auto transform = m_registry.try_get<lsw::ecs::Transform>(e);
    auto model = transform != nullptr ? transform->worldTransform : glm::mat4(1.0F);

    try {
      auto& material = m_renderSystem.getMaterialSystem().getMaterialById(r.materialId);
      ModelViewProjection* mvp = reinterpret_cast<ModelViewProjection*>(material.uniformBlocks.at("ModelViewProjection").data);
      mvp->model = model;
      mvp->view = view;
      mvp->proj = proj;
      mvp->viewPos = glm::vec3(transform->worldTransform[3]);
    } catch(std::out_of_range&) {
      throw std::runtime_error(fmt::format("(e: {}): cannot access MVP matrix. Does material have 'MVP' ubo?", e));
    }
  }
}

void DeferredRenderer::render(const entt::registry& registry) {
  static unsigned int colorAttachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
  glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
//  glClearColor(1.0, 1.0, 0.0, 0.0);
  //  glDrawBuffers(3, colorAttachments);

  // clear gbuffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto view = registry.view<const DeferredRenderable, const lsw::ecs::Transform>();

  for (entt::entity e : view) {
    try {
      auto rid = view.get<const DeferredRenderable>(e).renderStateId;
      const RenderState& rs = m_renderSystem.getRenderState(rid);

      //    RenderUtils::ActivateProgram();
      glUseProgram(rs.program);
      RenderUtils::ActivateTextures(rs);
      RenderUtils::UseBufferedMeshData(rs.meshData);

      // TODO: check if shader is dirty
      //  reason: if we push properties every frame (Except for MVP), we might
      //  unnecessary spend time doing that while we can immediately just render.
      RenderUtils::PushUniformProperties(rs);

      // model view projection matrix
      // model: determined by object - transform
      // projection: determined by camera
      // view: determined by camera position

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      if (rs.meshData.primitiveType == GL_TRIANGLES) {
        glDrawElements(rs.meshData.primitiveType, rs.meshData.drawElementCount, GL_UNSIGNED_INT, 0);
      } else {
        glDrawArrays(rs.meshData.primitiveType, 0, rs.meshData.drawElementCount);
      }
    } catch (std::exception& exc) {
      std::string msg = "";

      if (registry.all_of<lsw::ecs::Name>(e)) {
        auto& name = registry.get<lsw::ecs::Name>(e);
        msg = fmt::format("(e: {}) Rendering entity: {} - {}", static_cast<int>(e), name.name, exc.what());
      } else {
        msg = exc.what();
      }
      throw std::runtime_error(msg);
    }
  }

  // --- GBUFFER pass is finished. GBuffer is what we have -----
  //
  /* We are going to blit into the window (default framebuffer)                     */
  //  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  //  glDrawBuffer(GL_BACK); /* Use backbuffer as color dst.         */
  //
  //  /* Read from your FBO */
  //  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gBufferFbo);
  //  glReadBuffer(GL_COLOR_ATTACHMENT0); /* Use Color Attachment 0 as color src. */
  //
  //  glClearColor(1.0, 0.0, 0., 0.0);
  //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //  glClearColor(0.0,0.0,0.0,0.0);
  //  /* Copy the color and depth buffer from your FBO to the default framebuffer       */
  //  glBlitFramebuffer(0, 0, m_screenWidth, m_screenHeight, 0, 0, m_screenWidth, m_screenHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

