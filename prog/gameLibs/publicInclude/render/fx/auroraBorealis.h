//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_Point3.h>

struct AuroraBorealisParams
{
  bool enabled = false;
  float top_height = 9.0;
  Point3 top_color = Point3(1.0, 0.1, 0.3);
  float bottom_height = 5.0;
  Point3 bottom_color = Point3(0.1, 1.0, 1.0);
  float brightness = 2.2;
  float luminance = 8.0;
  float speed = 0.02;
  float ripples_scale = 20.0;
  float ripples_speed = 0.08;
  float ripples_strength = 0.8;

  AuroraBorealisParams() = default;

  bool operator!=(const AuroraBorealisParams &other) const;
  void write(DataBlock &blk) const;

  static AuroraBorealisParams read(const DataBlock &blk);
};


struct AuroraBorealis
{
  AuroraBorealis();
  ~AuroraBorealis();

  void setParams(const AuroraBorealisParams &parameters, int targetW, int targetH);

  void beforeRender();
  void render();

private:
  void setVars();

  UniqueTexHolder auroraBorealisTex;
  AuroraBorealisParams params;
  PostFxRenderer auroraBorealisFX, applyAuroraBorealis;

  int aurora_borealis_bottomVarId, aurora_borealis_topVarId, aurora_borealis_brightnessVarId, aurora_borealis_luminanceVarId,
    aurora_borealis_speedVarId, aurora_borealis_ripples_scaleVarId, aurora_borealis_ripples_speedVarId,
    aurora_borealis_ripples_strengthVarId;
};
