#include <rendInst/rendInstGen.h>
#include <ecs/core/entityManager.h>
#include <gpuObjects/placingParameters.h>

gpu_objects::PlacingParameters gpu_objects::prepare_gpu_object_parameters(int, const Point3 &, float, const Point2 &, const Point2 &,
  const Point4 &, const bool, const String &, const Point2 &, const Point2 &, const E3DCOLOR &, const E3DCOLOR &, const Point2 &,
  const ecs::Array &, const float &, const bool, const bool, const bool, const float &, const bool, const bool, const bool,
  const Point2 &, const bool)
{
  return gpu_objects::PlacingParameters();
}