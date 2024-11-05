//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>

class Point2;
class Point3;
class TMatrix;
class TMatrix4;
class BaseTexture;
typedef BaseTexture Texture;

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace rtao
{

void initialize(bool half_res);
void teardown();

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool performance_mode, TEXTUREID half_depth);

} // namespace rtao