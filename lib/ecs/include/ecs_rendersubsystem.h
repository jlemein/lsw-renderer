//
// Created by jlemein on 09-11-20.
//

#ifndef GLVIEWER_ECS_RENDERSUBSYSTEM_H
#define GLVIEWER_ECS_RENDERSUBSYSTEM_H

#include <entt/entt.hpp>

namespace affx {
namespace ecs
{
class IRenderSubsystem
{
 public:
  virtual void
  onRender(const entt::registry &registry, entt::entity entity) = 0;
};

} // end package
} // namespace affx

#endif  // GLVIEWER_ECS_RENDERSUBSYSTEM_H
