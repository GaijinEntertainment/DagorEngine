//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string_view.h>
#include <generic/dag_span.h>
#include <math/dag_Point3.h>

class TMatrix4;
class HudPrimitives;

struct DebugMultiTextOverlay
{
  int borderPadding = 5;
  int maxGroupSize = 10;
  int maxMergeSteps = 5;
  float zoomDistance = 25.f;
  float zoomScreenOffset = 0.15f;

  unsigned int bkgColor = 0xcc333333;
  unsigned int textColor = 0xffffffff;
  unsigned int lineColor = 0xffffffff;
  unsigned int borderColor = 0xccffffff;
};

void draw_debug_multitext_overlay(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names, HudPrimitives *imm_prim,
  const TMatrix4 &globtm, const DebugMultiTextOverlay &cfg);
void draw_debug_multitext_overlay(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names,
  dag::ConstSpan<int> complexities, HudPrimitives *imm_prim, const TMatrix4 &globtm, const DebugMultiTextOverlay &cfg);