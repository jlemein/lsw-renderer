//
// Created by jlemein on 10-01-21.
//

#ifndef ARTIFAX_ECS_DEBUGMODEL_H
#define ARTIFAX_ECS_DEBUGMODEL_H

#include <ecs_renderable.h>
#include <ecs_transform.h>
#include <ecs_name.h>
#include <geo_mesh.h>

namespace affx {
namespace ecs {

/**!
 * A collection of items needed to render a debug model.
 */
 // TODO the data is very related to ECS. Change this to create a higher level
 //  wrapper that models a generic model.
struct DebugModel {
  std::vector<Renderable> renderable;
  std::vector<Shader> shader;
  std::vector<Name> names;
  std::vector<geo::Mesh> mesh;
  std::vector<ecs::Transform> transformations;
//  geo::Curve curve;
};

} // end of package
} // end of enterprise

#endif // ARTIFAX_ECS_DEBUGMODEL_H
