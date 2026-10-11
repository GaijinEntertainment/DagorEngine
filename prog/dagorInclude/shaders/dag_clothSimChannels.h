//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef __DAG_CLOTH_SIM_CHANNELS_INCLUDED__
#define __DAG_CLOTH_SIM_CHANNELS_INCLUDED__

#define CLOTH_MAX_VERTICES    8192
#define CLOTH_MAX_EDGE_COLORS 128 // TODO: can lower it with better assets for perf


// Must be consistent with cloth_sim_skinning_inc.dshl (extra[58]/extra[59]) and dynamic_cloth_sim.dshl (extra[54]).
// Taken SCUSAGE_EXTRA indices: 0-3 bone channels and 100 tangents (shSkinMeshData, 2 bone channels per 4 bones),
// 50/51 texture space (shaderMeshData), 52 sun bump (nsb_StaticVisualScene), 53 vertex colour, 55 two-sided side
// and 100-102 AO (dynSceneResSrc), 56/57 domain UV (rendInstResSrc), 60 center of rotation (dag_corSkinningChannel.h).
#ifdef __cplusplus
enum ClothSimExtraChannel
{
  CLOTH_SKINNING_INDEX_EXTRA_CHANNEL = 58,
  CLOTH_SKINNING_WEIGHT_EXTRA_CHANNEL = 59,
  CLOTH_SIM_WEIGHT_EXTRA_CHANNEL = 54,
};
#endif

#endif // __DAG_CLOTH_SIM_CHANNELS_INCLUDED__
