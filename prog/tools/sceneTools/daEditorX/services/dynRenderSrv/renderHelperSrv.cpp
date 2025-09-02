// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_dynRenderService.h>
#include <de3_interface.h>
#include <render/dag_cur_view.h>
#include <perfMon/dag_graphStat.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_renderStateId.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_platform_pc.h>
#include <math/dag_TMatrix4more.h>
#include <stdio.h>

class GenericRenderHelperService : public IRenderHelperService
{
  struct ShaderBufData
  {
    PROGRAM prog = BAD_PROGRAM;
    Vbuffer *vb = nullptr;
    Ibuffer *ib = nullptr;

    bool inited() { return prog != BAD_PROGRAM; }
    void clear()
    {
      del_d3dres(vb);
      del_d3dres(ib);
      d3d::delete_program(prog);
      prog = BAD_PROGRAM;
    }
  };
  ShaderBufData cubeData, voltexData, bkgData;
  shaders::OverrideStateId noCullStateId;
  shaders::RenderStateId defaultRenderStateId = shaders::RenderStateId::Invalid;
  d3d::SamplerHandle clampSampler = d3d::INVALID_SAMPLER_HANDLE;
  bool pendingPerfDump = false;

public:
  void init() override
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::CULL_NONE);
    noCullStateId = shaders::overrides::create(state);
    defaultRenderStateId = shaders::render_states::create(shaders::RenderState());

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampSampler = d3d::request_sampler(smpInfo);
  }
  void term() override
  {
    cubeData.clear();
    bkgData.clear();
    voltexData.clear();
    shaders::overrides::destroy(noCullStateId);
  }

  void renderEnviCubeTexture(BaseTexture *tex, const Color4 &cMul, const Color4 &cAdd) override
  {
    if (!tex)
      return;
    if (!cubeData.inited())
      prepareCubeData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);
    d3d::set_sampler(STAGE_PS, 0, clampSampler);

    TMatrix cubeWtm;
    cubeWtm.identity();
    cubeWtm.setcol(3, ::grs_cur_view.pos);
    d3d::settm(TM_WORLD, cubeWtm);

    TMatrix4_vec4 gtm;
    d3d::getglobtm(gtm);
    process_tm_for_drv_consts(gtm);
    d3d::set_vs_const(0, gtm[0], 4);
    d3d::set_ps_const1(0, cMul.r, cMul.g, cMul.b, cMul.a);
    d3d::set_ps_const1(1, cAdd.r, cAdd.g, cAdd.b, cAdd.a);

    d3d::set_program(cubeData.prog);
    d3d::setvsrc(0, cubeData.vb, sizeof(Point3));
    d3d::setind(cubeData.ib);
    d3d::drawind(PRIM_TRILIST, 0, 12, 0);

    d3d::setvdecl(BAD_VDECL);
    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    d3d::set_sampler(STAGE_PS, 0, d3d::INVALID_SAMPLER_HANDLE);
    shaders::overrides::reset();
  }
  void renderEnviBkgTexture(BaseTexture *tex, d3d::SamplerHandle smp, const Color4 &scales) override
  {
    if (!tex)
      return;
    if (bkgData.prog == BAD_PROGRAM)
      prepareBkgData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    d3d::set_vs_const1(0, scales.r, scales.g, scales.b, scales.a);
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);
    d3d::set_sampler(STAGE_PS, 0, smp);

    d3d::set_program(bkgData.prog);
    d3d::setvsrc(0, bkgData.vb, sizeof(Point2));
    d3d::draw(PRIM_TRISTRIP, 0, 2);

    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    d3d::set_sampler(STAGE_PS, 0, d3d::INVALID_SAMPLER_HANDLE);
    shaders::overrides::reset();
  }
  void renderEnviVolTexture(BaseTexture *tex, d3d::SamplerHandle smp, //
    const Color4 &cMul, const Color4 &cAdd, const Color4 &scales, float tz) override
  {
    if (!tex)
      return;
    if (voltexData.prog == BAD_PROGRAM)
      prepareVolTexData();

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // since we change render states and constants/samplers
    d3d::set_ps_const1(0, cMul.r, cMul.g, cMul.b, cMul.a);
    d3d::set_ps_const1(1, cAdd.r, cAdd.g, cAdd.b, cAdd.a);
    d3d::set_ps_const1(2, tz, 0, 0, 0);
    d3d::set_vs_const1(0, scales.r, scales.g, scales.b, scales.a);
    shaders::overrides::set(noCullStateId);
    shaders::render_states::set(defaultRenderStateId);
    d3d::settex(0, tex);
    d3d::set_sampler(STAGE_PS, 0, smp);

    d3d::set_program(voltexData.prog);
    d3d::setvsrc(0, voltexData.vb, sizeof(Point2));
    d3d::draw(PRIM_TRISTRIP, 0, 2);

    d3d::set_program(BAD_PROGRAM);
    d3d::settex(0, NULL);
    d3d::set_sampler(STAGE_PS, 0, d3d::INVALID_SAMPLER_HANDLE);
    shaders::overrides::reset();
  }

  void enableFrameProfiler(bool enable) override
  {
    if (enable)
      da_profiler::add_mode(da_profiler::EVENTS | da_profiler::GPU);
    else
      da_profiler::remove_mode(da_profiler::EVENTS | da_profiler::GPU);
    DAEDITOR3.conNote("Time profiler %s", enable ? "ON" : "OFF");
  }
  void profilerDumpFrame() override
  {
    if ((da_profiler::get_active_mode() & da_profiler::EVENTS))
      pendingPerfDump = true;
  }
  void dumpPendingProfilerDumpOnFrameEnd() override
  {
    if (!pendingPerfDump || !(da_profiler::get_active_mode() & da_profiler::EVENTS))
      return;

    DAEDITOR3.conNote("=======Frame Log========");
    DAEDITOR3.conNote("name;calls;avFrameCnt;cpuMin;cpuMax;cpuSpike;cpuOwn;gpuMin;gpuMax;gpuSpike;"
                      "dip;tri;rt;prog;ins;lockivb;skipped");
    da_profiler::dump_frames(dump_ingame_profiler_cb, NULL);
    pendingPerfDump = false;
  }

protected:
  void prepareCubeData()
  {
#if _TARGET_PC_WIN // TODO: tools Linux porting: !!! prepareCubeData !!!
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float3 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float3 tc:TEXCOORD0; };\n"
                                 "float4x4 globTm: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = mul(float4(input.pos, 1), globTm); output.pos.z = 0;\n"
                                 "  output.tc  = input.pos;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput { float4 pos:POSITION; float3 tc:TEXCOORD0; };\n"
                                 "samplerCUBE tex : register(s0);\n"
                                 "float4 cMul: register(c0);\n"
                                 "float4 cAdd: register(c1);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 {\n"
                                 "  float4 c = texCUBE(tex, input.tc);\n"
                                 "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                 "}\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float3 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float3 tc:TEXCOORD0; };\n"
                                   "float4x4 globTm: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = mul(float4(input.pos, 1), globTm); output.pos.z = 0;\n"
                                   "  output.tc  = input.pos;\n"
                                   "  return output;\n"
                                   "}\n";
    static const char *ps_hlsl11 = "struct VSOutput { float4 pos:SV_POSITION; float3 tc:TEXCOORD0; };\n"
                                   "TextureCube tex: register(t0);\n"
                                   "SamplerState tex_samplerstate : register(s0);\n"
                                   "float4 cMul: register(c0);\n"
                                   "float4 cAdd: register(c1);\n"
                                   "float4 ps_main(VSOutput input):SV_Target0 {\n"
                                   "  float4 c = tex.Sample(tex_samplerstate, input.tc);\n"
                                   "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                   "}\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    cubeData.prog = d3d::create_program(vs, ps, vd);
    d3d::set_program(cubeData.prog);

    cubeData.vb = d3d::create_vb(sizeof(Point3) * 8, 0);
    cubeData.ib = d3d::create_ib(sizeof(short) * 3 * 12, 0);
    debug("cubeData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p ib=%p", cubeData.prog, vs, ps, vd, cubeData.vb, cubeData.ib);

    {
      Point3 *vp;
      d3d_err(cubeData.vb->lockEx(0, 0, &vp, VBLOCK_WRITEONLY));
      vp[0].set(-0.5, -0.5, -0.5);
      vp[1].set(+0.5, -0.5, -0.5);
      vp[2].set(-0.5, +0.5, -0.5);
      vp[3].set(+0.5, +0.5, -0.5);
      vp[4].set(-0.5, -0.5, +0.5);
      vp[5].set(+0.5, -0.5, +0.5);
      vp[6].set(-0.5, +0.5, +0.5);
      vp[7].set(+0.5, +0.5, +0.5);
      d3d_err(cubeData.vb->unlock());
    }
    {
      unsigned short *ip;
      d3d_err(cubeData.ib->lock(0, 0, &ip, VBLOCK_WRITEONLY));
      int ind = 0;
      ip[ind++] = 0;
      ip[ind++] = 1;
      ip[ind++] = 2;
      ip[ind++] = 1;
      ip[ind++] = 3;
      ip[ind++] = 2;

      ip[ind++] = 4;
      ip[ind++] = 5;
      ip[ind++] = 6;
      ip[ind++] = 5;
      ip[ind++] = 7;
      ip[ind++] = 6;

      ip[ind++] = 0;
      ip[ind++] = 4;
      ip[ind++] = 2;
      ip[ind++] = 2;
      ip[ind++] = 4;
      ip[ind++] = 6;

      ip[ind++] = 1;
      ip[ind++] = 3;
      ip[ind++] = 5;
      ip[ind++] = 3;
      ip[ind++] = 7;
      ip[ind++] = 5;

      ip[ind++] = 0;
      ip[ind++] = 4;
      ip[ind++] = 1;
      ip[ind++] = 1;
      ip[ind++] = 4;
      ip[ind++] = 5;

      ip[ind++] = 2;
      ip[ind++] = 6;
      ip[ind++] = 3;
      ip[ind++] = 3;
      ip[ind++] = 6;
      ip[ind++] = 7;

      d3d_err(cubeData.ib->unlock());
    }
#else
    LOGERR_CTX("TODO: tools Linux porting: prepareCubeData");
#endif
  }

  void prepareBkgData()
  {
#if _TARGET_PC_WIN // TODO: tools Linux porting: !!! prepareBkgData !!!
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float2 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "float4 tcMA: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = float4(input.pos, 0, 1);\n"
                                 "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "sampler tex : register(s0);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 { return float4(tex2D(tex, input.tc).rgb, 1); }\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float2 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "float4 tcMA: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = float4(input.pos, 0, 1);\n"
                                   "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                   "  return output;\n"
                                   "}\n";

    static const char *ps_hlsl11 =
      "struct VSOutput{float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
      "Texture2D tex: register(t0);\n"
      "SamplerState tex_samplerstate : register(s0);\n"
      "float4 ps_main(VSOutput input):SV_Target0 { return float4(tex.Sample(tex_samplerstate, input.tc).rgb, 1); }\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    bkgData.prog = d3d::create_program(vs, ps, vd);

    bkgData.vb = d3d::create_vb(sizeof(Point2) * 4, 0);
    debug("bkgData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p", bkgData.prog, vs, ps, vd, bkgData.vb);

    {
      Point2 *vp;
      d3d_err(bkgData.vb->lock(0, 0, (void **)&vp, VBLOCK_WRITEONLY));
      vp[0].set(-1, -1);
      vp[1].set(-1, +1);
      vp[2].set(+1, -1);
      vp[3].set(+1, +1);
      d3d_err(bkgData.vb->unlock());
    }
#else
    LOGERR_CTX("TODO: tools Linux porting: prepareBkgData");
#endif
  }

  void prepareVolTexData()
  {
#if _TARGET_PC_WIN // TODO: tools Linux porting: !!! prepareVolTexData !!!
    // DX9 HLSL
    static const char *vs_hlsl = "struct VSInput  { float2 pos:POSITION; };\n"
                                 "struct VSOutput { float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "float4 tcMA: register(c0);\n"
                                 "VSOutput vs_main(VSInput input)\n"
                                 "{\n"
                                 "  VSOutput output;\n"
                                 "  output.pos = float4(input.pos, 0, 1);\n"
                                 "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                 "  return output;\n"
                                 "}\n";
    static const char *ps_hlsl = "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
                                 "sampler3D tex : register(s0);\n"
                                 "float4 cMul: register(c0);\n"
                                 "float4 cAdd: register(c1);\n"
                                 "float4 cTcZ: register(c2);\n"
                                 "float4 ps_main(VSOutput input):COLOR0 {\n"
                                 "  float4 c = tex3D(tex, float3(input.tc, cTcZ.x));\n"
                                 "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                 "}\n";

    // DX11 HLSL
    static const char *vs_hlsl11 = "struct VSInput  { float2 pos:POSITION; };\n"
                                   "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "float4 tcMA: register(c0);\n"
                                   "VSOutput vs_main(VSInput input)\n"
                                   "{\n"
                                   "  VSOutput output;\n"
                                   "  output.pos = float4(input.pos, 0, 1);\n"
                                   "  output.tc  = input.pos*tcMA.xy+tcMA.zw;\n"
                                   "  return output;\n"
                                   "}\n";

    static const char *ps_hlsl11 = "struct VSOutput { float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
                                   "Texture3D tex: register(t0);\n"
                                   "SamplerState tex_samplerstate : register(s0);\n"
                                   "float4 cMul: register(c0);\n"
                                   "float4 cAdd: register(c1);\n"
                                   "float4 cTcZ: register(c2);\n"
                                   "float4 ps_main(VSOutput input):SV_Target0 {\n"
                                   "  float4 c = tex.Sample(tex_samplerstate, float3(input.tc, cTcZ.x));\n"
                                   "  return c*cMul+float4(c.aaa, 1)*cAdd;\n"
                                   "}\n";

    static VSDTYPE dcl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END};

    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" : "ps_3_0");
    VDECL vd = d3d::create_vdecl(dcl);
    voltexData.prog = d3d::create_program(vs, ps, vd);

    voltexData.vb = d3d::create_vb(sizeof(Point2) * 4, 0);
    debug("voltexData: prog=%d, vs=%d, ps=%d, vd=%d vb=%p", voltexData.prog, vs, ps, vd, voltexData.vb);

    {
      Point2 *vp;
      d3d_err(voltexData.vb->lock(0, 0, (void **)&vp, VBLOCK_WRITEONLY));
      vp[0].set(-1, -1);
      vp[1].set(-1, +1);
      vp[2].set(+1, -1);
      vp[3].set(+1, +1);
      d3d_err(voltexData.vb->unlock());
    }
#else
    LOGERR_CTX("TODO: tools Linux porting: prepareVolTexData");
#endif
  }
  static void dump_ingame_profiler_cb(void *, uintptr_t cNode, uintptr_t pNode, const char *name, const TimeIntervalInfo &ti)
  {
    char gpuinterval[64], gpuData[128];
    if (ti.gpuValid)
    {
      SNPRINTF(gpuinterval, sizeof(gpuinterval), "%.2f;%.2f;%.2f", ti.tGpuMin, ti.tGpuMax, ti.tGpuSpike);
      SNPRINTF(gpuData, sizeof(gpuData), "%d;%d;%d;%d;%d;%d", ti.stat.val[DRAWSTAT_DP], (int)ti.stat.tri, ti.stat.val[DRAWSTAT_RT],
        ti.stat.val[DRAWSTAT_PS], ti.stat.val[DRAWSTAT_INS], ti.stat.val[DRAWSTAT_LOCKVIB]);
    }
    else
    {
      SNPRINTF(gpuinterval, sizeof(gpuinterval), ";;");
      SNPRINTF(gpuData, sizeof(gpuData), ";;;;;;;;;");
    }

    char skipped[16];
    if (ti.skippedFrames)
      SNPRINTF(skipped, sizeof(skipped), "%d", ti.skippedFrames);
    else
      skipped[0] = 0;
    char averageFrameCnt[32];
    double avFrames = ((double)ti.lifetimeCalls) / max(1u, ti.lifetimeFrames);
    if (avFrames <= 0.99)
      SNPRINTF(averageFrameCnt, sizeof(averageFrameCnt), "%.2f", avFrames);
    else
      averageFrameCnt[0] = 0;

    DAEDITOR3.conNote("%*s%s;%d;%s;%.2f;%.2f;%.2f;%.2f;%s;%s", min<int>(ti.depth, 16) * 4, "", name, ti.perFrameCalls, averageFrameCnt,
      ti.tMin, ti.tMax, ti.tSpike, ti.unknownTimeMsec, gpuinterval, gpuData, skipped);
  }
};
static GenericRenderHelperService srv;

void *get_generic_render_helper_service() { return &srv; }
