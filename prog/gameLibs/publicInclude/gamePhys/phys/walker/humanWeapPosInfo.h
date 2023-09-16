//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_carray.h>
#include <math/dag_Point3.h>

struct LeanPreset
{
  float value;
  Point3 nodePos;
};
struct HeightPreset
{
  float value;
  carray<LeanPreset, 5> presets;
};
struct BodydirPreset
{
  float value;
  carray<HeightPreset, 3> presets;
};
struct PitchPreset
{
  float value;
  carray<BodydirPreset, 5> presets;
};
struct PrecomputedWeaponPositions //-V730 no init for some members
{
  bool isLoaded = false;
  carray<PitchPreset, 7> tpvPitchPresets;
  carray<PitchPreset, 7> fpvPitchPresets;
};

enum class PrecomputedPresetMode
{
  TPV,
  FPV,
};
Point3 get_weapon_pos(const PrecomputedWeaponPositions *weap_pos, PrecomputedPresetMode mode, float height, float pitch, float lean,
  float bodydir);
