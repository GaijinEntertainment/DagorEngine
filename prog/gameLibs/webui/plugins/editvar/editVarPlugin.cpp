#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_clipboard.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <util/dag_simpleString.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/editorCurves.h>
#include <webui/editVarPlugin.h>

using namespace webui;

static bool plugin_inited = false;
static bool need_update_html = false;
static Tab<GuiControlDescWebUi> vars;
static Tab<String> commands;

void de3_webui_init()
{
  plugin_inited = true;
  vars.clear();
  commands.clear();
}

bool de3_webui_check_inited() { return plugin_inited; }

void de3_webui_term()
{
  plugin_inited = false;
  vars.clear();
  commands.clear();
}

void de3_webui_enable_obj(const char *obj_name, bool enable)
{
  G_ASSERT(obj_name != NULL);
  for (int i = 0; i < vars.size(); i++)
    if (!strcmp(vars[i].name, obj_name))
    {
      commands.push_back(String(32, "%s%%enable%%%d%%:", obj_name, enable ? 1 : 0));
      vars[i].enabled = enable;
      break;
    }
}

bool de3_webui_is_obj_enabled(const char *obj_name)
{
  for (int i = 0; i < vars.size(); i++)
    if (!strcmp(vars[i].name, obj_name))
      return vars[i].enabled;

  return false;
}

void de3_webui_clear_c(const GuiControlDescWebUi *desc, int desc_num)
{
  for (int i = 0; i < desc_num; ++i)
    for (int j = 0; j < vars.size(); ++j)
      if (strcmp(vars[j].name, desc[i].name) == 0)
        erase_items(vars, j--, 1);
}

void de3_webui_build_c(const GuiControlDescWebUi *desc, int desc_num)
{
  G_ASSERT(plugin_inited);
  need_update_html = true;
  for (int i = 0; i < desc_num; i++)
  {
    vars.push_back(desc[i]);
    GuiControlDescWebUi &d = vars.back();
    switch (d.type)
    {
      case GuiControlDescWebUi::GCDT_Float: *(d.vFloat) = d.prevFloat; break;
      case GuiControlDescWebUi::GCDT_Point2: *(d.vPoint2) = d.prevPoint2; break;
      case GuiControlDescWebUi::GCDT_IPoint2: *(d.vIPoint2) = d.prevIPoint2; break;
      case GuiControlDescWebUi::GCDT_Point3: *(d.vPoint3) = d.prevPoint3; break;
      case GuiControlDescWebUi::GCDT_IPoint3: *(d.vIPoint3) = d.prevIPoint3; break;
      case GuiControlDescWebUi::GCDT_Curve: d.vCurve->parse(d.prevCurveAsText, d.curveType); break;
      case GuiControlDescWebUi::GCDT_Int:
      case GuiControlDescWebUi::GCDT_IntEnum:
      case GuiControlDescWebUi::GCDT_E3dcolor: *(d.vInt) = d.prevInt; break;
      case GuiControlDescWebUi::GCDT_Bool:
      case GuiControlDescWebUi::GCDT_BoolSingle: *(d.vBool) = d.prevBool; break;
      default: G_ASSERTF(0, "%s: unknown desc[%d].type == %d", __FUNCTION__, i, desc[i].type);
    };
  }
}


static void de3_webui_remove_object_by_index(int index)
{
  G_ASSERT(plugin_inited);
  G_ASSERT(index >= 0 && index < vars.size());
  erase_items(vars, index, 1);
  need_update_html = true;
}


bool de3_webui_remove_object(const void *value_ptr)
{
  for (int i = 0; i < vars.size(); i++)
    if (vars[i].vPtr == value_ptr)
    {
      de3_webui_remove_object_by_index(i);
      return true;
    }

  return false;
}

void de3_webui_save_values(DataBlock &dest_blk)
{
  dest_blk.clearData();
  if (!vars.size())
    return;

  for (int i = 0; i < vars.size(); i++)
  {
    const char *propName = vars[i].name;
    DataBlock *blk = NULL;
    if (const char *panelName = strchr(propName, '.'))
    {
      blk = dest_blk.addBlock(String(0, "%.*s", panelName - propName, propName));
      propName = panelName + 1;
    }
    else
      continue;

    switch (vars[i].type)
    {
      case GuiControlDescWebUi::GCDT_Float: blk->setReal(propName, *vars[i].vFloat); break;
      case GuiControlDescWebUi::GCDT_Point2: blk->setPoint2(propName, *vars[i].vPoint2); break;
      case GuiControlDescWebUi::GCDT_IPoint2: blk->setIPoint2(propName, *vars[i].vIPoint2); break;
      case GuiControlDescWebUi::GCDT_Point3: blk->setPoint3(propName, *vars[i].vPoint3); break;
      case GuiControlDescWebUi::GCDT_IPoint3: blk->setIPoint3(propName, *vars[i].vIPoint3); break;
      case GuiControlDescWebUi::GCDT_Int:
      case GuiControlDescWebUi::GCDT_IntEnum: blk->setInt(propName, *vars[i].vInt); break;
      case GuiControlDescWebUi::GCDT_Curve: blk->setStr(propName, vars[i].vCurve->asText); break;
      case GuiControlDescWebUi::GCDT_E3dcolor: blk->setE3dcolor(propName, E3DCOLOR(*vars[i].vInt)); break;
      case GuiControlDescWebUi::GCDT_Bool: blk->setBool(propName, *vars[i].vBool); break;
      case GuiControlDescWebUi::GCDT_BoolSingle: break;
      default: G_ASSERTF(0, "unknown type == %d", vars[i].type);
    }
  }
}

void de3_webui_load_values(const DataBlock &src_blk)
{
  if (!vars.size())
    return;

  for (int i = 0; i < vars.size(); i++)
  {
    const char *propName = vars[i].name;
    const DataBlock *blk = NULL;
    if (const char *panelName = strchr(propName, '.'))
    {
      blk = src_blk.getBlockByName(String(0, "%.*s", panelName - propName, propName));
      propName = panelName + 1;
    }
    else
      continue;

    if (!blk || !blk->paramExists(propName))
      continue;

    switch (vars[i].type)
    {
      case GuiControlDescWebUi::GCDT_Float: *(vars[i].vFloat) = blk->getReal(propName); break;
      case GuiControlDescWebUi::GCDT_Point2: *(vars[i].vPoint2) = blk->getPoint2(propName); break;
      case GuiControlDescWebUi::GCDT_IPoint2: *(vars[i].vIPoint2) = blk->getIPoint2(propName); break;
      case GuiControlDescWebUi::GCDT_Point3: *(vars[i].vPoint3) = blk->getPoint3(propName); break;
      case GuiControlDescWebUi::GCDT_IPoint3: *(vars[i].vIPoint3) = blk->getIPoint3(propName); break;
      case GuiControlDescWebUi::GCDT_Int:
      case GuiControlDescWebUi::GCDT_IntEnum: *(vars[i].vInt) = blk->getInt(propName); break;
      case GuiControlDescWebUi::GCDT_Curve: vars[i].vCurve->parse(blk->getStr(propName), vars[i].curveType); break;
      case GuiControlDescWebUi::GCDT_E3dcolor: *(vars[i].vInt) = int(blk->getE3dcolor(propName)); break;
      case GuiControlDescWebUi::GCDT_Bool: *(vars[i].vBool) = blk->getBool(propName); break;
      case GuiControlDescWebUi::GCDT_BoolSingle: break;
      default: G_ASSERTF(0, "unknown type == %d", vars[i].type);
    }
  }
}


void on_editvar(RequestInfo *params)
{
  if (!plugin_inited)
  {
    html_response_raw(params->conn, "");
    return;
  }

  if (!params->params)
  {
    const char *inlinedEditVar =
#include "editVar.html.inl"
      ;
    html_response_raw(params->conn, inlinedEditVar);
  }
  else if (!strcmp(params->params[0], "layout"))
  {
    String s(tmpmem);
    s.reserve(8000);
    String buf;
    for (int i = 0; i < vars.size(); i++)
    {
      switch (vars[i].type)
      {
        case GuiControlDescWebUi::GCDT_Float: buf.printf(64, "%.6g", *(vars[i].vFloat)); break;
        case GuiControlDescWebUi::GCDT_Point2: buf.printf(64, "%.6g,%.6g", (*(vars[i].vPoint2)).x, (*(vars[i].vPoint2)).y); break;
        case GuiControlDescWebUi::GCDT_IPoint2: buf.printf(64, "%d,%d", (*(vars[i].vIPoint2)).x, (*(vars[i].vIPoint2)).y); break;
        case GuiControlDescWebUi::GCDT_Point3:
          buf.printf(64, "%.6g,%.6g,%.6g", (*(vars[i].vPoint3)).x, (*(vars[i].vPoint3)).y, (*(vars[i].vPoint3)).z);
          break;
        case GuiControlDescWebUi::GCDT_IPoint3:
          buf.printf(64, "%d,%d,%d", (*(vars[i].vIPoint3)).x, (*(vars[i].vIPoint3)).y, (*(vars[i].vIPoint3)).z);
          break;
        case GuiControlDescWebUi::GCDT_Int: buf.printf(64, "%d", *(vars[i].vInt)); break;
        case GuiControlDescWebUi::GCDT_Curve: buf.printf(64, "%s", vars[i].vCurve->asText.str()); break;
        case GuiControlDescWebUi::GCDT_E3dcolor:
        {
          E3DCOLOR c(*(vars[i].vInt));
          buf.printf(64, "%02x%02x%02x%02x", int(c.r), int(c.g), int(c.b), int(c.a));
        }
        break;
        case GuiControlDescWebUi::GCDT_IntEnum:
        {
          int index = 0;
          for (int k = 0; k < countof(vars[i].iVals); k++)
            if (vars[i].iVals[k] == *(vars[i].vInt))
            {
              index = k;
              break;
            }
          buf.printf(64, "%d", index);
        }
        break;
        case GuiControlDescWebUi::GCDT_Bool:
        case GuiControlDescWebUi::GCDT_BoolSingle: buf.printf(64, "%d", *(vars[i].vBool) ? 1 : 0); break;
        default: buf.setStr(""); G_ASSERTF(0, "unknown type == %d", vars[i].type);
      };

      String layout(tmpmem);
      layout = vars[i].layout;
      layout.replaceAll("<def>", buf);
      s += layout.str();

      if (!vars[i].enabled)
        commands.push_back(String(32, "%s%%enable%%0%%:", vars[i].name));
    }

    need_update_html = false;
    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "state"))
  {
    String s(tmpmem);
    s.reserve(8000);
    for (int i = 0; i < commands.size(); i++)
      s += commands[i];

    commands.clear();

    for (int i = 0; i < vars.size(); i++)
      switch (vars[i].type)
      {
        case GuiControlDescWebUi::GCDT_Float:
          if (*(vars[i].vFloat) != vars[i].prevFloat)
          {
            s.aprintf(64, "%s%%value%%%.6g%%:", vars[i].name, *(vars[i].vFloat));
            vars[i].prevFloat = *(vars[i].vFloat);
          }
          break;
        case GuiControlDescWebUi::GCDT_Point2:
          if (*(vars[i].vPoint2) != vars[i].prevPoint2)
          {
            s.aprintf(64, "%s%%value%%%.6g,%.6g%%:", vars[i].name, (*(vars[i].vPoint2)).x, (*(vars[i].vPoint2)).y);
            vars[i].prevPoint2 = *(vars[i].vPoint2);
          }
          break;
        case GuiControlDescWebUi::GCDT_IPoint2:
          if (*(vars[i].vIPoint2) != vars[i].prevIPoint2)
          {
            s.aprintf(64, "%s%%value%%d,%d%%:", vars[i].name, (*(vars[i].vIPoint2)).x, (*(vars[i].vIPoint2)).y);
            vars[i].prevIPoint2 = *(vars[i].vIPoint2);
          }
          break;
        case GuiControlDescWebUi::GCDT_Point3:
          if (*(vars[i].vPoint3) != vars[i].prevPoint3)
          {
            s.aprintf(64, "%s%%value%%%.6g,%.6g,%.6g%%:", vars[i].name, (*(vars[i].vPoint3)).x, (*(vars[i].vPoint3)).y,
              (*(vars[i].vPoint3)).z);
            vars[i].prevPoint3 = *(vars[i].vPoint3);
          }
          break;
        case GuiControlDescWebUi::GCDT_IPoint3:
          if (*(vars[i].vIPoint3) != vars[i].prevIPoint3)
          {
            s.aprintf(64, "%s%%value%%d,%d,%d%%:", vars[i].name, (*(vars[i].vIPoint3)).x, (*(vars[i].vIPoint3)).y,
              (*(vars[i].vIPoint3)).y);
            vars[i].prevIPoint3 = *(vars[i].vIPoint3);
          }
          break;
        case GuiControlDescWebUi::GCDT_Int:
          if (*(vars[i].vInt) != vars[i].prevInt)
          {
            s.aprintf(64, "%s%%value%%%d%%:", vars[i].name, *(vars[i].vInt));
            vars[i].prevInt = *(vars[i].vInt);
          }
          break;
        case GuiControlDescWebUi::GCDT_Curve:
          if (vars[i].vCurve->asText != vars[i].prevCurveAsText)
          {
            s.aprintf(256, "%s%%value%%%s%%:", vars[i].name, vars[i].vCurve->asText.str());
            vars[i].prevCurveAsText = vars[i].vCurve->asText.str();
          }
          break;
        case GuiControlDescWebUi::GCDT_E3dcolor:
          if (*(vars[i].vInt) != vars[i].prevInt)
          {
            E3DCOLOR c(*(vars[i].vInt));
            s.aprintf(64, "%s%%value%%%02x%02x%02x%02x%%:", vars[i].name, int(c.r), int(c.g), int(c.b), int(c.a));
            vars[i].prevInt = *(vars[i].vInt);
          }
          break;
        case GuiControlDescWebUi::GCDT_IntEnum:
          if (*(vars[i].vInt) != vars[i].prevInt)
          {
            int index = 0;
            for (int k = 0; k < countof(vars[i].iVals); k++)
              if (vars[i].iVals[k] == *(vars[i].vInt))
              {
                index = k;
                break;
              }
            s.aprintf(64, "%s%%value%%%d%%:", vars[i].name, index);
            vars[i].prevInt = *(vars[i].vInt);
          }
          break;
        case GuiControlDescWebUi::GCDT_Bool:
        case GuiControlDescWebUi::GCDT_BoolSingle:
          if (*(vars[i].vBool) != vars[i].prevBool)
          {
            s.aprintf(64, "%s%%value%%%d%%:", vars[i].name, *(vars[i].vBool) ? 1 : 0);
            vars[i].prevBool = *(vars[i].vBool);
          }
          break;
        default: G_ASSERTF(0, "unknown type == %d", vars[i].type);
      };

    if (need_update_html)
    {
      s.aprintf(32, "%s", "refresh_layout%:");
    }

    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "setvar") && params->params[2] && params->params[4])
  {
    String varName(params->params[2]);
    char *atPos = strchr(varName, '@');
    int tupleIndex = 0;
    if (atPos)
    {
      if (atPos[1] == '0' || atPos[1] == '1' || atPos[1] == '2')
      {
        tupleIndex = atPos[1] - '0';
        *atPos = 0;
      }
      else
        G_ASSERTF(0, "invalid tuple index in variable name '%s'", varName.str());
    }

    for (int i = 0; i < vars.size(); i++)
      if (!strcmp(vars[i].name, varName))
      {
        switch (vars[i].type)
        {
          case GuiControlDescWebUi::GCDT_Float: vars[i].prevFloat = *(vars[i].vFloat) = atof(params->params[4]); break;
          case GuiControlDescWebUi::GCDT_Point2:
            (*(vars[i].vPoint2))[tupleIndex] = vars[i].prevPoint2[tupleIndex] = atof(params->params[4]);
            break;
          case GuiControlDescWebUi::GCDT_Point3:
            (*(vars[i].vPoint3))[tupleIndex] = vars[i].prevPoint3[tupleIndex] = atof(params->params[4]);
            break;
          case GuiControlDescWebUi::GCDT_IPoint2:
            (*(vars[i].vIPoint2))[tupleIndex] = vars[i].prevIPoint2[tupleIndex] = atoi(params->params[4]);
            break;
          case GuiControlDescWebUi::GCDT_IPoint3:
            (*(vars[i].vIPoint3))[tupleIndex] = vars[i].prevIPoint3[tupleIndex] = atoi(params->params[4]);
            break;
          case GuiControlDescWebUi::GCDT_Int: vars[i].prevInt = *(vars[i].vInt) = int(atof(params->params[4]) + 0.5f); break;
          case GuiControlDescWebUi::GCDT_Curve:
            vars[i].vCurve->parse(params->params[4]);
            vars[i].prevCurveAsText = vars[i].vCurve->asText.str();
            break;
          case GuiControlDescWebUi::GCDT_E3dcolor:
          {
            unsigned int rr, gg, bb, aa;
            sscanf(params->params[4], "%02x%02x%02x%02x", &rr, &gg, &bb, &aa);
            E3DCOLOR c(rr, gg, bb, aa);
            vars[i].prevInt = *(vars[i].vInt) = int(c);
          }
          break;
          case GuiControlDescWebUi::GCDT_IntEnum:
          {
            int index = clamp(atoi(params->params[4]), 0, int(countof(vars[i].iVals) - 1));
            vars[i].prevInt = *(vars[i].vInt) = vars[i].iVals[index];
          }
          break;
          case GuiControlDescWebUi::GCDT_Bool:
          case GuiControlDescWebUi::GCDT_BoolSingle: vars[i].prevBool = *(vars[i].vBool) = (params->params[4][0] != '0'); break;
          default: G_ASSERTF(0, "unknown type == %d", vars[i].type);
        };

        if (vars[i].onChangeCb)
          vars[i].onChangeCb->varChanged = true;
      }

    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "copy_to_clipboard"))
  {
    DataBlock blk;
    de3_webui_save_values(blk);
    DynamicMemGeneralSaveCB cwr(tmpmem);
    blk.saveToTextStream(cwr);
    char zero = 0;
    cwr.write(&zero, 1);
    clipboard::set_clipboard_utf8_text((const char *)cwr.data());

    html_response_raw(params->conn, "");
  }
  // else if (!strcmp(params->params[0], "detach")) {}
}

EditableVariablesNotifications::~EditableVariablesNotifications()
{
  for (int i = 0; i < ptrs.size(); i++)
    de3_webui_remove_object(ptrs[i]);
}


const char *EditableVariablesNotifications::makePermanent(const char *str_ptr)
{
  strings.push_back(SimpleString(str_ptr));
  return strings.back().str();
}

#define COMMON_DESC_INIT                                     \
  GuiControlDescWebUi desc;                                  \
  desc.reset();                                              \
  desc.vPtr = (void *)ptr;                                   \
  ptrs.push_back(desc.vPtr);                                 \
  desc.enabled = true;                                       \
  desc.onChangeCb = this;                                    \
  char buf[128];                                             \
  SNPRINTF(buf, sizeof(buf), "%s.%s", panel_name, var_name); \
  desc.name = makePermanent(buf);


void EditableVariablesNotifications::registerIntEditbox(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "int_editbox%%%s%%%s%%%d%%%d%%<def>%%:", panel_name, var_name, min_v, max_v);
  desc.layout = makePermanent(buf);
  desc.prevInt = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Int;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerIntSlider(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "int_slider%%%s%%%s%%%d%%%d%%<def>%%:", panel_name, var_name, min_v, max_v);
  desc.layout = makePermanent(buf);
  desc.prevInt = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Int;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerFloatEditbox(float *ptr, float min_v, float max_v, float step, const char *panel_name,
  const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "float_editbox%%%s%%%s%%%f%%%f%%<def>%%%f%%:", panel_name, var_name, min_v, max_v, step);
  desc.layout = makePermanent(buf);
  desc.prevFloat = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Float;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerPoint2Editbox(Point2 *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "point2_editbox%%%s%%%s%%<def>%%:", panel_name, var_name);
  desc.layout = makePermanent(buf);
  desc.prevPoint2 = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Point2;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerIPoint2Editbox(IPoint2 *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "ipoint2_editbox%%%s%%%s%%<def>%%:", panel_name, var_name);
  desc.layout = makePermanent(buf);
  desc.prevIPoint2 = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_IPoint2;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerPoint3Editbox(Point3 *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "point3_editbox%%%s%%%s%%<def>%%:", panel_name, var_name);
  desc.layout = makePermanent(buf);
  desc.prevPoint3 = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Point3;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerIPoint3Editbox(IPoint3 *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "ipoint3_editbox%%%s%%%s%%<def>%%:", panel_name, var_name);
  desc.layout = makePermanent(buf);
  desc.prevIPoint3 = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_IPoint3;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerFloatSlider(float *ptr, float min_v, float max_v, float step, const char *panel_name,
  const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "float_slider%%%s%%%s%%%f%%%f%%<def>%%%f%%:", panel_name, var_name, min_v, max_v, step);
  desc.layout = makePermanent(buf);
  desc.prevFloat = *ptr;
  desc.type = GuiControlDescWebUi::GCDT_Float;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerE3dcolor(E3DCOLOR *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;
  SNPRINTF(buf, sizeof(buf), "e3dcolor%%%s%%%s%%<def>%%:", panel_name, var_name);
  desc.layout = makePermanent(buf);
  desc.prevInt = *(int *)ptr;
  desc.type = GuiControlDescWebUi::GCDT_E3dcolor;

  de3_webui_build_c(&desc, 1);
}

void EditableVariablesNotifications::registerCurve(EditorCurve *ptr, const char *panel_name, const char *var_name)
{
  COMMON_DESC_INIT;

  const char *curveTypeStr = NULL;
  switch (ptr->getType())
  {
    case EditorCurve::STEPS: curveTypeStr = "steps_curve"; break;
    case EditorCurve::POLYNOM: curveTypeStr = "polynom_curve"; break;
    case EditorCurve::PIECEWISE_LINEAR: curveTypeStr = "linear_curve"; break;
    case EditorCurve::PIECEWISE_MONOTONIC: curveTypeStr = "monotonic_curve"; break;

    default: G_ASSERTF(0, "Curve type is unset: %d", ptr->getType()); curveTypeStr = "";
  };

  SNPRINTF(buf, sizeof(buf), "%s%%%s%%%s%%<def>%%:", curveTypeStr, panel_name, var_name);
  desc.prevCurveAsText = ptr->asText;
  desc.layout = makePermanent(buf);
  desc.type = GuiControlDescWebUi::GCDT_Curve;
  desc.curveType = ptr->getType();

  de3_webui_build_c(&desc, 1);
}


void EditableVariablesNotifications::registerVariable(void *value_ptr)
{
  for (int i = 0; i < vars.size(); i++)
    if (vars[i].vPtr == value_ptr)
    {
      vars[i].onChangeCb = this;
      ptrs.push_back(value_ptr);
      return;
    }

  G_ASSERTF(0, "registerVariable: pointer not found");
}

void EditableVariablesNotifications::unregisterVariable(void *value_ptr)
{
  for (int i = 0; i < vars.size(); i++)
    if (vars[i].vPtr == value_ptr)
      vars[i].onChangeCb = NULL;

  erase_item_by_value(ptrs, value_ptr);
}

void EditableVariablesNotifications::removeVariable(void *value_ptr)
{
  de3_webui_remove_object(value_ptr);
  erase_item_by_value(ptrs, value_ptr);
}


webui::HttpPlugin webui::editvar_http_plugin = {"editvar", "show variable editor", NULL, on_editvar};
