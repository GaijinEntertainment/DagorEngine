// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderMeshlets.h>
#include <shaders/dag_shaderMesh.h>
#include <drv/3d/dag_driverDesc.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/algorithm.h>


bool shader_meshlets::is_enabled()
{
  static const bool enabled =
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableMeshlets", false) && d3d::get_driver_desc().caps.hasMeshShader;
  return enabled;
}

shader_meshlets::VdataElemSpan shader_meshlets::VdataElemSpan::forShaderMatVdataIndex(int smvd_idx) const
{
  G_ASSERT(selectedSmvdIndex < 0);

  const VdataElemRange *first = eastl::lower_bound(elems.begin(), elems.end(), smvd_idx,
    [](const VdataElemRange &e, int idx) { return e.shaderMatVdataIndex < idx; });
  const VdataElemRange *last =
    eastl::upper_bound(first, elems.end(), smvd_idx, [](int idx, const VdataElemRange &e) { return idx < e.shaderMatVdataIndex; });
  return VdataElemSpan(dag::ConstSpan<VdataElemRange>(first, last - first), smvd_idx);
}

shader_meshlets::VdataElemSpan shader_meshlets::VdataElemSpan::forVdataIndex(int vd_idx) const
{
  G_ASSERT(selectedSmvdIndex >= 0 && selectedVdataIndex < 0);

  const VdataElemRange *first =
    eastl::lower_bound(elems.begin(), elems.end(), vd_idx, [](const VdataElemRange &e, int idx) { return e.vdataIndex < idx; });
  const VdataElemRange *last =
    eastl::upper_bound(first, elems.end(), vd_idx, [](int idx, const VdataElemRange &e) { return idx < e.vdataIndex; });
  return VdataElemSpan(dag::ConstSpan<VdataElemRange>(first, last - first), selectedSmvdIndex, vd_idx);
}

shader_meshlets::GlobalMeshletData *shader_meshlets::build(const GlobalVertexData &vd, VdataElemSpan vd_elems, const uint8_t *vb,
  const uint16_t *ib)
{
  G_UNUSED(vd);
  G_UNUSED(vd_elems);
  G_UNUSED(vb);
  G_UNUSED(ib);
  return GlobalMeshletData::create();
}

shader_meshlets::GlobalMeshletData *shader_meshlets::GlobalMeshletData::create()
{
  return new (memalloc(sizeof(GlobalMeshletData), midmem), _NEW_INPLACE) GlobalMeshletData;
}

void shader_meshlets::GlobalMeshletData::destroy(GlobalMeshletData *d)
{
  if (d)
    memfree(d, midmem);
}
