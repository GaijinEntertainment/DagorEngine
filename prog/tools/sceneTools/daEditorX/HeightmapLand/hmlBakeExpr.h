// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Editor-bake landClassEval helpers, split out of hmlGenColorMap.cpp so the
// per-layer / common / debug compile path and the per-pixel typed-var hoist
// no longer ride on that translation unit's compile cost.
//
// LcExprContext::init() is the only piece that stays in hmlGenColorMap.cpp:
// it registers the editor-local builtin nodes (LcExprStepNode etc.) which are
// declared in that file via HML_LC_USER_BINARY / TERNARY macros and would
// drag those into this header otherwise.

#include "hmlTypedVars.h"

#include <generic/dag_tab.h>
#include <generic/dag_span.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/bitset.h>
#include <EASTL/string.h>
#include <landClassEval/lcExprFusion.h>


// Language-static state only: parse/emit/fusion tables describe the expression
// language itself and never change after init(). Symbol-table state (varMap,
// nextVarId, mask_<name> bindings, user var/let slots) is rebuilt from scratch
// by HmapLandPlugin::recompileGenLayerExpressions on every recompile, so it
// lives as a local in the walker, not here.
struct LcExprContext
{
  enum VarSlot : uint16_t
  {
    VAR_HEIGHT = 0,
    VAR_ANGLE,
    VAR_CURVATURE,
    VAR_MASK,
    VAR_WORLD_X,
    VAR_WORLD_Y,
    VAR_COUNT
  };
  // Every gen layer reserves a slot at VAR_COUNT + layerIdx in exprVars[] for its mask;
  // the slot is used both for own-layer `mask` access and for peers reading
  // mask_<layer_name>. Slot reservation is uncapped (sized to numLayers).
  // MAX_LAYER_VARS caps the varMap *name* registration: layers at index >= MAX_LAYER_VARS
  // still have a slot for own-mask access, but no peer can reference their mask_<name> by
  // name (no varMap entry). exprVarMask is an eastl::bitset<lcexpr::MAX_VAR_ID>, so it
  // tracks every possible varId the parser may emit, including user var/let slots.
  static constexpr int MAX_LAYER_VARS = 32;
  static_assert(VAR_COUNT + MAX_LAYER_VARS <= lcexpr::MAX_VAR_ID, "named-mask range must fit inside lcexpr's varId budget");

  lcexpr::FuncParseMap parseMap;
  lcexpr::NodeEmitMap emitMap;
  lcexpr::FusionRule fusionRules[16] = {};
  int numFusionRules = 0;
  bool inited = false;

  // Defined in hmlGenColorMap.cpp because it registers the LcExpr*Node types
  // declared there. Idempotent: subsequent calls are no-ops.
  void init();
};

extern LcExprContext g_lcExpr;

// MAX_VAR_ID-wide variant of lcexpr::computeVarMask (which only tracks the first 32
// varIds in a uint32_t). Sized to the parser's hard varId cap so it scales with any
// number of layers and any reasonable amount of user var/let declarations. Both `var`
// reads and `assign` writes carry a varId and must contribute -- an expression like
// `var a = 1` only emits assign, no var read, so missing the assign tag here would
// silently undersize the bitset.
eastl::bitset<lcexpr::MAX_VAR_ID> lcexpr_compute_var_mask_bitset(const Tab<lcexpr::IRNode> &ir, int idx);

// Shared parse + tryFuse + compile + varMask core for every callsite that wants
// to compile a landClassEval expression (per-layer compileExpr, common-expr
// compile_common_into). Returns true on success with out_root holding the
// compiled bytecode root and out_var_mask populated; returns false on failure
// with out_err set to either the parser's error text (parse phase) or
// "compile failed" (compile phase). vm/nv/vt are pass-through to
// lcexpr::parseToIR; whether the caller wants them mutated (common extends
// the shared scope) or discarded (private layer scope -- caller passes a copy)
// is the caller's choice. Caller is responsible for clearing arena and the
// out parameters before calling, and for resetting / save+restore on failure.
bool compile_expression(lcexpr::Arena &arena, const char *text, uint32_t parse_flags, lcexpr::NameMap &vm, uint16_t &nv,
  lcexpr::VarTypeTable &vt, uint32_t &out_root, eastl::bitset<lcexpr::MAX_VAR_ID> &out_var_mask, eastl::string &out_err);

// Hoist constant typed-var values out of the per-pixel loop and gather only
// the Mask typedVars that some expression we will evaluate actually reads.
//
// On entry expr_vars_0 must be zero across the typed-var range. Float / Range /
// Curve typedVars get written once into expr_vars_0 (caller fans out to other
// per-thread slices). Mask typedVars are not written here -- they vary per
// pixel; out_used_mask_tvs collects the indices the per-pixel loop should
// sample. The filter is "primaryVarId is referenced by aggregate_var_mask AND
// the mask asset resolved to a script-image". Primary/secondary varIds also
// extend out_typed_vars_end_nv so the caller's per-pixel zero of common's
// user-var range starts past these slots and never clobbers the constants.
//
// Caller initialises out_typed_vars_end_nv to the post-layer-mask boundary
// (VAR_COUNT + lcNumLayerVars).
void prepare_typed_vars(dag::ConstSpan<TypedVar> typed_vars, dag::ConstSpan<TypedVarRuntime> typed_var_runtime,
  const eastl::bitset<lcexpr::MAX_VAR_ID> &aggregate_var_mask, float *expr_vars_0, uint16_t &out_typed_vars_end_nv,
  dag::RelocatableFixedVector<int, 8> &out_used_mask_tvs);
