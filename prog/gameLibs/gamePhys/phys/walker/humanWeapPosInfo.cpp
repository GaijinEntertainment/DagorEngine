// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanWeapPosInfo.h>
#include <math/dag_mathUtils.h>

template <unsigned N>
static Point3 lerp_preset(const carray<LeanPreset, N> &presets, float val)
{
  for (int i = 1; i < presets.size(); ++i)
    if (val >= presets[i - 1].value && val <= presets[i].value)
      return lerp(presets[i - 1].nodePos, presets[i].nodePos, cvt(val, presets[i - 1].value, presets[i].value, 0.f, 1.f));
  if (val < presets[0].value)
    return presets[0].nodePos;
  return presets.back().nodePos;
}

template <typename U, typename... Args>
static Point3 lerp_preset(const U &presets, float val, float next, Args... args)
{
  for (int i = 1; i < presets.size(); ++i)
    if (val >= presets[i - 1].value && val <= presets[i].value)
      return lerp(lerp_preset(presets[i - 1].presets, next, args...), lerp_preset(presets[i].presets, next, args...),
        cvt(val, presets[i - 1].value, presets[i].value, 0.f, 1.f));
  if (val < presets[0].value)
    return lerp_preset(presets[0].presets, next, args...);
  return lerp_preset(presets.back().presets, next, args...);
}

Point3 get_weapon_pos(const PrecomputedWeaponPositions *weap_pos, PrecomputedPresetMode mode, float height, float pitch, float lean,
  float bodydir)
{
  if (weap_pos && weap_pos->isLoaded)
    return lerp_preset((mode == PrecomputedPresetMode::TPV) ? weap_pos->tpvPitchPresets : weap_pos->fpvPitchPresets, pitch, bodydir,
      height, lean);

  return ZERO<Point3>();
}
