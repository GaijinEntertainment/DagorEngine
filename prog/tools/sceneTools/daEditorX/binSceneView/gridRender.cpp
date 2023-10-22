#include "gridRender.h"
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>

void GridRender::render() { gridRendElem->render(true, 0); }

bool GridRender::init(const char *shader_name, float ht, int subdiv, const Point2 &min, const Point2 &max)
{
  gridMaterial = new_shader_material_by_name(shader_name, NULL);
  if (!gridMaterial)
  {
    logerr("no material for grid <%s>", shader_name);
    return false;
  }
  // gridMaterial->addRef();

  static CompiledShaderChannelId chan[1] = {
    {SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0},
  };

  if (!gridMaterial->checkChannels(chan, sizeof(chan) / sizeof(chan[0])))
  {
    logerr("invalid channels for grid <%s>", shader_name);
    del_it(gridMaterial);
    return false;
  }
  gridRendElem = new dynrender::RElem;
  gridRendElem->shElem = gridMaterial->make_elem();
  gridRendElem->vDecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));
  G_ASSERT(gridRendElem->vDecl != BAD_VDECL);
  gridRendElem->stride = dynrender::getStride(chan, sizeof(chan) / sizeof(chan[0]));
  gridRendElem->minVert = 0;
  gridRendElem->numVert = subdiv * subdiv;
  gridRendElem->startIndex = 0;
  gridRendElem->numPrim = ((subdiv - 1) * (subdiv - 1) * 2);

  gridVb = d3d::create_vb(gridRendElem->numVert * gridRendElem->stride, SBCF_MAYBELOST, "gridVb");
  d3d_err(gridVb);

  gridIb = d3d::create_ib(gridRendElem->numPrim * 2 * 3, SBCF_MAYBELOST);
  d3d_err(gridIb);
  gridRendElem->vb = gridVb;
  gridRendElem->ib = gridIb;
  struct WaterStubVertex
  {
    Point3 pos;
  };

  G_ASSERT(sizeof(WaterStubVertex) == gridRendElem->stride);

  WaterStubVertex *vertices;
  d3d_err(gridVb->lock(0, gridRendElem->numVert * gridRendElem->stride, (void **)&vertices, VBLOCK_WRITEONLY));
  Point2 width = max - min;
  for (int i = 0, y = 0; y < subdiv; ++y)
    for (int x = 0; x < subdiv; ++x, ++i)
      vertices[i].pos = Point3(min.x + width.x * (x / (subdiv - 1.f)), ht, min.y + width.y * (y / (subdiv - 1.f)));
  gridVb->unlock();

  uint16_t *indices = NULL;
  d3d_err(gridIb->lock(0, gridRendElem->numPrim * 2 * 3, &indices, VBLOCK_WRITEONLY));
  for (int y = 0; y < subdiv - 1; ++y)
    for (int x = 0; x < subdiv - 1; ++x, indices += 6)
    {
      indices[0] = y * subdiv + x;
      indices[1] = indices[0] + 1;
      indices[2] = indices[0] + subdiv;
      indices[3] = indices[1];
      indices[4] = indices[2];
      indices[5] = indices[2] + 1;
    }

  gridIb->unlock();
  return true;
}

void GridRender::close()
{
  del_it(gridMaterial);
  if (gridRendElem)
    gridRendElem->shElem = NULL;
  del_it(gridRendElem);
  del_d3dres(gridVb);
  del_d3dres(gridIb);
}
