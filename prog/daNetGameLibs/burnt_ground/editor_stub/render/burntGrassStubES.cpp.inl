// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>
#include <math/dag_hlsl_floatx.h>

#include "burnt_ground/shaders/burnt_grass.hlsli"

static UniqueBufWithShaderVar burnt_tiles_stub;
static UniqueBufWithShaderVar burnt_tile_indices_stub;
static bool initialized = false;

static void fill_buffer(auto &buf, uint32_t size, uint32_t value)
{
  if (auto data = lock_sbuffer<uint32_t>(buf.getBuf(), 0, size, VBLOCK_WRITEONLY))
  {
    for (size_t i = 0; i < size; ++i)
      data[i] = value;
  }
}

ECS_TAG(render)
static void burnt_grass_renderer_stub_lazy_setup_es(const UpdateStageInfoBeforeRender &)
{
  if (!eastl::exchange(initialized, true))
  {
    if (int tilesVarId = ::get_shader_variable_id("burnt_grass_tiles"); tilesVarId >= 0)
    {
      constexpr int GPU_TEXTURE_TILES = 512;
      constexpr int BURNT_GRASS_TILES_BUF_SIZE = BURNT_GRASS_TILE_RESOLUTION * BURNT_GRASS_TILE_RESOLUTION * GPU_TEXTURE_TILES;
      burnt_tiles_stub = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), BURNT_GRASS_TILES_BUF_SIZE, "burnt_grass_tiles");
      burnt_tiles_stub.setVar();
      fill_buffer(burnt_tiles_stub, BURNT_GRASS_TILES_BUF_SIZE, BURNT_GRASS_NO_FIRE);
    }
    if (int tilesIndicesVarId = ::get_shader_variable_id("burnt_grass_tiles_indices"); tilesIndicesVarId >= 0)
    {
      constexpr int INDEX_BUFFER_SIZE = BURNT_GRASS_INDEX_MAP_RESOLUTION * BURNT_GRASS_INDEX_MAP_RESOLUTION;
      burnt_tile_indices_stub =
        dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), INDEX_BUFFER_SIZE, "burnt_grass_tiles_indices");
      burnt_tile_indices_stub.setVar();
      fill_buffer(burnt_tile_indices_stub, INDEX_BUFFER_SIZE, BURNT_GRASS_INVALID_GPU_TILE_INDEX);
    }
  }
}
