//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// constraint: must equal the cor channel in dynamic_simple_inc.dshl and the BVH skinned prepass.
// The value is (center of rotation - vertex position) in bind space as color8: three signed bytes times
// 2^((w - 200) / 8). Zero keeps the linear blend skinning position.
inline constexpr int COR_SKINNING_EXTRA_CHANNEL = 60;
