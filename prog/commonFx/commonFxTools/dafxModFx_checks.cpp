// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/effectClassTools.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_e3dColor.h>
#include <scriptHelpers/scriptHelpers.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <string.h>
#include <EASTL/array.h>
#include <EASTL/sort.h>
#include <EASTL/string_view.h>
#include <EASTL/algorithm.h>

static const DataBlock &sub(const DataBlock &blk, const char *name) { return *blk.getBlockByNameEx(name); }
static bool get_bool(const DataBlock &blk, const char *name, bool def) { return sub(blk, name).getBool("value", def); }
static float get_real(const DataBlock &blk, const char *name, float def) { return sub(blk, name).getReal("value", def); }
static E3DCOLOR get_color(const DataBlock &blk, const char *name, E3DCOLOR def) { return sub(blk, name).getE3dcolor("value", def); }
static const char *get_string(const DataBlock &blk, const char *name) { return sub(blk, name).getStr("value", ""); }
static bool has_value(const DataBlock &blk, const char *name, const char *value) { return strcmp(get_string(blk, name), value) == 0; }

static bool grad_has_alpha_below_255(const DataBlock &grad)
{
  for (int i = 0; i < grad.paramCount(); ++i)
    if (strncmp(grad.getParamName(i), "color_", 6) == 0 && grad.getParamType(i) == DataBlock::TYPE_E3DCOLOR &&
        grad.getE3dcolor(i).a < 255)
      return true;
  return false;
}

static void add_warning(ScriptHelpers::ParamWarnings &out, const char *path, const char *text)
{
  eastl::string &warning = out[path];
  if (!warning.empty())
    warning += "\n\n";
  warning += text;
}

struct ColorCurveDesc
{
  const char *name;
  E3DCOLOR defMask;
  bool needsLife;
};
static const ColorCurveDesc color_curves[] = {
  {"curve_over_part_life", E3DCOLOR(0, 0, 0, 255), true},
  {"curve_over_part_idx", E3DCOLOR(255, 255, 255, 255), false},
};

struct ModfxState
{
  bool premult = false;
  bool lifeOn = false;
  bool emissionOn = false;
  bool firstSelfLit = false;
  bool secondSelfLit = false;
  bool remapOn = false;
  bool secondMaskOn = false;
  bool firstIgnoresColor = false;
  bool secondIgnoresColor = false;
  bool alphaCanDrop = false;
  struct CurveState
  {
    bool changesAlpha = false;
    bool alphaOnly = false;
  };
  CurveState curveStates[countof(color_curves)] = {};
};

static const auto ignore_premult_shaders = []() {
  eastl::array arr = {
    eastl::string_view("modfx_bboard_atest"),      // atest draws without blending
    eastl::string_view("modfx_bboard_distortion"), // distortion writes offsets, not premult colors
    eastl::string_view("modfx_vol_shape")          // volshape discards the pixel when alpha is near 0
  };
  eastl::sort(arr.begin(), arr.end());
  return arr;
}();

static void read_blending(const DataBlock &params, ModfxState &s)
{
  const DataBlock &shader = sub(params, "FxRenderShader_data");
  s.premult = has_value(sub(params, "FxBlending_data"), "type", "premult_alpha");
  if (eastl::binary_search(ignore_premult_shaders.begin(), ignore_premult_shaders.end(), get_string(shader, "shader")))
    s.premult = false;
}

static void read_emission(const DataBlock &params, ModfxState &s)
{
  const DataBlock &emission = sub(params, "FxEmission_data");
  s.emissionOn = get_bool(emission, "enabled", false);
  // an applied over_part_life curve scales the strength, so it is not known here
  if (!s.emissionOn || (s.lifeOn && get_bool(sub(emission, "over_part_life"), "enabled", false)))
    return;
  const float value = get_real(emission, "value", 0.f);
  const E3DCOLOR mask = get_color(emission, "mask", E3DCOLOR(255, 255, 255, 0));
  s.firstSelfLit = value * mask.r / 255.f >= 1.f;
  s.secondSelfLit = value * mask.g / 255.f >= 1.f;
}

static void read_remap(const DataBlock &params, ModfxState &s)
{
  const DataBlock &tex = sub(params, "FxTexture_data");
  const DataBlock &remap = sub(tex, "color_remap");
  if (!get_bool(tex, "enabled", false) || !get_bool(remap, "enabled", false))
    return;
  s.remapOn = true;
  s.secondMaskOn = get_bool(remap, "second_mask_enabled", false);
  s.firstIgnoresColor = !get_bool(remap, "apply_base_color", true);
  s.secondIgnoresColor = s.secondMaskOn && !get_bool(remap, "second_mask_apply_base_color", false);
}

static void read_alpha(const DataBlock &params, ModfxState &s)
{
  const DataBlock &color = sub(params, "FxColor_data");
  if (!get_bool(color, "enabled", false))
    return;

  const E3DCOLOR colMin = get_color(color, "col_min", E3DCOLOR(128, 128, 128, 255));
  const E3DCOLOR colMax = get_color(color, "col_max", E3DCOLOR(128, 128, 128, 255));
  s.alphaCanDrop = colMin.a < 255 || colMax.a < 255;

  const DataBlock &grad = sub(color, "grad_over_part_life");
  if (s.lifeOn && get_bool(grad, "enabled", false) && grad_has_alpha_below_255(sub(grad, "grad")))
    s.alphaCanDrop = true;

  for (int i = 0; i < countof(color_curves); ++i)
  {
    const DataBlock &curve = sub(color, color_curves[i].name);
    if ((!s.lifeOn && color_curves[i].needsLife) || !get_bool(curve, "enabled", false))
      continue;
    const E3DCOLOR mask = get_color(curve, "mask", color_curves[i].defMask);
    s.curveStates[i].changesAlpha = mask.a > 0;
    s.curveStates[i].alphaOnly = mask.a > 0 && mask.r == 0 && mask.g == 0 && mask.b == 0;
    if (s.curveStates[i].changesAlpha)
      s.alphaCanDrop = true;
  }

  if (get_bool(sub(color, "alpha_by_velocity"), "enabled", false))
    s.alphaCanDrop = true;
}

static void check_remap_ignores_alpha(const ModfxState &s, ScriptHelpers::ParamWarnings &out)
{
  const bool firstShows = s.firstIgnoresColor && !s.firstSelfLit;
  const bool secondShows = s.secondIgnoresColor && !s.secondSelfLit;
  if (!s.premult || !s.alphaCanDrop || !(firstShows || secondShows))
    return;
  const char *text = "Color remap without apply_base_color (second_mask_apply_base_color for the second mask layer)\n"
                     "ignores the particle color, so with premult_alpha the remapped color does not fade with the\n"
                     "particle alpha; at alpha 0 it still shows. Alpha fades it only when the emission strength of\n"
                     "that layer (value times mask R, or G for the second layer) is 1 or more.\n"
                     "Enable that flag, or use alpha_blend or additive blending.";
  if (s.emissionOn)
    add_warning(out, "FxEmission_data/value", text);
  if (firstShows)
    add_warning(out, "FxTexture_data/color_remap/apply_base_color", text);
  if (secondShows)
    add_warning(out, "FxTexture_data/color_remap/second_mask_apply_base_color", text);
  for (int i = 0; i < countof(color_curves); ++i)
    if (s.curveStates[i].changesAlpha)
      add_warning(out, String(0, "FxColor_data/%s/enabled", color_curves[i].name).str(), text);
}

static void check_alpha_only_curves(const ModfxState &s, ScriptHelpers::ParamWarnings &out)
{
  const bool alphaHidesRemap = s.remapOn && s.firstSelfLit && (!s.secondMaskOn || s.secondSelfLit);
  if (!s.premult || alphaHidesRemap)
    return;
  for (int i = 0; i < countof(color_curves); ++i)
    if (s.curveStates[i].alphaOnly)
      add_warning(out, String(0, "FxColor_data/%s/mask", color_curves[i].name).str(),
        "With premult_alpha, a curve on alpha only changes how much the particle covers, not whether it is visible.\n"
        "Add RGB to the mask, or use alpha_blend.");
}

static void get_modfx_param_warnings(const DataBlock &params, ScriptHelpers::ParamWarnings &out)
{
  ModfxState s;
  read_blending(params, s);
  // the runtime applies the life curves and gradient only when spawn type is not fixed
  s.lifeOn = !has_value(sub(params, "FxSpawn_data"), "type", "fixed");
  read_emission(params, s);
  read_remap(params, s);
  read_alpha(params, s);

  check_remap_ignores_alpha(s, out);
  check_alpha_only_curves(s, out);
}

ScriptHelpers::ParamWarnings get_effect_param_warnings(const char *class_name, const DataBlock &params)
{
  ScriptHelpers::ParamWarnings warnings;
  if (class_name && strcmp(class_name, "DafxModFx") == 0)
    get_modfx_param_warnings(params, warnings);
  return warnings;
}
