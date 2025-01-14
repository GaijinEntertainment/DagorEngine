// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

LRUCollisionVoxelization::~LRUCollisionVoxelization()
{
  if (d3d::get_driver_desc().caps.hasWellSupportedIndirect)
  {
    if (renderCollisionElem)
      d3d::delete_vdecl(renderCollisionElem->getEffectiveVDecl());
    if (voxelizeCollisionElem)
      d3d::delete_vdecl(voxelizeCollisionElem->getEffectiveVDecl());
  }
}

void LRUCollisionVoxelization::init()
{
  renderCollisionElem = nullptr;
  voxelizeCollisionElem = nullptr;
  renderCollisionMat.reset(new_shader_material_by_name_optional("render_lru_collision"));
  if (renderCollisionMat)
  {
    renderCollisionMat->addRef();
    renderCollisionElem = renderCollisionMat->make_elem();
  }
  voxelizeCollisionMat.reset(new_shader_material_by_name_optional("world_sdf_collision_rasterize"));
  if (voxelizeCollisionMat)
  {
    voxelizeCollisionMat->addRef();
    voxelizeCollisionElem = voxelizeCollisionMat->make_elem();
  }
  else
    logerr("no world_sdf_collision_rasterize shader");
  const bool multiDrawIndirectSupported = d3d::get_driver_desc().caps.hasWellSupportedIndirect;
  if (multiDrawIndirectSupported && renderCollisionElem)
  {
    VSDTYPE vsdInstancing[] = {VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_HALF4), VSD_STREAM_PER_INSTANCE_DATA(1),
      VSD_REG(VSDR_TEXC0, VSDT_INT1), VSD_END};
    renderCollisionElem->replaceVdecl(d3d::create_vdecl(vsdInstancing));
  }
  if (multiDrawIndirectSupported && voxelizeCollisionElem)
  {
    VSDTYPE vsdInstancing[] = {VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_HALF4), VSD_STREAM_PER_INSTANCE_DATA(1),
      VSD_REG(VSDR_TEXC0, VSDT_INT1), VSD_END};
    voxelizeCollisionElem->replaceVdecl(d3d::create_vdecl(vsdInstancing));
  }
  voxelize_collision_cs.reset(new_compute_shader("world_sdf_voxelize_collision_cs"));
}

void LRUCollisionVoxelization::rasterize(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles, VolTexture *world_sdf,
  VolTexture *world_sdf_alpha, ShaderElement *e, int instMul, bool prim)
{
  if (!e)
    return;
  DA_PROFILE_GPU;
  lruColl.updateLRU(handles);
  lruColl.draw(handles, world_sdf, world_sdf_alpha, instMul, *e, prim);
}

void LRUCollisionVoxelization::rasterizeSDF(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles, bool prim)
{
  rasterize(lruColl, handles, nullptr, nullptr, voxelizeCollisionElem, 3, prim);
}

void LRUCollisionVoxelization::voxelizeSDFCompute(LRURendinstCollision &lruColl, dag::ConstSpan<uint64_t> handles)
{
  if (!voxelize_collision_cs)
    return;
  DA_PROFILE_GPU;
  lruColl.updateLRU(handles);
  lruColl.dispatchInstances(handles, nullptr, nullptr, *voxelize_collision_cs);
}
