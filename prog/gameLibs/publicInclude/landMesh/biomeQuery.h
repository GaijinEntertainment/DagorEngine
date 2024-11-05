//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include "gpuReadbackQuery/gpuReadbackResult.h"
#include <math/dag_hlsl_floatx.h>
#include "landMesh/biome_query_result.hlsli"
#include <drv/3d/dag_resId.h>

class Point3;
struct BiomeQueryResult;

namespace biome_query
{
void init();
void init_land_class(TEXTUREID lc_texture, float tile, dag::ConstSpan<const char *> biome_group_names,
  dag::ConstSpan<int> biome_group_indices);
void close();
void update();

int query(const Point3 &world_pos, const float radius);
GpuReadbackResultState get_query_result(int query_id, BiomeQueryResult &result);

int get_biome_group(int biome_id);
int get_biome_group_id(const char *biome_group_name);
const char *get_biome_group_name(int biome_group_id);
int get_num_biome_groups();
int get_max_num_biome_groups();
void set_details_cb(Sbuffer *buffer);
void console_query_pos(const char *argv[], int argc);
void console_query_camera_pos(const char *argv[], int argc, const TMatrix &view_itm);
} // namespace biome_query
