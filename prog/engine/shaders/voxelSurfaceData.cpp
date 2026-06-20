// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_voxelSurfaceData.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_ptrTab.h>
#include <ioSys/dag_zstdIo.h>
#include <mutex>


static PtrTab<VoxelSurfaceData> all_data(midmem_ptr());
static std::mutex data_mutex;

namespace vars
{
static ShaderVariableInfo voxel_color_atlas("voxel_color_atlas", true);
static ShaderVariableInfo voxel_normal_atlas("voxel_normal_atlas", true);
static ShaderVariableInfo voxel_atlas_offsets("voxel_atlas_offsets", true);
} // namespace vars


void VoxelSurfaceData::setToShader() const
{
  auto [rgbaTex, rgbaOfs] = voxeldata::rgbaAtlasManager.getChunkPosition(rgbaLoc);
  auto [normTex, normOfs] = voxeldata::normAtlasManager.getChunkPosition(normLoc);
  vars::voxel_color_atlas.set_texture(rgbaTex);
  vars::voxel_normal_atlas.set_texture(normTex);
  vars::voxel_atlas_offsets.set_int4(rgbaOfs, normOfs, 0, 0);
}

void VoxelSurfaceData::setToShader(uint32_t data_id)
{
  if (auto vd = getById(data_id))
  {
    if (!vd->isLoadedToAtlas())
      vd->loadToAtlas();
    vd->setToShader();
  }
  else
  {
    vars::voxel_color_atlas.set_texture(nullptr);
    vars::voxel_normal_atlas.set_texture(nullptr);
    vars::voxel_atlas_offsets.set_int4(0, 0, 0, 0);
  }
}


void VoxelSurfaceData::onAfterD3dReset()
{
  voxeldata::rgbaAtlasManager.onAfterD3dReset();
  voxeldata::normAtlasManager.onAfterD3dReset();

  std::lock_guard guard(::data_mutex);
  bool failed = false;
  for (auto &p : ::all_data)
    if (p)
    {
      p->loadedToTex = false;
      if (!p->loadToAtlas())
        failed = true;
    }
  if (failed)
    logerr("failed to load voxel surface data to atlas textures");
}


VoxelSurfaceData::VoxelSurfaceData() {}

VoxelSurfaceData::~VoxelSurfaceData()
{
  std::lock_guard guard(::data_mutex);
  if (id < ::all_data.size())
    ::all_data[id] = nullptr;
  compressed = {};
}

VoxelSurfaceData *VoxelSurfaceData::load(IGenLoad &crd, int size)
{
  if (size == 0)
    return nullptr;

  auto res = (VoxelSurfaceData *)memalloc(dumpStartOfs() + size, midmem);
  new (res, _NEW_INPLACE) VoxelSurfaceData();
  crd.read(res->dumpStartPtr(), size);
  res->compressed = make_span((char *)res->dataStartPtr(), size - ((uint8_t *)res->dataStartPtr() - (uint8_t *)res->dumpStartPtr()));

  std::lock_guard guard(::data_mutex);
  for (uint32_t i = 0, n = ::all_data.size(); i < n; i++)
    if (!::all_data[i])
    {
      ::all_data[i] = res;
      res->id = i;
      return res;
    }

  res->id = ::all_data.size();
  ::all_data.push_back(res);
  return res;
}

VoxelSurfaceData *VoxelSurfaceData::getById(uint32_t id)
{
  if (id == INVALID_ID)
    return nullptr;
  std::lock_guard guard(::data_mutex);
  if (id < ::all_data.size())
    return ::all_data[id].get();
  return nullptr;
}

eastl::array<uint32_t, 2> VoxelSurfaceData::allocateAtlas()
{
  if (rgbaLoc.valid() and normLoc.valid())
    return {0, 0};

  loadedToTex = false;
  uint32_t rgbaNum = 0;
  if (!rgbaLoc.valid())
  {
    rgbaLoc = voxeldata::rgbaAtlasManager.allocateChunk(numRgbaBlocks * 2);
    if (!rgbaLoc.valid())
      rgbaNum = numRgbaBlocks * 2;
  }
  uint32_t normNum = 0;
  if (!normLoc.valid())
  {
    normLoc = voxeldata::normAtlasManager.allocateChunk(numNormBlocks);
    if (!normLoc.valid())
      normNum = numNormBlocks;
  }
  return {rgbaNum, normNum};
}

eastl::array<uint32_t, 2> VoxelSurfaceData::releaseAtlas()
{
  loadedToTex = false;
  uint32_t rgbaNum = 0;
  if (rgbaLoc.valid())
  {
    voxeldata::rgbaAtlasManager.releaseChunk({rgbaLoc, numRgbaBlocks * 2});
    rgbaLoc = {};
    rgbaNum = numRgbaBlocks * 2;
  }
  uint32_t normNum = 0;
  if (normLoc.valid())
  {
    voxeldata::normAtlasManager.releaseChunk({normLoc, numNormBlocks});
    normLoc = {};
    normNum = numNormBlocks;
  }
  return {rgbaNum, normNum};
}

bool VoxelSurfaceData::loadToAtlas()
{
  bool ok = true;
  if (numRgbaBlocks > 0 and rgbaLoc.valid())
  {
    DAGOR_TRY
    {
      ZstdLoadFromMemCB reader(compressed.first(normCompressedOffset));
      if (!voxeldata::rgbaAtlasManager.loadChunkData({rgbaLoc, numRgbaBlocks * 2}, reader))
        ok = false;
    }
    DAGOR_CATCH(...) { ok = false; }
  }
  if (numNormBlocks > 0 and normLoc.valid())
  {
    DAGOR_TRY
    {
      ZstdLoadFromMemCB reader(compressed.subspan(normCompressedOffset));
      if (!voxeldata::normAtlasManager.loadChunkData({normLoc, numNormBlocks}, reader))
        ok = false;
    }
    DAGOR_CATCH(...) { ok = false; }
  }
  if (ok)
    loadedToTex = true;
  return ok;
}
