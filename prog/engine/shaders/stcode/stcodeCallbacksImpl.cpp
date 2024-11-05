// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stcodeCallbacksImpl.h"
#include <shaders/dag_stcode.h>
#include <shaders/dag_shaders.h>

#include "compareStcode.h"
#include "../shadersBinaryData.h"

#include <3d/dag_render.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <3d/fileTexFactory.h>
#include <3d/dag_texIdSet.h>
#include <math/dag_TMatrix4more.h>
#include <EASTL/type_traits.h>
#include <debug/dag_assert.h>

// These are callbacks for cpp stcode to interact with d3d,
// this table is passed into the dll on intialization

namespace stcode::cpp
{

#define CB_VISIBILITY static

CB_VISIBILITY void set_tex(int stage, uint reg, unsigned id);
CB_VISIBILITY void set_buf(int stage, uint reg, unsigned id);
CB_VISIBILITY void set_const_buf(int stage, uint reg, unsigned id);
CB_VISIBILITY void set_sampler(int stage, uint reg, void *handle_ptr);
CB_VISIBILITY void set_rwtex(int stage, uint reg, unsigned id);
CB_VISIBILITY void set_rwbuf(int stage, uint reg, unsigned id);
CB_VISIBILITY void set_tlas(int stage, uint reg, void *ptr);
CB_VISIBILITY void get_globtm(float4x4 *out);
CB_VISIBILITY void get_projtm(float4x4 *out);
CB_VISIBILITY void get_viewprojtm(float4x4 *out);
CB_VISIBILITY void get_lview(int component, float4 *out);
CB_VISIBILITY void get_lworld(int component, float4 *out);
CB_VISIBILITY float4 get_tex_dim(unsigned int id, int mip);
CB_VISIBILITY int get_buf_size(unsigned int id);
CB_VISIBILITY float4 get_viewport();
CB_VISIBILITY int exists_tex(unsigned int id);
CB_VISIBILITY int exists_buf(unsigned int id);
CB_VISIBILITY uint64_t request_sampler(int smp_id, float4 border_color, int anisotropic_max, int mipmap_bias);
CB_VISIBILITY void set_const(int stage, unsigned int id, float4 *val, int cnt);
CB_VISIBILITY void *get_shadervar_ptr_from_dump(int var_id);

// @TODO: remake buffer callbacks to take Sbuffer * directly

G_STATIC_ASSERT((eastl::is_same_v<decltype(request_sampler(0, {}, 0, 0)), eastl::underlying_type_t<d3d::SamplerHandle>>));

// clang-format off
const CallbackTable cb_table_impl =
{
  &set_tex,
  &set_buf,
  &set_const_buf,
  &set_sampler,
  &set_rwtex,
  &set_rwbuf,
  &set_tlas,
  &get_globtm,
  &get_projtm,
  &get_viewprojtm,
  &get_lview,
  &get_lworld,
  &get_shader_global_time_phase,
  &get_tex_dim,
  &get_buf_size,
  &get_viewport,
  &exists_tex,
  &exists_buf,
  &request_sampler,
  &set_const,
  &get_shadervar_ptr_from_dump
};
// clang-format on

static void on_before_resource_used_cb_stub(const D3dResource *, const char *) {}
OnBeforeResourceUsedCb on_before_resource_used_cb = &on_before_resource_used_cb_stub;

const char *last_resource_name = nullptr;
int req_tex_level = 15;

shaders_internal::ConstSetter *custom_const_setter = nullptr;

#if VALIDATE_CPP_STCODE
#define BEGIN_SETTER_BODY()                   \
  if (execution_mode() == ExecutionMode::CPP) \
  {
#define END_SETTER_BODY() }
#else
#define BEGIN_SETTER_BODY(...)
#define END_SETTER_BODY(...)
#endif


CB_VISIBILITY void set_tex(int stage, uint reg, unsigned int id)
{
  TEXTUREID tid = (TEXTUREID)id;
  BaseTexture *tex = D3dResManagerData::getBaseTex(tid);

  BEGIN_SETTER_BODY()
  mark_managed_tex_lfu(tid, req_tex_level);
  on_before_resource_used_cb(tex, last_resource_name);
  d3d::set_tex(stage, reg, tex);
  END_SETTER_BODY()

  dbg::record_set_tex(dbg::RecordType::CPP, stage, reg, tex);
}

CB_VISIBILITY void set_buf(int stage, uint reg, unsigned int id)
{
  D3DRESID bufId = (D3DRESID)id;
  Sbuffer *buf = (Sbuffer *)D3dResManagerData::getD3dRes(bufId);

  BEGIN_SETTER_BODY()
  on_before_resource_used_cb(buf, last_resource_name);
  d3d::set_buffer(stage, reg, buf);
  END_SETTER_BODY()

  dbg::record_set_buf(dbg::RecordType::CPP, stage, reg, buf);
}

CB_VISIBILITY void set_const_buf(int stage, uint reg, unsigned int id)
{
  D3DRESID bufId = (D3DRESID)id;
  Sbuffer *buf = (Sbuffer *)D3dResManagerData::getD3dRes(bufId);

  BEGIN_SETTER_BODY()
  on_before_resource_used_cb(buf, last_resource_name);
  d3d::set_const_buffer(stage, reg, buf);
  END_SETTER_BODY()

  dbg::record_set_const_buf(dbg::RecordType::CPP, stage, reg, buf);
}

CB_VISIBILITY void set_sampler(int stage, uint reg, void *handle_ptr)
{
  d3d::SamplerHandle smp = *(d3d::SamplerHandle *)handle_ptr;

  BEGIN_SETTER_BODY()
  if (smp != d3d::INVALID_SAMPLER_HANDLE)
    d3d::set_sampler(stage, reg, smp);
  END_SETTER_BODY()

  dbg::record_set_sampler(dbg::RecordType::CPP, stage, reg, smp);
}

CB_VISIBILITY void set_rwtex(int stage, uint reg, unsigned int id)
{
  TEXTUREID tid = (TEXTUREID)id;
  BaseTexture *tex = D3dResManagerData::getBaseTex(tid);

  BEGIN_SETTER_BODY()
  on_before_resource_used_cb(tex, last_resource_name);
  d3d::set_rwtex(stage, reg, tex, 0, 0);
  END_SETTER_BODY()

  dbg::record_set_rwtex(dbg::RecordType::CPP, stage, reg, tex);
}

CB_VISIBILITY void set_rwbuf(int stage, uint reg, unsigned int id)
{
  D3DRESID bufId = (D3DRESID)id;
  Sbuffer *buf = (Sbuffer *)D3dResManagerData::getD3dRes(bufId);

  BEGIN_SETTER_BODY()
  on_before_resource_used_cb(buf, last_resource_name);
  d3d::set_rwbuffer(stage, reg, buf);
  END_SETTER_BODY()

  dbg::record_set_rwbuf(dbg::RecordType::CPP, stage, reg, buf);
}

CB_VISIBILITY void set_tlas(int stage, uint reg, void *ptr)
{
  auto *tlas = *(RaytraceTopAccelerationStructure **)ptr;

#if D3D_HAS_RAY_TRACING
  BEGIN_SETTER_BODY()
  d3d::set_top_acceleration_structure(ShaderStage(stage), reg, tlas);
  END_SETTER_BODY()
#else
  logerr("exec_stcode: SHCOD_TLAS ignored for shader ??"); // @TODO: make current shader data visible
#endif

  stcode::dbg::record_set_tlas(stcode::dbg::RecordType::CPP, stage, reg, tlas);
}

CB_VISIBILITY void get_globtm(float4x4 *out)
{
  d3d::getglobtm(*out);
  process_tm_for_drv_consts(*out);
}

CB_VISIBILITY void get_projtm(float4x4 *out)
{
  d3d::gettm(TM_PROJ, out);
  process_tm_for_drv_consts(*out);
}

CB_VISIBILITY void get_viewprojtm(float4x4 *out)
{
  float4x4 v, p;
  d3d::gettm(TM_VIEW, &v);
  d3d::gettm(TM_PROJ, &p);
  *out = v * p;
  process_tm_for_drv_consts(*out);
}

CB_VISIBILITY void get_lview(int component, float4 *out)
{
  const vec4f *tm_lview_c = &d3d::gettm_cref(TM_VIEW2LOCAL).col0;
  memcpy(out, &tm_lview_c[component], sizeof(*out));
}

CB_VISIBILITY void get_lworld(int component, float4 *out)
{
  const vec4f *tm_world_c = &d3d::gettm_cref(TM_WORLD).col0;
  memcpy(out, &tm_world_c[component], sizeof(*out));
}

CB_VISIBILITY float4 get_tex_dim(unsigned int id, int mip)
{
  TEXTUREID tid = (TEXTUREID)id;
  auto tex = acquire_managed_tex(tid);
  TextureInfo info;
  if (tex)
  {
    tex->getinfo(info, mip);
    switch (info.resType)
    {
      case RES3D_ARRTEX: info.d = info.a; break;
      case RES3D_CUBEARRTEX: info.d = info.a / 6; break;
    }
  }
  else
  {
    info.w = info.h = info.d = info.mipLevels = 0; //-V1048
  }
  float4 dims(info.w, info.h, info.d, info.mipLevels);
  release_managed_tex(tid);
  return dims;
}

CB_VISIBILITY int get_buf_size(unsigned int id)
{
  D3DRESID bufId = (D3DRESID)id;
  Sbuffer *buf = (Sbuffer *)D3dResManagerData::getD3dRes(bufId);
  int size = 0;
  if (buf)
    size = buf->getNumElements();
  return size;
}

CB_VISIBILITY float4 get_viewport()
{
  int x = 0, y = 0, w = 0, h = 0;
  float zn = 0, zf = 0;
  d3d::getview(x, y, w, h, zn, zf);
  return float4(x, y, w, h);
}

CB_VISIBILITY int exists_tex(unsigned int id)
{
  TEXTUREID tid = (TEXTUREID)id;
  bool res = acquire_managed_tex(tid) != nullptr;
  release_managed_tex(tid);
  return res;
}

CB_VISIBILITY int exists_buf(unsigned int id)
{
  D3DRESID bufId = (D3DRESID)id;
  return D3dResManagerData::getD3dRes(bufId) != nullptr;
}

CB_VISIBILITY uint64_t request_sampler(int smp_id, float4 border_color, int anisotropic_max, int mipmap_bias)
{
  auto *dump = shBinDumpOwner().getDumpV3();
  G_ASSERTF(dump, "Used incompatible version of shader dump for call 'request_sampler' intrinsic");
  d3d::SamplerInfo info = dump->samplers[smp_id];

  uint32_t rgb = border_color.r || border_color.g || border_color.b ? 0xFFFFFF : 0;
  uint32_t a = border_color.a ? 0xFF000000 : 0;
  info.border_color = static_cast<d3d::BorderColor::Color>(a | rgb);
  info.anisotropic_max = anisotropic_max;
  info.mip_map_bias = mipmap_bias;

  d3d::SamplerHandle handle = d3d::request_sampler(info);

  dbg::record_request_sampler(dbg::RecordType::CPP, info);
  return eastl::to_underlying(handle);
}

CB_VISIBILITY void set_const(int stage, unsigned int id, float4 *val, int cnt)
{
  BEGIN_SETTER_BODY()
  if (custom_const_setter)
    custom_const_setter->setConst((ShaderStage)stage, id, val, cnt);
  else
    d3d::set_const(stage, id, val, cnt);
  END_SETTER_BODY()

  dbg::record_set_const(dbg::RecordType::CPP, stage, id, val, cnt);
}

CB_VISIBILITY void *get_shadervar_ptr_from_dump(int var_id)
{
  const auto &var = shBinDump().globVars.v[var_id];

  // @TODO: are these guaranteed to be non-relocatable?
  if (var.type == SHVT_TEXTURE)
  {
    shaders_internal::Tex &tex = (shaders_internal::Tex &)var.valPtr.get();
    return (void *)&tex.texId;
  }
  else if (var.type == SHVT_BUFFER)
  {
    shaders_internal::Buf &buf = (shaders_internal::Buf &)var.valPtr.get();
    return (void *)&buf.bufId;
  }
  else
    return (void *)&var.valPtr.get();
}

CB_VISIBILITY real get_shader_global_time_phase(float period, float offset) { return ::get_shader_global_time_phase(period, offset); }

#undef CB_VISIBILITY

} // namespace stcode::cpp
