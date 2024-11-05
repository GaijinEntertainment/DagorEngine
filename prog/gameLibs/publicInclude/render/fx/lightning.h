//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_Point3.h>

struct LightningParams
{
  bool enabled = false;
  float aura_brightness = 1.0;
  float aura_radius = 0.1;
  float brightness = 1.0;
  float strike_min_time = 1;
  float strike_max_time = 1.5;
  float sleep_min_time = 4;
  float sleep_max_time = 2;
  float width = 0.2;
  int segment_num = 12;
  float segment_len = 0.02;
  float pos = 0.87;
  float deadzone_radius = 1.0;
  float tremble_freq = 5.0;
  Point3 color = Point3(0.7, 0.8, 1.0);

  void write(DataBlock &blk) const;

  static LightningParams read(const DataBlock &blk);

  LightningParams() = default;
};


struct Lightning
{
  Lightning();
  ~Lightning(){};

  void render();
  void setParams(LightningParams p);

private:
  float strike_start_time_ = -1;
  float strike_end_time_ = -2;
  float strike_time_ = 0;
  float sleep_time_ = 0;

  LightningParams params;
  PostFxRenderer renderer;

  int lightning_aura_brightness_VarId, lightning_width_VarId, lightning_seg_num_VarId, lightning_seg_len_VarId, lightning_pos_VarId,
    lightning_color_VarId, lightning_brightnesss_VarId, lightning_aura_radius_VarId, lightning_tremble_freq_VarId,
    lightning_deadzone_radius_VarId;
};
