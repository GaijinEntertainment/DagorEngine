// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Out-of-line definitions for editor-bake expression helpers split out of
// hmlGenColorMap.cpp. LcExprContext::init() and the LcExpr*Node types it
// registers stay there because they reference the editor-local builtin macros
// (HML_LC_USER_BINARY / TERNARY) and would re-couple this TU to that file's
// transitive header set.

#include "hmlBakeExpr.h"


LcExprContext g_lcExpr;

eastl::bitset<lcexpr::MAX_VAR_ID> lcexpr_compute_var_mask_bitset(const Tab<lcexpr::IRNode> &ir, int idx)
{
  eastl::bitset<lcexpr::MAX_VAR_ID> m;
  if (idx < 0)
    return m;
  const lcexpr::IRNode &n = ir[idx];
  const bool touchesVar = n.tag == lcexpr::hash_name("var") || n.tag == lcexpr::hash_name("assign");
  if (touchesVar && n.varId < lcexpr::MAX_VAR_ID)
    m.set(n.varId);
  for (int i = 0; i < n.nc; i++)
    m |= lcexpr_compute_var_mask_bitset(ir, n.c[i]);
  return m;
}

bool compile_expression(lcexpr::Arena &arena, const char *text, uint32_t parse_flags, lcexpr::NameMap &vm, uint16_t &nv,
  lcexpr::VarTypeTable &vt, uint32_t &out_root, eastl::bitset<lcexpr::MAX_VAR_ID> &out_var_mask, eastl::string &out_err)
{
  Tab<lcexpr::IRNode> ir;
  int irRoot = lcexpr::parseToIR(text, ir, g_lcExpr.parseMap, g_lcExpr.emitMap, vm, nv, out_err, lcexpr::MAX_TEMP_REGS, nullptr,
    parse_flags, &vt);
  if (irRoot < 0)
    return false;
  out_var_mask = lcexpr_compute_var_mask_bitset(ir, irRoot);
  uint32_t fused = lcexpr::tryFuse(ir, irRoot, arena, g_lcExpr.fusionRules, g_lcExpr.numFusionRules);
  uint32_t root = (fused != lcexpr::PARSE_ERROR) ? fused : lcexpr::compile(ir, irRoot, arena, g_lcExpr.emitMap);
  if (root == lcexpr::PARSE_ERROR)
  {
    out_err = "compile failed";
    out_var_mask.reset();
    return false;
  }
  out_root = root;
  return true;
}

void prepare_typed_vars(dag::ConstSpan<TypedVar> typed_vars, dag::ConstSpan<TypedVarRuntime> typed_var_runtime,
  const eastl::bitset<lcexpr::MAX_VAR_ID> &aggregate_var_mask, float *expr_vars_0, uint16_t &out_typed_vars_end_nv,
  dag::RelocatableFixedVector<int, 8> &out_used_mask_tvs)
{
  for (int i = 0; i < typed_vars.size(); i++)
  {
    const TypedVar &tv = typed_vars[i];
    const TypedVarRuntime &tr = typed_var_runtime[i];
    if (tr.primaryVarId == 0xFFFFu)
      continue;
    const uint16_t pEnd = uint16_t(tr.primaryVarId + 1);
    if (pEnd > out_typed_vars_end_nv)
      out_typed_vars_end_nv = pEnd;
    if (tv.kind == TypedVarKind::Range && tr.secondaryVarId != 0xFFFFu)
    {
      const uint16_t sEnd = uint16_t(tr.secondaryVarId + 1);
      if (sEnd > out_typed_vars_end_nv)
        out_typed_vars_end_nv = sEnd;
    }
    if (tv.kind == TypedVarKind::Mask)
    {
      if (aggregate_var_mask.test(tr.primaryVarId) && tr.maskImageIdx >= 0)
        out_used_mask_tvs.push_back(i);
      continue;
    }
    switch (tv.kind)
    {
      case TypedVarKind::Float: expr_vars_0[tr.primaryVarId] = tv.value; break;
      case TypedVarKind::Range:
        expr_vars_0[tr.primaryVarId] = tv.rangeLo;
        if (tr.secondaryVarId != 0xFFFFu)
          expr_vars_0[tr.secondaryVarId] = tv.rangeHi;
        break;
      case TypedVarKind::Curve: expr_vars_0[tr.primaryVarId] = (float)tr.curveIdx; break;
      default: break;
    }
  }
}
