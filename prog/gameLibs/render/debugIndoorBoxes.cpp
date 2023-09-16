#include <3d/dag_drv3d.h>
#include <shaders/dag_shaders.h>

#include "render/debugIndoorBoxes.h"
#include <render/lightProbeSpecularCubesContainer.h>

DebugIndoorBoxes::DebugIndoorBoxes(const char *shaderName)
{
  mat.reset(new_shader_material_by_name(shaderName));
  if (mat)
  {
    mat->addRef();
    elem = mat->make_elem();
  }
}

void DebugIndoorBoxes::render() const
{
  if (!mat)
    return;

  elem->setStates(0, true);

  uint32_t boxCount = LightProbeSpecularCubesContainer::INDOOR_PROBES;
  d3d::draw(PRIM_TRILIST, 0, 12 * boxCount);
}
