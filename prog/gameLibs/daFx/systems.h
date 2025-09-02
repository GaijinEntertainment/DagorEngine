// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include "binds.h"
#include "buffers.h"
#include "shaders.h"
#include "emitters.h"
#include <drv/3d/dag_consts.h>
#include <3d/dag_resMgr.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <util/dag_generationReferencedData.h>

namespace dafx
{
using SystemNameMap = eastl::hash_map<eastl::string, SystemId>;

enum
{
  SYS_VALID = 1 << 0,
  SYS_ENABLED = 1 << 1,
  SYS_RENDERABLE = 1 << 2,
  SYS_GPU_TRANFSERABLE = 1 << 3,

  SYS_EMITTER_REQ = 1 << 4,

  SYS_CPU_EMISSION_REQ = 1 << 5,
  SYS_GPU_EMISSION_REQ = 1 << 6,

  SYS_CPU_SIMULATION_REQ = 1 << 7,
  SYS_GPU_SIMULATION_REQ = 1 << 8,

  SYS_RENDER_REQ = 1 << 9,
  SYS_CPU_RENDER_REQ = 1 << 10,
  SYS_GPU_RENDER_REQ = 1 << 11,
  SYS_GPU_TRANFSER_REQ = 1 << 12,
  SYS_GPU_TRANFSER_PENDING = 1 << 13,

  SYS_VISIBLE = 1 << 14,
  SYS_BBOX_VALID = 1 << 15,
  SYS_CULL_FETCHED = 1 << 16,
  SYS_RENDER_BUF_TAKEN = 1 << 17,

  SYS_POS_VALID = 1 << 18,

  SYS_SKIP_SCREEN_PROJ_CULL_DISCARD = 1 << 19,

  SYS_SKIP_SIMULATION_ON_THIS_FRAME = 1 << 20,
  SYS_ALLOW_SIMULATION_LODS = 1 << 21,

  SYS_DUMMY_SYSTEM = 1 << 31,
};

struct SystemTemplate
{
  int depth;
  uint32_t refFlags;
  uint32_t qualityFlags;
  eastl::string name;
  int gameResId;
  float spawnRangeLimit;

  EmitterState emitterState;
  EmitterRandomizer emitterRandomizer;

  GpuComputeShaderId gpuEmissionShaderId;
  GpuComputeShaderId gpuSimulationShaderId;

  CpuComputeShaderId cpuEmissionShaderId;
  CpuComputeShaderId cpuSimulationShaderId;

  RefDataId renderRefDataId;
  RefDataId simulationRefDataId;
  RefDataId serviceRefDataId;
  ValueBindId valueBindId;

  unsigned int renderElemSize;
  unsigned int simulationElemSize;

  unsigned int renderDataSize;
  unsigned int simulationDataSize;

  unsigned int renderPrimPerPart;

  unsigned int localGpuDataSize;
  unsigned int localCpuDataSize;
  unsigned int totalGpuDataSize;
  unsigned int totalCpuDataSize;
  unsigned int serviceDataSize;

  uint32_t renderTags;
  int renderSortDepth;
  eastl::vector<RenderShaderId> renderShaders;
  eastl::array<eastl::vector<TextureDesc>, STAGE_MAX> resources;

  eastl::vector<SystemTemplate> subsystems;
};

struct Systems
{
  SystemId dummySystemId;
  SystemNameMap nameMap;
  GenerationReferencedData<SystemId, SystemTemplate> list;
};
} // namespace dafx