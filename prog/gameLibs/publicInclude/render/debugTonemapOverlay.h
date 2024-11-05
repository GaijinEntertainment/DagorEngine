//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <util/dag_string.h>
#include <math/dag_Point2.h>
#include <math/dag_color.h>
#include <shaders/dag_overrideStateId.h>

struct Color4;

class DebugTonemapOverlay
{
public:
  DebugTonemapOverlay();
  ~DebugTonemapOverlay();

  static constexpr int GRAPH_SIZE = 512;

  String processConsoleCmd(const char *argv[], int argc);
  void setTargetSize(const Point2 &target_size);
  void render();

private:
  PostFxRenderer renderer;
  bool isEnabled = false;
  Point2 targetSize;
  Color4 tonemapColor;
  float maxIntensity = 2.0f;
  shaders::UniqueOverrideStateId scissor;
};
