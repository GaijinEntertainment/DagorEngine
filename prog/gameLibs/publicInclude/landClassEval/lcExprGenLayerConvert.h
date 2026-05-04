//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <math/dag_check_nan.h>
#include <stdio.h>

// Convert old data-driven genLayers (wtMode, mask_conv, ht_conv, etc.)
// to expression strings.
namespace lcexpr
{

// Mirrors PostScriptParamLandLayer::WtRange conv modes
enum GenLayerConv
{
  WMT_ONE = 0,
  WMT_AS_IS = 1,
  WMT_SMOOTH_STEP = 2,
  WMT_ZERO = 3,
};

struct GenLayerParams
{
  int maskConv;                  // 0=one 1=asIs 2=smoothStep 3=zero
  int htConv, angConv, curvConv; // same
  float htV0, htDv;              // height ramp: v0, v0+dv
  float angV0, angDv;            // angle ramp: v0, v0+dv
  float curvV0, curvDv;          // curvature ramp: v0, v0+dv
  int wtMode;                    // 0=multiply 1=sum 2=max(mask,terrain)
};

// Format a float without trailing zeros. Non-finite inputs (NaN/inf) are coerced to "0"
// because the expression parser does not accept "nan" / "inf" literals, and emitting them
// would turn the whole layer into a silent parse failure. Corrupt input data surfaces as
// an obviously-wrong "0" term instead.
//
// Locale handling: snprintf("%g") honours LC_NUMERIC, so on a comma-decimal host process
// (German / French / Russian Windows) it emits strings like "3,14". The expression parser
// is locale-independent and accepts only '.' as the decimal separator, so round-tripping
// would fail. Rewriting ',' -> '.' in place normalises the output; "%g" cannot emit any
// other ','-bearing tokens (there are no thousands separators in the format).
inline void fmtFloat(char *buf, int bufsz, float v)
{
  snprintf(buf, bufsz, "%g", check_finite(v) ? v : 0.f);
  for (char *p = buf; *p; p++)
    if (*p == ',')
      *p = '.';
}

// Generate a single ramp/smooth_ramp term for a component.
// Returns empty string if the component is WMT_ONE (always 1) or WMT_ZERO (always 0).
// Legacy calcWeight treats fabsf(dv) < 1e-4 as a step function (equality test at v0),
// so we emit select(abs(var - v0) < 0.0001, 1, 0) to preserve that behavior.
inline eastl::string genTerm(const char *varName, int conv, float v0, float dv)
{
  if (conv == WMT_ONE)
    return "";
  if (conv == WMT_ZERO)
    return "0";
  if (fabsf(dv) < 1e-4f)
  {
    char sv0[32];
    fmtFloat(sv0, sizeof(sv0), v0);
    eastl::string s;
    s.sprintf("select(abs(%s - %s) < 0.0001, 1, 0)", varName, sv0);
    return s;
  }
  char sv0[32], sv1[32];
  fmtFloat(sv0, sizeof(sv0), v0);
  fmtFloat(sv1, sizeof(sv1), v0 + dv);
  const char *fn = (conv == WMT_SMOOTH_STEP) ? "smooth_ramp" : "ramp";
  eastl::string s;
  s.sprintf("%s(%s, %s, %s)", fn, varName, sv0, sv1);
  return s;
}

// Convert a genLayer to an expression string.
// Returns the expression (e.g., "max(mask, ramp(height,16,26) * smooth_ramp(angle,2,22))").
inline eastl::string genLayerToExpr(const GenLayerParams &p)
{
  // Build mask term
  eastl::string maskTerm;
  if (p.maskConv == WMT_ZERO)
    maskTerm = "0";
  else if (p.maskConv == WMT_ONE)
    maskTerm = ""; // no mask
  else if (p.maskConv == WMT_SMOOTH_STEP)
    maskTerm = "smoothstep(mask)";
  else
    maskTerm = "mask";

  // Build terrain terms (height, angle, curvature)
  eastl::string htTerm = genTerm("height", p.htConv, p.htV0, p.htDv);
  eastl::string angTerm = genTerm("angle", p.angConv, p.angV0, p.angDv);
  eastl::string curvTerm = genTerm("curvature", p.curvConv, p.curvV0, p.curvDv);

  // Collect non-empty terrain terms
  const eastl::string *terrainParts[3];
  int nTerrain = 0;
  if (!htTerm.empty() && htTerm != "0")
    terrainParts[nTerrain++] = &htTerm;
  if (!angTerm.empty() && angTerm != "0")
    terrainParts[nTerrain++] = &angTerm;
  if (!curvTerm.empty() && curvTerm != "0")
    terrainParts[nTerrain++] = &curvTerm;

  // Check if any terrain term is zero (makes whole product zero in multiply mode)
  bool hasZeroTerrain = (htTerm == "0" || angTerm == "0" || curvTerm == "0");

  // wtMode: 0=multiply, 1=sum, 2=max(mask, terrain_product)
  if (p.wtMode == 2) // max(mask, terrain_product)
  {
    eastl::string terrain;
    if (hasZeroTerrain)
      terrain = "0";
    else if (nTerrain == 0)
      terrain = "1"; // all terrain components are WMT_ONE, product = 1
    else
    {
      terrain = *terrainParts[0];
      for (int i = 1; i < nTerrain; i++)
        terrain.append_sprintf(" * %s", terrainParts[i]->c_str());
    }
    if (maskTerm.empty()) // WMT_ONE: mask is always 1
      maskTerm = "1";
    return eastl::string().sprintf("max(%s, %s)", maskTerm.c_str(), terrain.c_str());
  }

  if (p.wtMode == 1) // sum: mask + ht + ang + curv
  {
    eastl::string result;
    auto addTerm = [&](int conv, const eastl::string &t) {
      if (conv == WMT_ZERO)
        return;
      if (!result.empty())
        result += " + ";
      result += (conv == WMT_ONE) ? "1" : t;
    };
    addTerm(p.maskConv, maskTerm);
    addTerm(p.htConv, htTerm);
    addTerm(p.angConv, angTerm);
    addTerm(p.curvConv, curvTerm);
    return result.empty() ? eastl::string("0") : result;
  }

  // wtMode 0: multiply: mask * ht * ang * curv
  // Collect all non-empty, non-"1" terms
  eastl::string parts[4];
  int nParts = 0;
  if (!maskTerm.empty())
    parts[nParts++] = maskTerm;
  for (int i = 0; i < nTerrain; i++)
    parts[nParts++] = *terrainParts[i];

  // If any term is "0", whole product is 0
  if (hasZeroTerrain || maskTerm == "0")
    return "0";

  if (nParts == 0)
    return "1";
  eastl::string result = parts[0];
  for (int i = 1; i < nParts; i++)
    result.append_sprintf(" * %s", parts[i].c_str());
  return result;
}

} // namespace lcexpr
