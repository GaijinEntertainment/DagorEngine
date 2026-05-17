// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_curveParams.h>
#include <math/dag_Point2.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>

class DataBlock;

// Artist-editable named variables persisted at the HmapLandPlugin level. These
// are declaration-only in milestone 1: storage + UI + BLK round-trip. The
// landClassEval parser does not yet see them. Float / Range / Mask / Curve are
// the four kinds; all share one TypedVar struct (the unused fields stay zero).
//
// Range is two scalars under the hood: declaring "foo" produces foo_lo and
// foo_hi, with rangeLo < rangeHi enforced. Curve packs its data in the same
// shape as daFx ScriptHelpers::TunedCubicCurveParam so the BLK round-trip is
// byte-equivalent and lets us reuse PropPanel CurveEdit verbatim.
enum class TypedVarKind : int
{
  Float = 0,
  Range,
  Mask,
  Curve,
};

const char *typed_var_kind_to_str(TypedVarKind k);
bool typed_var_kind_from_str(const char *s, TypedVarKind &out);

struct TypedVar
{
  SimpleString name;
  TypedVarKind kind = TypedVarKind::Float;

  // Float / Range schema (all in [minV, maxV] with `step` granularity).
  float minV = 0.f;
  float maxV = 1.f;
  float step = 0.01f;

  // Float live value.
  float value = 0.f;

  // Range live values; rangeLo strictly less than rangeHi.
  float rangeLo = 0.f;
  float rangeHi = 1.f;

  // Mask source (asset name resolved via HmapLandPlugin::getScriptImage).
  SimpleString maskAsset;

  // Curve, mirroring daFx TunedCubicCurveParam:
  //   curveType 0 = Cubic polynom (must have exactly 4 points)
  //   curveType 1 = Hermite spline (2..MAX_POINTS points)
  // Default initialiser matches daFx initPoints(): 4 evenly spaced points at y=1.
  int curveType = 0;
  int curvePtCnt = 4;
  Point2 curvePos[CubicCurveSampler::MAX_POINTS] = {
    Point2(0.f, 1.f), Point2(1.f / 3.f, 1.f), Point2(2.f / 3.f, 1.f), Point2(1.f, 1.f)};
};

// Returns true if `name` is a legal identifier suffix usable as a varName. Empty
// is rejected (typed vars must be referenceable from expressions, unlike unnamed
// gen layers).
bool typed_var_is_valid_name(const char *name);

// Linear scan; returns -1 on miss.
int find_typed_var(const Tab<TypedVar> &vars, const char *name);

// Per-typedVar runtime binding produced by the recompile path. Float/Mask/Curve
// use primaryVarId only; Range uses primary (= <name>_lo slot) and secondary
// (= <name>_hi slot). curveIdx is the slot in HmapLandPlugin::evalCurves where
// the CubicCurveSampler for a Curve typedVar lives; -1 for the other kinds.
// maskImageIdx is the script-image index returned by getScriptImage for a Mask
// typedVar; -1 means the asset name failed to resolve and the bake writes 0.
// 0xFFFF on either varId means registration failed (e.g. varId budget overflow)
// and the bake loop must skip the per-pixel write for that entry.
struct TypedVarRuntime
{
  uint16_t primaryVarId = 0xFFFFu;
  uint16_t secondaryVarId = 0xFFFFu;
  int curveIdx = -1;
  int maskImageIdx = -1;
  int maskImageBpp = 0;
};

// PID layout per typed-var row. Each row in the panel consumes LV_PID_PER_VAR
// contiguous PIDs starting at PID_LAND_VAR_PARAM_START + row * LV_PID_PER_VAR.
// Slots not relevant to a kind stay unused; the stride is fixed so dispatch is
// just (pid - base) / LV_PID_PER_VAR.
enum : int
{
  LV_PID_NAME = 0,     // static label: "Float foo" / "Range bar" / etc.
  LV_PID_MIN_LBL,      // static label: min   (Float / Range only)
  LV_PID_MAX_LBL,      // static label: max   (Float / Range only)
  LV_PID_STEP_LBL,     // static label: step  (Float / Range only)
  LV_PID_VALUE,        // edit-float: value   (Float only)
  LV_PID_RANGE_LO,     // edit-float: rangeLo (Range only)
  LV_PID_RANGE_HI,     // edit-float: rangeHi (Range only)
  LV_PID_ASSET_LBL,    // static label: asset (Mask only)
  LV_PID_CURVE_TYPE,   // combo: cubic polynom / hermite spline (Curve only)
  LV_PID_CURVE_EDIT,   // curve edit widget   (Curve only)
  LV_PID_DELETE,       // delete button
  LV_PID_PER_VAR = 16, // stride; oversized so future kinds can extend

  // Hard cap on row count. PID_LAST_LAND_VAR_PARAM in hmlCm.h reserves a
  // fixed window of 2048 IDs starting at PID_LAND_VAR_PARAM_START, so only
  // 2048 / LV_PID_PER_VAR = 128 rows fit before later rows fall outside the
  // dispatched range. Add / load enforce this; a static_assert in
  // hmlPlugin.cpp keeps the two headers in sync.
  LV_MAX_TYPED_VARS = 128,
};

// BLK round-trip. Block layout (sibling of "genLayers"):
//   landVars {
//     var { name:t="..."; kind:t="float|range|mask|curve"; ... }
//     ...
//   }
void load_typed_vars(Tab<TypedVar> &out, const DataBlock &blk);
void save_typed_vars(const Tab<TypedVar> &vars, DataBlock &blk);

// Panel UI helpers. fill_typed_vars_panel builds one row per var inside `grp`,
// using PIDs in [base_pid, base_pid + vars.size() * LV_PID_PER_VAR). The Add
// button is created at `base_pid - 1` by the caller (PID_LAND_VAR_ADD), and
// the row dispatch is split into onChange (live edits) and onClick (delete).
namespace PropPanel
{
class ContainerPropertyControl;
}

void fill_typed_vars_panel(PropPanel::ContainerPropertyControl &grp, int base_pid, const Tab<TypedVar> &vars);

// Apply a live UI change to vars[row] given the sub-PID within the row's slice.
// Returns true if the change was recognised; false if sub_pid is for a label /
// delete button / unrelated control. Curve writes pull control points from the
// curve edit widget; curve-type changes also re-init the widget.
bool on_typed_var_change(int sub_pid, TypedVar &var, PropPanel::ContainerPropertyControl &panel, int row_base_pid);

// Run the two-step Add modal. On OK, fills `out_var` with a fully-populated
// TypedVar (validated against `existing` for name uniqueness) and returns true.
// On Cancel or validation failure, returns false. Pops up its own message
// boxes for invalid input.
bool show_typed_var_add_dialog(TypedVar &out_var, const Tab<TypedVar> &existing);
