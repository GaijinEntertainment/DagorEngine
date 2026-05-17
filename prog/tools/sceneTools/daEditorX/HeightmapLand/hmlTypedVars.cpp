// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlTypedVars.h"
#include "hmlPlugin.h"

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <math/dag_mathBase.h>
#include <generic/dag_tab.h>

#include <propPanel/c_common.h>
#include <propPanel/control/container.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <oldEditor/de_interface.h>
#include <de3_interface.h>
#include <EditorCore/ec_interface.h>

#include <debug/dag_debug.h>

#include <string.h>

const char *typed_var_kind_to_str(TypedVarKind k)
{
  switch (k)
  {
    case TypedVarKind::Float: return "float";
    case TypedVarKind::Range: return "range";
    case TypedVarKind::Mask: return "mask";
    case TypedVarKind::Curve: return "curve";
  }
  return "float";
}

bool typed_var_kind_from_str(const char *s, TypedVarKind &out)
{
  if (!s)
    return false;
  if (strcmp(s, "float") == 0)
  {
    out = TypedVarKind::Float;
    return true;
  }
  if (strcmp(s, "range") == 0)
  {
    out = TypedVarKind::Range;
    return true;
  }
  if (strcmp(s, "mask") == 0)
  {
    out = TypedVarKind::Mask;
    return true;
  }
  if (strcmp(s, "curve") == 0)
  {
    out = TypedVarKind::Curve;
    return true;
  }
  return false;
}

bool typed_var_is_valid_name(const char *name)
{
  // Must match landClassEval's lexer in lcExprParserImpl.h: first char is a
  // letter or underscore, the rest are letters / digits / underscore, total
  // length capped at 63 (the lexer's tokId buffer). Reserved keywords listed
  // in is_keyword() (var/let/if/then/else) are also rejected -- the parser
  // tokenises them as T_ID but parsePrimary handles "if" specially and
  // parseStmt reserves "var"/"let", so a typedVar with one of those names
  // could never be referenced from an expression.
  if (!name || !*name)
    return false;
  const char first = name[0];
  const bool firstOk = (first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || first == '_';
  if (!firstOk)
    return false;
  int len = 0;
  for (const char *p = name; *p; p++, len++)
  {
    if (len >= 63)
      return false;
    const char c = *p;
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok)
      return false;
  }
  if (strcmp(name, "var") == 0 || strcmp(name, "let") == 0 || strcmp(name, "if") == 0 || strcmp(name, "then") == 0 ||
      strcmp(name, "else") == 0)
    return false;
  return true;
}

int find_typed_var(const Tab<TypedVar> &vars, const char *name)
{
  if (!name || !*name)
    return -1;
  for (int i = 0; i < vars.size(); i++)
    if (strcmp(vars[i].name.str(), name) == 0)
      return i;
  return -1;
}


static void clamp_curve_pt(Point2 &p)
{
  p.x = clamp<float>(p.x, 0.f, 1.f);
  p.y = clamp<float>(p.y, 0.f, 1.f);
}

static void load_curve_fields(TypedVar &v, const DataBlock &b)
{
  // Mirrors ScriptHelpers::TunedCubicCurveParam::loadValues: read curveType then
  // posN until the first missing index. Cubic-polynom (type 0) requires exactly
  // 4 points; on mismatch, fall back to the daFx default 4-point shape.
  v.curveType = b.getInt("curveType", 0);
  v.curvePtCnt = 0;
  for (int i = 0; i < CubicCurveSampler::MAX_POINTS; i++)
  {
    String key(32, "pos%d", i);
    if (!b.paramExists(key))
      break;
    v.curvePos[i] = b.getPoint2(key, v.curvePos[i]);
    clamp_curve_pt(v.curvePos[i]);
    v.curvePtCnt++;
  }
  if (v.curveType == 0 && v.curvePtCnt != 4)
  {
    v.curvePtCnt = 4;
    for (int i = 0; i < 4; i++)
      v.curvePos[i] = Point2(i / 3.f, 1.f);
  }
}

static void save_curve_fields(const TypedVar &v, DataBlock &b)
{
  if (v.curveType)
    b.setInt("curveType", v.curveType);
  for (int i = 0; i < v.curvePtCnt; i++)
    b.setPoint2(String(32, "pos%d", i), v.curvePos[i]);
}

void load_typed_vars(Tab<TypedVar> &out, const DataBlock &blk)
{
  out.clear();
  const int blockCount = blk.blockCount();
  out.reserve(blockCount);
  for (int i = 0; i < blockCount; i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    if (strcmp(b.getBlockName(), "var") != 0)
      continue;

    TypedVar v;
    v.name = b.getStr("name", "");
    if (!typed_var_is_valid_name(v.name.str()))
      continue;

    const char *kindStr = b.getStr("kind", "float");
    if (!typed_var_kind_from_str(kindStr, v.kind))
      v.kind = TypedVarKind::Float;

    v.minV = b.getReal("min", v.minV);
    v.maxV = b.getReal("max", v.maxV);
    v.step = b.getReal("step", v.step);
    if (v.maxV <= v.minV)
      v.maxV = v.minV + 1.f;
    if (v.step <= 0.f)
      v.step = 0.01f;

    switch (v.kind)
    {
      case TypedVarKind::Float: v.value = clamp<float>(b.getReal("value", v.minV), v.minV, v.maxV); break;

      case TypedVarKind::Range:
      {
        v.rangeLo = clamp<float>(b.getReal("rangeLo", v.minV), v.minV, v.maxV);
        v.rangeHi = clamp<float>(b.getReal("rangeHi", v.maxV), v.minV, v.maxV);
        if (v.rangeHi <= v.rangeLo)
        {
          // Bumping rangeHi up by step is the natural fix, but if rangeLo got
          // clamped to maxV the bump saturates and we end up with lo == hi.
          // Fall back to pulling rangeLo down so the strict invariant survives.
          const float bump = max(v.step, 1e-6f);
          if (v.rangeLo + bump <= v.maxV)
            v.rangeHi = v.rangeLo + bump;
          else
          {
            v.rangeHi = v.maxV;
            v.rangeLo = max(v.minV, v.maxV - bump);
          }
        }
        break;
      }

      case TypedVarKind::Mask: v.maskAsset = b.getStr("asset", ""); break;

      case TypedVarKind::Curve: load_curve_fields(v, b); break;
    }

    if (find_typed_var(out, v.name.str()) >= 0)
      continue; // skip duplicates silently; UI add-path enforces uniqueness

    // Hand-edited BLK files can declare typed vars whose parser-visible names
    // collide with the runtime builtins (height/angle/curvature/mask/world_x/
    // world_y) or with mask_<layer> bindings. register_typed_vars would then
    // get the existing builtin's varId back from register_var, and the bake's
    // per-pixel typed-var write would silently overwrite the builtin slot
    // (e.g. exprVarsPtr[VAR_HEIGHT] gets the artist's UI value instead of the
    // terrain height). Mirror the Add dialog: reject the entry up front.
    //
    // Note: this load runs after loadGenLayers, so genLayers / commonExprText /
    // every layer's exprText are already populated -- isReservedVarName and
    // findVarLetDeclSite both have the state they need.
    HmapLandPlugin *plugin = HmapLandPlugin::self;
    auto probeNames = [&v](Tab<String> &out) {
      if (v.kind == TypedVarKind::Range)
      {
        out.push_back(String(0, "%s_lo", v.name.str()));
        out.push_back(String(0, "%s_hi", v.name.str()));
      }
      else
        out.push_back(String(v.name.str()));
    };
    bool drop = false;
    if (plugin)
    {
      Tab<String> probes(tmpmem);
      probeNames(probes);
      for (int p = 0; p < probes.size(); p++)
      {
        if (plugin->isReservedVarName(probes[p].str()))
        {
          logerr("landVars: dropping '%s' (kind=%s); slot '%s' collides with a builtin or mask_<layer>", v.name.str(), kindStr,
            probes[p].str());
          drop = true;
          break;
        }
        // Same up-front var/let scan the Add dialog uses: a typedVar whose
        // slot is also declared as `var <name>` / `let <name>` in any layer
        // expression or in the common expression would make the next
        // recompileGenLayerExpressions reject the redeclaration, and the
        // entire project's expressions go silent until the artist repairs
        // the BLK by hand. Drop the entry on load with a clear logerr that
        // names the offending site instead.
        SimpleString site;
        if (plugin->findVarLetDeclSite(probes[p].str(), site))
        {
          logerr("landVars: dropping '%s' (kind=%s); slot '%s' is already declared as 'var' or 'let' in %s", v.name.str(), kindStr,
            probes[p].str(), site.str());
          drop = true;
          break;
        }
      }
    }
    if (drop)
      continue;

    if (out.size() >= LV_MAX_TYPED_VARS)
    {
      logwarn("landVars: dropping '%s' and any subsequent entries; per-panel cap is %d", v.name.str(), (int)LV_MAX_TYPED_VARS);
      break;
    }

    out.push_back(v);
  }
}

void save_typed_vars(const Tab<TypedVar> &vars, DataBlock &blk)
{
  for (int i = 0; i < vars.size(); i++)
  {
    const TypedVar &v = vars[i];
    DataBlock &b = *blk.addNewBlock("var");
    b.setStr("name", v.name);
    b.setStr("kind", typed_var_kind_to_str(v.kind));
    b.setReal("min", v.minV);
    b.setReal("max", v.maxV);
    b.setReal("step", v.step);

    switch (v.kind)
    {
      case TypedVarKind::Float: b.setReal("value", v.value); break;

      case TypedVarKind::Range:
        b.setReal("rangeLo", v.rangeLo);
        b.setReal("rangeHi", v.rangeHi);
        break;

      case TypedVarKind::Mask:
        if (!v.maskAsset.empty())
          b.setStr("asset", v.maskAsset);
        break;

      case TypedVarKind::Curve: save_curve_fields(v, b); break;
    }
  }
}

// ---------------------------------------------------------------------------
// Panel UI helpers
// ---------------------------------------------------------------------------

using hdpi::_pxScaled;

static const char *kind_label(TypedVarKind k)
{
  switch (k)
  {
    case TypedVarKind::Float: return "Float";
    case TypedVarKind::Range: return "Range";
    case TypedVarKind::Mask: return "Mask";
    case TypedVarKind::Curve: return "Curve";
  }
  return "?";
}

// Match the daFx scriptCurves curve-edit init: approximator + min/max/lockEnds +
// fixed [0,1]^2 axes + push the current control points. Reused both at panel
// fill time and after a curveType change so the widget stays consistent.
static void init_curve_widget(PropPanel::ContainerPropertyControl &row, int curve_pid, const TypedVar &v)
{
  if (v.curveType == 0)
    row.setInt(curve_pid, PropPanel::CURVE_CUBICPOLYNOM_APP);
  else
    row.setInt(curve_pid, PropPanel::CURVE_CUBIC_P_SPLINE_APP);

  row.setBool(curve_pid, true); // lock ends, matching daFx convention

  if (v.curveType == 0)
    row.setMinMaxStep(curve_pid, 4, 4, PropPanel::CURVE_MIN_MAX_POINTS);
  else
    row.setMinMaxStep(curve_pid, 2, CubicCurveSampler::MAX_POINTS, PropPanel::CURVE_MIN_MAX_POINTS);

  row.setMinMaxStep(curve_pid, 0, 1, PropPanel::CURVE_MIN_MAX_X);
  row.setMinMaxStep(curve_pid, 0, 1, PropPanel::CURVE_MIN_MAX_Y);

  Tab<Point2> pts(tmpmem);
  for (int i = 0; i < v.curvePtCnt; i++)
    pts.push_back(v.curvePos[i]);
  row.setControlPoints(curve_pid, pts);
}

void fill_typed_vars_panel(PropPanel::ContainerPropertyControl &grp, int base_pid, const Tab<TypedVar> &vars)
{
  // Defensive cap: load_typed_vars and the Add dialog already enforce this, but
  // keep the panel from emitting controls past the dispatched PID window if
  // some other call site grew the list directly.
  const int n = min<int>(vars.size(), LV_MAX_TYPED_VARS);
  for (int i = 0; i < n; i++)
  {
    const TypedVar &v = vars[i];
    const int rowBase = base_pid + i * LV_PID_PER_VAR;

    // Header shows the parser-visible name(s) so artists know exactly what to
    // type in expressions. Range expands to <name>_lo / <name>_hi (two lcexpr
    // slots), Curve is read via curve(<name>, x). Float and Mask are referenced
    // by the bare name. Same string is set as a tooltip on the editable
    // sub-controls below for hover discoverability.
    String parserHint;
    switch (v.kind)
    {
      case TypedVarKind::Float: parserHint.printf(0, "in expressions: %s", v.name.str()); break;
      case TypedVarKind::Range: parserHint.printf(0, "in expressions: %s_lo, %s_hi", v.name.str(), v.name.str()); break;
      case TypedVarKind::Mask: parserHint.printf(0, "in expressions: %s (mask sample)", v.name.str()); break;
      case TypedVarKind::Curve: parserHint.printf(0, "in expressions: curve(%s, x)", v.name.str()); break;
    }

    String hdr(96, "%s: %s  [%s]", kind_label(v.kind), v.name.str(), parserHint.str());
    PropPanel::ContainerPropertyControl *row = grp.createGroup(rowBase + LV_PID_NAME, hdr.str());
    if (!row)
      continue;

    switch (v.kind)
    {
      case TypedVarKind::Float:
        row->createStatic(rowBase + LV_PID_MIN_LBL, String(64, "min: %g  max: %g  step: %g", v.minV, v.maxV, v.step));
        row->createEditFloat(rowBase + LV_PID_VALUE, "value", v.value);
        row->setMinMaxStep(rowBase + LV_PID_VALUE, v.minV, v.maxV, v.step);
        row->setTooltipId(rowBase + LV_PID_VALUE, parserHint.str());
        break;

      case TypedVarKind::Range:
        row->createStatic(rowBase + LV_PID_MIN_LBL, String(64, "min: %g  max: %g  step: %g", v.minV, v.maxV, v.step));
        row->createEditFloat(rowBase + LV_PID_RANGE_LO, "lo", v.rangeLo);
        row->setMinMaxStep(rowBase + LV_PID_RANGE_LO, v.minV, v.maxV, v.step);
        row->setTooltipId(rowBase + LV_PID_RANGE_LO, parserHint.str());
        row->createEditFloat(rowBase + LV_PID_RANGE_HI, "hi", v.rangeHi);
        row->setMinMaxStep(rowBase + LV_PID_RANGE_HI, v.minV, v.maxV, v.step);
        row->setTooltipId(rowBase + LV_PID_RANGE_HI, parserHint.str());
        break;

      case TypedVarKind::Mask:
        row->createStatic(rowBase + LV_PID_ASSET_LBL, String(0, "asset: %s", v.maskAsset.empty() ? "<none>" : v.maskAsset.str()));
        row->setTooltipId(rowBase + LV_PID_ASSET_LBL, parserHint.str());
        break;

      case TypedVarKind::Curve:
      {
        Tab<String> ct(tmpmem);
        ct.push_back() = "Cubic polynom";
        ct.push_back() = "Hermite spline";
        row->createCombo(rowBase + LV_PID_CURVE_TYPE, "type", ct, v.curveType);
        row->setTooltipId(rowBase + LV_PID_CURVE_TYPE, parserHint.str());
        row->createCurveEdit(rowBase + LV_PID_CURVE_EDIT, "");
        init_curve_widget(*row, rowBase + LV_PID_CURVE_EDIT, v);
        row->setTooltipId(rowBase + LV_PID_CURVE_EDIT, parserHint.str());
        break;
      }
    }

    row->createButton(rowBase + LV_PID_DELETE, "Delete");
  }
}

bool on_typed_var_change(int sub_pid, TypedVar &var, PropPanel::ContainerPropertyControl &panel, int row_base_pid)
{
  switch (var.kind)
  {
    case TypedVarKind::Float:
      if (sub_pid == LV_PID_VALUE)
      {
        var.value = clamp<float>(panel.getFloat(row_base_pid + LV_PID_VALUE), var.minV, var.maxV);
        return true;
      }
      break;

    case TypedVarKind::Range:
      if (sub_pid == LV_PID_RANGE_LO || sub_pid == LV_PID_RANGE_HI)
      {
        var.rangeLo = clamp<float>(panel.getFloat(row_base_pid + LV_PID_RANGE_LO), var.minV, var.maxV);
        var.rangeHi = clamp<float>(panel.getFloat(row_base_pid + LV_PID_RANGE_HI), var.minV, var.maxV);
        // Side-aware repair of the strict lo < hi invariant: push the *other*
        // side away from the just-edited one by `step`, so the artist's edit
        // wins and the partner moves to follow. If the partner saturates at
        // its bound, fall back to nudging the edited side off the bound to
        // keep the invariant.
        const float bump = max(var.step, 1e-6f);
        if (sub_pid == LV_PID_RANGE_LO && var.rangeLo >= var.rangeHi)
        {
          if (var.rangeLo + bump <= var.maxV)
            var.rangeHi = var.rangeLo + bump;
          else
          {
            var.rangeHi = var.maxV;
            var.rangeLo = max(var.minV, var.maxV - bump);
          }
        }
        else if (sub_pid == LV_PID_RANGE_HI && var.rangeHi <= var.rangeLo)
        {
          if (var.rangeHi - bump >= var.minV)
            var.rangeLo = var.rangeHi - bump;
          else
          {
            var.rangeLo = var.minV;
            var.rangeHi = min(var.maxV, var.minV + bump);
          }
        }
        // Reflect any auto-correction back into the panel so the field shown
        // matches the value we kept; otherwise the artist sees the stale text
        // they typed even though the model has moved.
        panel.setFloat(row_base_pid + LV_PID_RANGE_LO, var.rangeLo);
        panel.setFloat(row_base_pid + LV_PID_RANGE_HI, var.rangeHi);
        return true;
      }
      break;

    case TypedVarKind::Mask:
      // No live-editable fields.
      break;

    case TypedVarKind::Curve:
      if (sub_pid == LV_PID_CURVE_TYPE)
      {
        const int newType = panel.getInt(row_base_pid + LV_PID_CURVE_TYPE);
        if (newType != var.curveType)
        {
          var.curveType = newType;
          // Cubic polynom requires exactly 4 points; reset to defaults on switch in
          // either direction to avoid mid-state with a wrong point count.
          if (var.curveType == 0)
          {
            var.curvePtCnt = 4;
            for (int i = 0; i < 4; i++)
              var.curvePos[i] = Point2(i / 3.f, 1.f);
          }
          init_curve_widget(panel, row_base_pid + LV_PID_CURVE_EDIT, var);
        }
        return true;
      }
      if (sub_pid == LV_PID_CURVE_EDIT)
      {
        Tab<Point2> pts(tmpmem);
        panel.getCurveCoefs(row_base_pid + LV_PID_CURVE_EDIT, pts);
        const int n = min<int>(pts.size(), CubicCurveSampler::MAX_POINTS);
        var.curvePtCnt = n;
        for (int i = 0; i < n; i++)
        {
          var.curvePos[i] = pts[i];
          clamp_curve_pt(var.curvePos[i]);
        }
        return true;
      }
      break;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Add-variable two-step modal
// ---------------------------------------------------------------------------

namespace
{
struct DlgGuard
{
  PropPanel::DialogWindow *d;
  DlgGuard(PropPanel::DialogWindow *dlg) : d(dlg) {}
  ~DlgGuard()
  {
    if (d)
      DAGORED2->deleteDialog(d);
  }
};
} // namespace

static bool ask_step2_float_or_range(TypedVar &v)
{
  const bool isRange = (v.kind == TypedVarKind::Range);
  const char *title = isRange ? "Range variable parameters" : "Float variable parameters";

  PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(300), _pxScaled(isRange ? 260 : 220), title);
  DlgGuard guard(dlg);
  dlg->setManualModalSizingEnabled();
  PropPanel::ContainerPropertyControl *p = dlg->getPanel();

  p->createEditFloat(0, "Min", 0.f);
  p->createEditFloat(1, "Max", 1.f);
  p->createEditFloat(2, "Step", 0.01f);
  if (isRange)
  {
    p->createEditFloat(3, "Initial lo", 0.25f);
    p->createEditFloat(4, "Initial hi", 0.75f);
  }
  else
  {
    p->createEditFloat(3, "Initial value", 0.f);
  }

  if (dlg->showDialog() != PropPanel::DIALOG_ID_OK)
    return false;

  v.minV = p->getFloat(0);
  v.maxV = p->getFloat(1);
  v.step = p->getFloat(2);
  if (v.maxV <= v.minV)
  {
    wingw::message_box(wingw::MBS_EXCL, "Invalid range", "Max must be greater than Min.");
    return false;
  }
  if (v.step <= 0.f)
  {
    wingw::message_box(wingw::MBS_EXCL, "Invalid step", "Step must be greater than zero.");
    return false;
  }

  if (isRange)
  {
    v.rangeLo = clamp<float>(p->getFloat(3), v.minV, v.maxV);
    v.rangeHi = clamp<float>(p->getFloat(4), v.minV, v.maxV);
    if (v.rangeHi <= v.rangeLo)
    {
      wingw::message_box(wingw::MBS_EXCL, "Invalid range", "Initial hi must be strictly greater than Initial lo.");
      return false;
    }
  }
  else
  {
    v.value = clamp<float>(p->getFloat(3), v.minV, v.maxV);
  }
  return true;
}

static bool ask_step2_mask(TypedVar &v)
{
  // Use the dagor asset selector for "tex" type so the artist picks from the
  // existing texture/mask asset list (matching the layer-mask convention) rather
  // than typing a raw filesystem path. The returned name is what
  // HmapLandPlugin::getScriptImage(name, 1, -1) expects: a basename without
  // extension, resolvable via the bitmask manager.
  const char *picked = DAEDITOR3.selectAssetX(/*initial*/ "", "Select mask asset for mask variable", "tex");
  if (!picked || !*picked)
    return false;
  v.maskAsset = picked;
  return true;
}

static bool ask_step2_curve(TypedVar &v)
{
  PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(280), _pxScaled(140), "Curve variable parameters");
  DlgGuard guard(dlg);
  dlg->setManualModalSizingEnabled();
  PropPanel::ContainerPropertyControl *p = dlg->getPanel();

  Tab<String> ct(tmpmem);
  ct.push_back() = "Cubic polynom";
  ct.push_back() = "Hermite spline";
  p->createCombo(0, "Curve type", ct, 0);

  if (dlg->showDialog() != PropPanel::DIALOG_ID_OK)
    return false;

  v.curveType = p->getInt(0);
  // Curve points stay at constructor defaults (4 evenly spaced at y=1).
  return true;
}

bool show_typed_var_add_dialog(TypedVar &out_var, const Tab<TypedVar> &existing)
{
  if (existing.size() >= LV_MAX_TYPED_VARS)
  {
    wingw::message_box(wingw::MBS_EXCL, "Too many typed variables",
      "The land-variables panel is capped at %d entries (PID window in hmlCm.h). "
      "Delete an existing entry before adding a new one.",
      (int)LV_MAX_TYPED_VARS);
    return false;
  }

  PropPanel::DialogWindow *dlg1 = DAGORED2->createDialog(_pxScaled(300), _pxScaled(150), "Create typed variable");
  DlgGuard guard(dlg1);
  dlg1->setManualModalSizingEnabled();
  PropPanel::ContainerPropertyControl *p1 = dlg1->getPanel();

  p1->createEditBox(0, "Name");
  Tab<String> kinds(tmpmem);
  kinds.push_back() = "Float";
  kinds.push_back() = "Range";
  kinds.push_back() = "Mask";
  kinds.push_back() = "Curve";
  p1->createCombo(1, "Kind", kinds, 0);
  p1->setFocusById(0);

  if (dlg1->showDialog() != PropPanel::DIALOG_ID_OK)
    return false;

  SimpleString name = p1->getText(0);
  if (!typed_var_is_valid_name(name.str()))
  {
    wingw::message_box(wingw::MBS_EXCL, "Invalid name",
      "Variable name '%s' must be a valid identifier (letters, digits, underscore; non-empty).", name.str());
    return false;
  }
  if (find_typed_var(existing, name.str()) >= 0)
  {
    wingw::message_box(wingw::MBS_EXCL, "Duplicate name", "A typed variable named '%s' already exists.", name.str());
    return false;
  }

  TypedVar v;
  v.name = name;
  v.kind = (TypedVarKind)p1->getInt(1);

  // Range expands to <name>_lo / <name>_hi -- 3 extra chars. typed_var_is_valid_name
  // caps the bare name at 63, but a 61-char Range base produces a 64-char slot
  // identifier that the lcexpr lexer rejects (`identifier too long (max 63 chars)`).
  // The variable would save and register, but no expression could reference it.
  // Cap Range bases at 60 so both derived slots fit.
  if (v.kind == TypedVarKind::Range && (int)strlen(name.str()) > 60)
  {
    wingw::message_box(wingw::MBS_EXCL, "Name too long",
      "Range '%s' would expand to '%s_lo' / '%s_hi' (%d chars), exceeding the lcexpr "
      "lexer's 63-character identifier limit. Use a Range name with at most 60 characters.",
      name.str(), name.str(), name.str(), (int)strlen(name.str()) + 3);
    return false;
  }

  // Reject collisions with builtins and mask_<layer> names. Range expands to
  // <name>_lo and <name>_hi, so check those instead of the bare name -- a
  // Range called "world" wouldn't itself enter the varMap, but world_lo /
  // world_hi would (and would clash with anything else by those names).
  HmapLandPlugin *plugin = HmapLandPlugin::self;
  auto checkReserved = [&](const char *probe) -> bool {
    if (plugin && plugin->isReservedVarName(probe))
    {
      wingw::message_box(wingw::MBS_EXCL, "Reserved name",
        "'%s' collides with a builtin (height/angle/curvature/mask/world_x/world_y) or "
        "with mask_<layer> for an existing gen layer. Pick a different name.",
        probe);
      return true;
    }
    // Catch every parser-visible name collision: a probe matches an existing
    // entry's bare name (non-Range) OR an existing Range entry's <name>_lo /
    // <name>_hi slot. find_typed_var alone misses the asymmetric Range case
    // (e.g. existing Range "foo" + new Float "foo_lo") because it only looks
    // at panel-display names.
    SimpleString conflict;
    if (plugin && plugin->collidesWithTypedVar(probe, conflict))
    {
      wingw::message_box(wingw::MBS_EXCL, "Duplicate name",
        "'%s' would collide with typed variable '%s' (parser-visible slot). Pick a different name.", probe, conflict.str());
      return true;
    }
    // recompileGenLayerExpressions registers typed vars before parsing the
    // common / layer texts. If `probe` is also declared as `var`/`let` in any
    // of those texts, the parser would reject the redeclaration on the next
    // recompile -- after the panel has already accepted the new entry. Reject
    // here so the artist hits a clear error up front.
    SimpleString site;
    if (plugin && plugin->findVarLetDeclSite(probe, site))
    {
      wingw::message_box(wingw::MBS_EXCL, "Name already declared",
        "'%s' is already declared as 'var' or 'let' in %s. Pick a different name "
        "or remove the declaration first.",
        probe, site.str());
      return true;
    }
    return false;
  };
  if (v.kind == TypedVarKind::Range)
  {
    String loName(0, "%s_lo", name.str());
    String hiName(0, "%s_hi", name.str());
    if (checkReserved(loName.str()) || checkReserved(hiName.str()))
      return false;
  }
  else if (checkReserved(name.str()))
  {
    return false;
  }

  bool ok = false;
  switch (v.kind)
  {
    case TypedVarKind::Float:
    case TypedVarKind::Range: ok = ask_step2_float_or_range(v); break;
    case TypedVarKind::Mask: ok = ask_step2_mask(v); break;
    case TypedVarKind::Curve: ok = ask_step2_curve(v); break;
  }

  if (!ok)
    return false;

  out_var = v;
  return true;
}
