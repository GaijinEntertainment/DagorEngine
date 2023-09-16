//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_overrideStateId.h>
#include <util/dag_convar.h>


class PostFxRenderer;

void update_blurred_from(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, const TextureIDPair &downsampled_frame,
  TextureIDHolder &ui_mip, const int max_ui_mip, PostFxRenderer &downsample_first_step, PostFxRenderer &downsample_4,
  PostFxRenderer &blur, TextureIDHolder &bloom_last_mip, bool bw = false);

void update_blurred_from(const TextureIDPair &src, const TextureIDPair &background, const IBBox2 *begin, const IBBox2 *end,
  const TextureIDPair &downsampled_frame, TextureIDHolder &ui_mip, const int max_ui_mip, PostFxRenderer &downsample_first_step,
  PostFxRenderer &downsample_4, PostFxRenderer &blur, TextureIDHolder &bloom_last_mip, bool bw = false);
