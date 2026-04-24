// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "robjShader.h"
#include "elementTree.h"
#include "guiScene.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderState.h>

#include <gui/dag_stdGuiRender.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderCommon.h>

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_consts_base.h>

#include <osApiWrappers/dag_files.h>
#include <math/dag_color.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_info.h>
#endif

#include <perfMon/dag_cpuFreq.h>


namespace darg
{


//
// Shader global variable IDs for production path
//

static int darg_frame_params_varId = -2;
static int darg_cursor_params_varId = -2;
static int darg_params0_varId = -2;
static int darg_params1_varId = -2;
static int darg_params2_varId = -2;
static int darg_params3_varId = -2;

static void ensure_shader_var_ids()
{
  if (darg_frame_params_varId != -2)
    return;
  darg_frame_params_varId = get_shader_variable_id("darg_frame_params", true);
  darg_cursor_params_varId = get_shader_variable_id("darg_cursor_params", true);
  darg_params0_varId = get_shader_variable_id("darg_params0", true);
  darg_params1_varId = get_shader_variable_id("darg_params1", true);
  darg_params2_varId = get_shader_variable_id("darg_params2", true);
  darg_params3_varId = get_shader_variable_id("darg_params3", true);
}

#if _TARGET_PC_WIN

static bool read_file_to_string(const char *path, String &out)
{
  file_ptr_t fp = df_open(path, DF_READ);
  if (!fp)
    return false;
  int len = df_length(fp);
  if (len <= 0)
  {
    df_close(fp);
    return false;
  }
  out.resize(len + 1);
  int bytes_read = df_read(fp, out.data(), len);
  df_close(fp);
  if (bytes_read != len)
    return false;
  out[len] = '\0';
  return true;
}

//
// HLSL sources baked as string constants
//

// Fullscreen triangle VS (same math as postfx_inc.dshl get_fullscreen_inpos)
static const char fullscreen_vs_hlsl[] = "struct VsOutput {\n"
                                         "  float4 pos : SV_Position;\n"
                                         "  noperspective float2 texcoord : TEXCOORD0;\n"
                                         "};\n"
                                         "\n"
                                         "VsOutput main_vs(uint vertexId : SV_VertexID) {\n"
                                         "  VsOutput o;\n"
                                         "  float2 inpos = float2((vertexId == 2) ? 3.0 : -1.0,\n"
                                         "                         (vertexId == 1) ? -3.0 : 1.0);\n"
                                         "  o.pos = float4(inpos, 0.0, 1.0);\n"
                                         "  o.texcoord = inpos * float2(0.5, -0.5) + 0.5;\n"
                                         "  return o;\n"
                                         "}\n";

// PS preamble: declares built-in globals before user code
static const char ps_preamble[] = "struct VsOutput {\n"
                                  "  float4 pos : SV_Position;\n"
                                  "  noperspective float2 texcoord : TEXCOORD0;\n"
                                  "};\n"
                                  "\n"
                                  "float4 _darg_c0 : register(c0);\n"
                                  "float4 _darg_c1 : register(c1);\n"
                                  "float4 darg_params0 : register(c2);\n"
                                  "float4 darg_params1 : register(c3);\n"
                                  "float4 darg_params2 : register(c4);\n"
                                  "float4 darg_params3 : register(c5);\n"
                                  "\n"
                                  "#define darg_time       _darg_c0.x\n"
                                  "#define darg_opacity    _darg_c0.y\n"
                                  "#define darg_resolution _darg_c0.zw\n"
                                  "#define darg_cursor     _darg_c1\n"
                                  "\n";

// PS entry point wrapper appended after user code
static const char ps_entry_point[] = "\n"
                                     "float4 main_ps(VsOutput input) : SV_Target {\n"
                                     "  return darg_ps(input.texcoord);\n"
                                     "}\n";


//
// Runtime shader compilation (dev path, PC only)
//

static VDECL empty_vdecl = BAD_VDECL;
static int rt_program_count = 0;

static VDECL get_empty_vdecl()
{
  if (empty_vdecl == BAD_VDECL)
  {
    VSDTYPE vsd[] = {VSD_STREAM(0), VSD_END};
    empty_vdecl = d3d::create_vdecl(vsd);
  }
  return empty_vdecl;
}

static void release_empty_vdecl_if_unused()
{
  if (rt_program_count <= 0 && empty_vdecl != BAD_VDECL)
  {
    d3d::delete_vdecl(empty_vdecl);
    empty_vdecl = BAD_VDECL;
  }
}

static PROGRAM compile_shader_runtime(const char *hlsl_body)
{
  String ps_source(0, "%s%s%s", ps_preamble, hlsl_body, ps_entry_point);

  const char *vs_profile = d3d::get_driver_code().is(d3d::dx12) ? "vs_6_0" : "vs_5_0";
  const char *ps_profile = d3d::get_driver_code().is(d3d::dx12) ? "ps_6_0" : "ps_5_0";

  String vs_err, ps_err;
  VPROG vs =
    d3d::create_vertex_shader_hlsl(fullscreen_vs_hlsl, (unsigned)(sizeof(fullscreen_vs_hlsl) - 1), "main_vs", vs_profile, &vs_err);
  if (vs == BAD_VPROG)
  {
    logerr("ROBJ_SHADER: VS compilation failed:\n%s", vs_err.c_str());
    return BAD_PROGRAM;
  }

  FSHADER ps = d3d::create_pixel_shader_hlsl(ps_source.c_str(), (unsigned)ps_source.length(), "main_ps", ps_profile, &ps_err);
  if (ps == BAD_FSHADER)
  {
    logerr("ROBJ_SHADER: PS compilation failed:\n%s", ps_err.c_str());
    d3d::delete_vertex_shader(vs);
    return BAD_PROGRAM;
  }

  PROGRAM prog = d3d::create_program(vs, ps, get_empty_vdecl());
  if (prog == BAD_PROGRAM)
  {
    logerr("ROBJ_SHADER: program creation failed");
    d3d::delete_vertex_shader(vs);
    d3d::delete_pixel_shader(ps);
  }
  else
    rt_program_count++;
  return prog;
}
#endif


//
// RobjShaderParams
//

RobjShaderParams::~RobjShaderParams()
{
  if (rtProgram != BAD_PROGRAM)
  {
    d3d::delete_program(rtProgram);
#if _TARGET_PC_WIN
    rt_program_count--;
    release_empty_vdecl_if_unused();
#endif
  }
}

static const char *read_sq_str(const Sqrat::Object &s) { return s.GetType() == OT_STRING ? s.GetVar<const char *>().value : ""; }

bool RobjShaderParams::load(const Element *elem)
{
  ensure_shader_var_ids();

  const Properties &props = elem->props;
  const StringKeys *csk = elem->csk;

  // Clean up previous state
  if (rtProgram != BAD_PROGRAM)
  {
    d3d::delete_program(rtProgram);
    rtProgram = BAD_PROGRAM;
#if _TARGET_PC_WIN
    rt_program_count--;
#endif
  }
  material = NULL;
  shaderElem = NULL;

  shaderName = read_sq_str(props.scriptDesc.RawGetSlot(csk->shaderName));
  shaderSource = read_sq_str(props.scriptDesc.RawGetSlot(csk->shaderSource));

  brightness = props.getFloat(csk->brightness, 1.0f);

  // Read shader params (Color4 values from table)
  for (int i = 0; i < 4; i++)
    params[i] = Color4(0, 0, 0, 0);

  Sqrat::Object paramsObj = props.getObject(csk->shaderParams);
  if (paramsObj.GetType() == OT_TABLE)
  {
    static const char *paramNames[4] = {"params0", "params1", "params2", "params3"};
    Sqrat::Table paramsTable(paramsObj);
    for (int i = 0; i < 4; i++)
    {
      Sqrat::Object val = paramsTable.RawGetSlot(paramNames[i]);
      if (val.IsNull())
        continue;
      if (Sqrat::ClassType<Color4>::IsClassInstance(val.GetObject()))
        params[i] = *val.Cast<const Color4 *>();
    }
  }

  // Resolve shader: shaderSource (dev) takes priority over shaderName (production)
#if _TARGET_PC_WIN
  if (shaderSource.length() > 0)
  {
    if (!d3d::get_driver_code().is(d3d::dx11) && !d3d::get_driver_code().is(d3d::dx12))
    {
      logerr("ROBJ_SHADER: runtime HLSL compilation requires DX11 or DX12 (current driver: %s), falling back to shaderName",
        d3d::get_driver_name());
    }
    else
    {
      String hlslBody;
      if (read_file_to_string(shaderSource.c_str(), hlslBody))
      {
        rtProgram = compile_shader_runtime(hlslBody.c_str());
        if (rtProgram != BAD_PROGRAM)
          return true;
        logerr("ROBJ_SHADER: runtime compilation failed for '%s', falling back to shaderName", shaderSource.c_str());
      }
      else
      {
        logerr("ROBJ_SHADER: cannot read shader source '%s'", shaderSource.c_str());
      }
    }
  }
#endif

  if (shaderName.length() > 0)
  {
    material = new_shader_material_by_name_optional(shaderName.c_str());
    if (material.get())
      shaderElem = material->make_elem();
    if (!shaderElem.get())
      logerr("ROBJ_SHADER: shader '%s' not found in shader dump", shaderName.c_str());
  }

  return true;
}


bool RobjShaderParams::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


//
// Render callback
//

static constexpr int DARG_SHADER_CMD = 42;

static void shader_render_callback(StdGuiRender::CallBackState &cbs)
{
  G_ASSERT_RETURN(cbs.command == DARG_SHADER_CMD, );
  RobjShaderParams *p = (RobjShaderParams *)cbs.data;

  // Set viewport to element bounds so fullscreen triangle fills only this element
  d3d::setview((int)cbs.pos.x, (int)cbs.pos.y, (int)cbs.size.x, (int)cbs.size.y, 0.f, 1.f);

  float time = get_time_msec() * 0.001f;

  // Built-in uniforms: c0 = (time, opacity, res_x, res_y), c1 = (cursor_x, cursor_y, 0, 0)
  Color4 c0(time, p->cachedOpacity, cbs.size.x, cbs.size.y);
  Color4 c1(-1, -1, 0, 0);
  float mx = (p->cursorPos.x - cbs.pos.x) / cbs.size.x;
  float my = (p->cursorPos.y - cbs.pos.y) / cbs.size.y;
  if (mx >= 0.f && mx <= 1.f && my >= 0.f && my <= 1.f)
  {
    c1.r = mx;
    c1.g = my;
  }

  if (p->rtProgram != BAD_PROGRAM)
  {
    // Dev path: runtime-compiled shader, set constants directly
    d3d::set_ps_const(0, &c0, 1);
    d3d::set_ps_const(1, &c1, 1);
    d3d::set_ps_const(2, &p->params[0], 4);

    d3d::set_program(p->rtProgram);
    d3d::setvsrc(0, nullptr, 0);
    d3d::draw(PRIM_TRILIST, 0, 1);
  }
  else if (p->shaderElem.get())
  {
    // Production path: set shader globals, render via ShaderElement
    ShaderGlobal::set_float4(darg_frame_params_varId, c0);
    ShaderGlobal::set_float4(darg_cursor_params_varId, c1);
    ShaderGlobal::set_float4(darg_params0_varId, p->params[0]);
    ShaderGlobal::set_float4(darg_params1_varId, p->params[1]);
    ShaderGlobal::set_float4(darg_params2_varId, p->params[2]);
    ShaderGlobal::set_float4(darg_params3_varId, p->params[3]);

    d3d::setvsrc(0, nullptr, 0);
    p->shaderElem->render(0, 0, RELEM_NO_INDEX_BUFFER, 1, 0, PRIM_TRILIST);
  }
}


//
// RenderObjectShader
//

void RenderObjectShader::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjShaderParams *p = static_cast<RobjShaderParams *>(rdata->params);
  G_ASSERT_RETURN(p, );

  if (p->rtProgram == BAD_PROGRAM && !p->shaderElem.get())
  {
    // Error indicator: semi-transparent magenta quad (premultiplied alpha)
    ctx.set_alpha_blend(PREMULTIPLIED);
    ctx.set_color(E3DCOLOR(80, 0, 80, 80));
    ctx.reset_textures();
    ctx.render_box(rdata->pos, rdata->pos + rdata->size);
    return;
  }

  p->cachedOpacity = render_state.opacity;
  GuiScene *scene = GuiScene::get_from_elem(elem);
  p->cursorPos = scene->activePointerPos();
  ctx.execCommand(DARG_SHADER_CMD, rdata->pos, rdata->size, shader_render_callback, uintptr_t(p));
}


ROBJ_FACTORY_IMPL(RenderObjectShader, RobjShaderParams)

void register_shader_rendobj_factory() { add_rendobj_factory("ROBJ_SHADER", ROBJ_FACTORY_PTR(RenderObjectShader)); }


} // namespace darg
