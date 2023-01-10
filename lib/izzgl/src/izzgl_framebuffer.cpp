//
// Created by jeffrey on 6-1-23.
//
#include <izzgl_framebuffer.h>
#include <izzgl_gpu.h>
using namespace izz::gl;

Framebuffer::Framebuffer(izz::gl::Gpu& gpu, uint32_t fbo)
  : m_gpu{gpu}
  , m_fbo{fbo} {}

uint32_t Framebuffer::bufferId() {
  return m_fbo;
}

void Framebuffer::bind() {
  m_gpu.bindFramebuffer(this);
}

void Framebuffer::bindAsRead() {
  m_gpu.bindReadFramebuffer(this);
}

void Framebuffer::bindAsDestination() {
  m_gpu.bindDrawFramebuffer(this);
}