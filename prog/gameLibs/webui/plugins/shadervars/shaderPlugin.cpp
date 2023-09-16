#include <webui/httpserver.h>
#include <util/dag_lookup.h>
#include <errno.h>

#include <stdio.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderVar.h>
#include <math/dag_color.h>
#include <math/dag_TMatrix4.h>
#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include "webui_internal.h"

using namespace webui;

static const char *get_sh_var_value(const String &name, int var_id, ShaderVarType type, char *buf, int b_size)
{
  switch (type)
  {
    case SHVT_FLOAT4X4:
    {
      TMatrix4 mat = ShaderGlobal::get_float4x4(var_id);
      SNPRINTF(buf, b_size, "(%.3f %.3f %.3f %.3f)<br/>(%.3f %.3f %.3f %.3f)<br/>(%.3f %.3f %.3f %.3f)<br/>(%.3f %.3f %.3f %.3f)",
        P4D(mat.getrow(0)), P4D(mat.getrow(1)), P4D(mat.getrow(2)), P4D(mat.getrow(3)));
    }
    break;
    case SHVT_INT:
    {
      ShaderGlobal::Interval interv(name, name.length());
      if (ShaderGlobal::is_glob_interval_presented(interv))
      {
        ShaderGlobal::Subinterval currSubinterv = ShaderGlobal::get_variant(interv);
        dag::Vector<String> subintervs = ShaderGlobal::get_subinterval_names(interv);
        for (auto &subinterv : subintervs)
          if (currSubinterv == ShaderGlobal::Subinterval(subinterv, subinterv.length()))
            SNPRINTF(buf, b_size, "%s (%d)", subinterv.c_str(), ShaderGlobal::get_int(var_id));
      }
      else
        SNPRINTF(buf, b_size, "%d", ShaderGlobal::get_int(var_id));
      break;
    }
    case SHVT_INT4:
    {
      const IPoint4 &p = ShaderGlobal::get_int4(var_id);
      SNPRINTF(buf, b_size, "(%d, %d, %d, %d)", p.x, p.y, p.z, p.w);
    }
    break;
    case SHVT_REAL: SNPRINTF(buf, b_size, "%.3f", ShaderGlobal::get_real(var_id)); break;
    case SHVT_COLOR4:
    {
      const Color4 &c = ShaderGlobal::get_color4(var_id);
      SNPRINTF(buf, b_size, "(%.3f, %.3f, %.3f, %.3f)", c.r, c.g, c.b, c.a);
    }
    break;
    case SHVT_TEXTURE:
    {
      TEXTUREID texId = ShaderGlobal::get_tex(var_id);
      if (texId == BAD_TEXTUREID)
        return "BAD_TEXTUREID";
      else
      {
        if (const char *n = get_managed_texture_name(texId))
          SNPRINTF(buf, b_size, "tex #0x%x ('%s')", unsigned(texId), n);
        else
          SNPRINTF(buf, b_size, "tex #0x%x", unsigned(texId));
      }
    }
    break;
    default: return "unknown";
  }
  return buf;
}

static const char *get_sh_interval_ranges(const String &name, int var_id, char *buf, int b_size)
{
  int w = 0;
  int type = ShaderGlobal::get_var_type(var_id);
  dag::ConstSpan<float> maxVal = ShaderGlobal::get_interval_ranges(var_id);

  ShaderGlobal::Interval interv(name, name.length());
  if (ShaderGlobal::is_glob_interval_presented(interv) && type == SHVT_INT)
  {
    dag::Vector<String> subintervs = ShaderGlobal::get_subinterval_names(interv);

    for (uint32_t i = 0; i < maxVal.size(); i++)
      w += _snprintf(buf + w, b_size - w, "%s<br/>", subintervs[i].c_str());
    _snprintf(buf + w, b_size - w, "%s", subintervs.back().c_str());

    buf[b_size - 1] = 0;
    return buf;
  }

  w = _snprintf(buf, b_size, "(");
  int saved_w = w;
  bool ascending = true;
  float prev = 0.f;

  for (int i = 0; i < maxVal.size(); ++i)
  {
    if (i)
      ascending &= maxVal[i] - 1.f == prev;
    prev = maxVal[i];
    w += _snprintf(buf + w, b_size - w, "%.1f%s", maxVal[i], i == (maxVal.size() - 1) ? ")" : ",");
  }
  if ((type == SHVT_INT || type == SHVT_TEXTURE) && ascending && (maxVal.size() > 1 || maxVal[0] == 1.f)) // folding
    _snprintf(buf + saved_w, b_size - saved_w, "%d-%d)", (int)maxVal[0] - 1, (int)maxVal.back());
  buf[b_size - 1] = 0;
  return buf;
}

static const char *str_color4_html(char *buf, int buf_size, const Color4 &c)
{
  if (c.r < 0.0 || c.r > 1.0 || c.g < 0.0 || c.g > 1.0 || c.b < 0.0 || c.b > 1.0)
    return "#not_a_color";
#define TO_INT(cc) (int)floorf((cc)*255.f + 0.5f)
  SNPRINTF(buf, buf_size, "#%02x%02x%02x", TO_INT(c.r), TO_INT(c.g), TO_INT(c.b));
#undef TO_INT
  return buf;
}

static const char *str_color4_float(char *buf, int buf_size, const Color4 &c)
{
  SNPRINTF(buf, buf_size, "%.3f, %.3f, %.3f, %.3f", c.r, c.g, c.b, c.a);
  return buf;
}


static bool parse_float_color(const char *scolor, Color4 &c)
{
  if (*scolor == '#')
    return false;

  float r, g, b, a;
  if (sscanf(scolor, "%f,%f,%f,%f", &r, &g, &b, &a) != 4)
    return false;

  c.r = clamp(r, 0.f, 1.f);
  c.g = clamp(g, 0.f, 1.f);
  c.b = clamp(b, 0.f, 1.f);
  c.a = clamp(a, 0.f, 1.f);

  return true;
}

static void on_shader_vars(RequestInfo *params)
{
  YAMemSave buf;
  char stack_buf[256];
  char stack_buf2[256];
  const char *tn[] = {"INT", "REAL", "COL4", "TEX", "BUF", "INT4", "MAT"};
  const char *hs[] = {
    "#",
    "name",
    "type",
    "value",
    "assotiated interval",
    "color",
  };

  const char *script =
#include "shaderVars.html.inl"
    ;

  if (params->params)
  {
    const char *cmds[] = {"color", "range", "color4"};
    int cmdi = lup(*params->params, cmds, countof(cmds));
    switch (cmdi)
    {
      case 0: // color
      {
        logerr("n/a");
      }
      break;
      case 1: // range
      {
        int nparams = 0;
        for (char **pp = params->params; *pp; pp++, nparams++)
          ;
        if (nparams < 4)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        if (strcmp(*(params->params + 2), "id") != 0) // second parameter must be id
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        int id = strtol(*(params->params + 3), NULL, 0);
        if (ShaderGlobal::get_var_type(id) != SHVT_REAL)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        errno = 0;
        double val = strtod(*(params->params + 1), NULL);
        if (errno != 0)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        ShaderGlobal::set_real(id, val);
      }
      break;
      case 2: // color4
      {
        int nparams = 0;
        for (char **pp = params->params; *pp; pp++, nparams++)
          ;
        if (nparams < 4)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        if (strcmp(*(params->params + 2), "id") != 0) // second parameter must be id
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        int id = strtol(*(params->params + 3), NULL, 0);
        if (ShaderGlobal::get_var_type(id) != SHVT_COLOR4)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        Color4 new_color;
        if (!parse_float_color(*(params->params + 1), new_color))
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        ShaderGlobal::set_color4(id, new_color);
      }
      break;
      default: return html_response(params->conn, NULL, HTTP_NOT_FOUND);
    };
    return html_response(params->conn, NULL, HTTP_OK);
  }

  buf.printf("%s", script);

  buf.printf("\n<table border>\n");
  print_table_header(buf, hs, countof(hs));

  int row = 0;
  for (const String &varName : VariableMap::getPresentedGlobalVariableNames())
  {
    // By getting ID of variable we may lose information that this variable was never used in code
    // but this doesn't look like a real issue. If someone need this info he can just not open this
    // webui window.
    int varId = VariableMap::getVariableId(varName);

    int type = ShaderGlobal::get_var_type(varId);
    G_ASSERT(type >= 0);
    if (type >= countof(tn))
    {
      logerr("unkown shaderVar type %d", type);
    }

    buf.printf("  <tr align=\"center\">");
    TCOL("%d", row++);
    TCOL("%s", varName);
    TCOL("%s", type < countof(tn) ? tn[type] : "unknown");
    buf.printf("<td id='sh_val_%d'>%s</td>", varId,
      get_sh_var_value(varName, varId, static_cast<ShaderVarType>(type), stack_buf, sizeof(stack_buf)));
    if (ShaderGlobal::has_associated_interval(varId))
      TCOL("%s",
        ShaderGlobal::is_var_assumed(varId) ? "assumed" : get_sh_interval_ranges(varName, varId, stack_buf, sizeof(stack_buf)));
    else
      TCOL("");
    if (type == SHVT_COLOR4)
      TCOL("<button style=\"background:%s; width:50;\" name=\"%s\" "
           "onclick=\"show_color_picker(this,%d,shadervars_color_cb)\">&nbsp;</button>",
        str_color4_html(stack_buf, sizeof(stack_buf), ShaderGlobal::get_color4(varId)),
        str_color4_float(stack_buf2, sizeof(stack_buf2), ShaderGlobal::get_color4(varId)), varId);
    else
      TCOL("n/a");
    if (type == SHVT_REAL)
    {
      float cur_val = ShaderGlobal::get_real(varId);
      float abs_cur_val = fabsf(cur_val);
      float norm_cur_val = abs_cur_val < 1e-6f ? 1.f : abs_cur_val;
      float min_val = cur_val - norm_cur_val;
      float max_val = cur_val + norm_cur_val;
      float step = abs_cur_val < 1e-6f ? 0.01f : abs_cur_val / 100.f;
      TCOL("<input id='edit_box_%d' type='text' value='%.04f' onchange='on_edit_box(%d)'>"
           "<input id='slider_%d' type='range' min='%.04f' max='%.04f' value='%.04f' step='%.04f' onchange='on_slider(%d)'>",
        varId, cur_val, varId, varId, min_val, max_val, cur_val, step, varId);
    }
    else
      TCOL("n/a");

    buf.printf("</tr>\n");
  }
  buf.printf("</table>\n");

  html_response(params->conn, buf.mem);
}

webui::HttpPlugin webui::shader_http_plugin = {"shader_vars", "show global shader vars", NULL, on_shader_vars};
