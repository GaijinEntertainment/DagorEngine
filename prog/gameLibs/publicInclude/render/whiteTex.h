//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_samplerHandle.h>

class BaseTexture;
const UniqueTex &get_white_on_demand();
BaseTexture *get_white_tex_on_demand();
d3d::SamplerHandle get_white_sampler_on_demand();
