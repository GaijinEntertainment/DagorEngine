#include "riGen/riGenRender.h"
#include <gpuObjects/gpuObjects.h>
#include <3d/dag_drv3dCmd.h>

void erase_gpu_objects(const Point3 &point, const float radius)
{
  if (gpu_objects_mgr)
  {
    gpu_objects_mgr->addBombHole(point, radius);
    d3d::GpuAutoLock gpu_al;
    BBox2 bbox{point.x - radius, point.z + radius, point.x + radius, point.z - radius};
    gpu_objects_mgr->invalidateBBox(bbox);
  }
}
