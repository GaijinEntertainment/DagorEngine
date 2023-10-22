//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>

#if _TARGET_D3D_MULTI
#include <3d/dag_drv3di.h>

class String;
class D3dEventQuery;

namespace d3d
{
typedef D3dEventQuery EventQuery;

bool init_driver();
bool is_inited();

bool init_video(void *hinst, main_wnd_f *f, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

void release_driver();
bool fill_interface_table(D3dInterfaceTable &d3dit);
void prepare_for_destroy();
void window_destroyed(void *hwnd);

DriverCode get_driver_code();
static inline bool is_stub_driver() { return d3di.driverCode.is(d3d::stub); }

static inline const char *get_driver_name() { return d3di.driverName; }
static inline const char *get_device_driver_version() { return d3di.driverVer; }
static inline const char *get_device_name() { return d3di.deviceName; }
static inline const char *get_last_error() { return d3di.get_last_error(); }
static inline uint32_t get_last_error_code() { return d3di.get_last_error_code(); }

static inline void *get_device() { return d3di.get_device(); }

static inline const Driver3dDesc &get_driver_desc() { return d3di.drvDesc; }

static inline int driver_command(int cmd, void *p1, void *p2, void *p3) { return d3di.driver_command(cmd, p1, p2, p3); }

static inline bool device_lost(bool *can_reset_now) { return d3di.device_lost(can_reset_now); }
static inline bool reset_device() { return d3di.reset_device(); }
static inline bool is_in_device_reset_now() { return d3di.is_in_device_reset_now(); }
static inline bool is_window_occluded() { return d3di.is_window_occluded(); }

static inline bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats)
{
  return d3di.should_use_compute_for_image_processing(formats);
}

static inline bool check_texformat(int cflg) { return d3di.check_texformat(cflg); }
static inline unsigned get_texformat_usage(int cflg, int restype = RES3D_TEX) { return d3di.get_texformat_usage(cflg, restype); }
static inline bool issame_texformat(int cflg1, int cflg2) { return d3di.issame_texformat(cflg1, cflg2); }
static inline bool check_cubetexformat(int cflg) { return d3di.check_cubetexformat(cflg); }
static inline bool issame_cubetexformat(int cflg1, int cflg2) { return d3di.issame_cubetexformat(cflg1, cflg2); }
static inline bool check_voltexformat(int cflg) { return d3di.check_voltexformat(cflg); }
static inline bool issame_voltexformat(int cflg1, int cflg2) { return d3di.issame_voltexformat(cflg1, cflg2); }

static inline BaseTexture *create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.create_tex(img, w, h, flg, levels, stat_name);
}
static inline BaseTexture *create_cubetex(int size, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.create_cubetex(size, flg, levels, stat_name);
}
static inline BaseTexture *create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.create_voltex(w, h, d, flg, levels, stat_name);
}
static inline BaseTexture *create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.create_array_tex(w, h, d, flg, levels, stat_name);
}
static inline BaseTexture *create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.create_cube_array_tex(side, d, flg, levels, stat_name);
}

static inline BaseTexture *create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels = 0, const char *stat_name = NULL)
{
  return d3di.create_ddsx_tex(crd, flg, quality_id, levels, stat_name);
}

static inline BaseTexture *alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels = 0,
  const char *stat_name = NULL, int stub_tex_idx = -1)
{
  return d3di.alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name, stub_tex_idx);
}
static inline bool load_ddsx_tex_contents(BaseTexture *t, const ddsx::Header &hdr, IGenLoad &crd, int q_id)
{
  return d3d_load_ddsx_tex_contents ? d3d_load_ddsx_tex_contents(t, t->getTID(), BAD_TEXTUREID, hdr, crd, q_id, 0, 0) : false;
}

static inline BaseTexture *alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels,
  const char *stat_name = NULL)
{
  return d3di.alias_tex(baseTexture, img, w, h, flg, levels, stat_name);
}

static inline BaseTexture *alias_cubetex(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name = NULL)
{
  return d3di.alias_cubetex(baseTexture, size, flg, levels, stat_name);
}

static inline BaseTexture *alias_voltex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels,
  const char *stat_name = NULL)
{
  return d3di.alias_voltex(baseTexture, w, h, d, flg, levels, stat_name);
}

static inline BaseTexture *alias_array_tex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return d3di.alias_array_tex(baseTexture, w, h, d, flg, levels, stat_name);
}

static inline BaseTexture *alias_cube_array_tex(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name)
{
  return d3di.alias_cube_array_tex(baseTexture, side, d, flg, levels, stat_name);
}

static inline bool set_tex_usage_hint(int w, int h, int mips, const char *format, unsigned int tex_num)
{
  return d3di.set_tex_usage_hint(w, h, mips, format, tex_num);
}

static inline bool stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc = NULL, RectInt *rdst = NULL)
{
  return d3di.stretch_rect(src, dst, rsrc, rdst);
}
static inline bool copy_from_current_render_target(BaseTexture *to_tex) { return d3di.copy_from_current_render_target(to_tex); }

static inline void discard_managed_textures() { return d3di.discard_managed_textures(); }
static inline void get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  d3di.get_texture_statistics(num_textures, total_mem, out_dump);
}

static inline SamplerHandle create_sampler(const SamplerInfo &sampler_info) { return d3di.create_sampler(sampler_info); }
static inline void destroy_sampler(SamplerHandle sampler) { d3di.destroy_sampler(sampler); }

static inline void set_sampler(unsigned shader_stage, unsigned slot, SamplerHandle sampler)
{
  d3di.set_sampler(shader_stage, slot, sampler);
}

static inline PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides = 0, unsigned streams = 0)
{
  return d3di.create_program_0(vprog, fsh, vdecl, strides, streams);
}
static inline PROGRAM create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides = 0,
  unsigned streams = 0)
{
  return d3di.create_program_1(vpr_native, fsh_native, vdecl, strides, streams);
}

static inline PROGRAM create_program_cs(const uint32_t *cs_native) { return d3di.create_program_cs(cs_native); }

static inline bool set_program(PROGRAM p) { return d3di.set_program(p); }
static inline void delete_program(PROGRAM p) { return d3di.delete_program(p); }

static inline VPROG create_vertex_shader(const uint32_t *native_code) { return d3di.create_vertex_shader(native_code); }
static inline void delete_vertex_shader(VPROG vs) { return d3di.delete_vertex_shader(vs); }

static inline bool set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs)
{
  return d3di.set_const(stage, reg_base, data, num_regs);
}
static inline bool set_vs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_VS, reg_base, data, num_regs);
}
static inline bool set_ps_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_PS, reg_base, data, num_regs);
}
static inline bool set_cs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_CS, reg_base, data, num_regs);
}
static inline bool set_vs_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_vs_const(reg, v, 1);
}
static inline bool set_ps_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_ps_const(reg, v, 1);
}

static inline bool set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words)
{
  return d3di.set_immediate_const(stage, data, num_words);
}

static inline FSHADER create_pixel_shader(const uint32_t *native_code) { return d3di.create_pixel_shader(native_code); }
static inline void delete_pixel_shader(FSHADER ps) { return d3di.delete_pixel_shader(ps); }

static inline int set_vs_constbuffer_size(int required_size) { return d3di.set_vs_constbuffer_size(required_size); }
static inline int set_cs_constbuffer_size(int required_size) { return d3di.set_cs_constbuffer_size(required_size); }

static inline bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset = 0,
  uint32_t consts_size = 0)
{
  return d3di.set_const_buffer(stage, slot, buffer, consts_offset, consts_size);
}

static inline bool set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler = true)
{
  return d3di.set_tex(shader_stage, slot, tex, use_sampler);
}

static inline bool settex(int slot, BaseTexture *tex) { return set_tex(STAGE_PS, slot, tex); }
static inline bool settex_vs(int slot, BaseTexture *tex) { return set_tex(STAGE_VS, slot, tex); }

static inline uint32_t register_bindless_sampler(BaseTexture *texture) { return d3di.register_bindless_sampler(texture); }

static inline uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  return d3di.allocate_bindless_resource_range(resource_type, count);
}
static inline uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count,
  uint32_t new_count)
{
  return d3di.resize_bindless_resource_range(resource_type, index, current_count, new_count);
}
static inline void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  d3di.free_bindless_resource_range(resource_type, index, count);
}
static inline void update_bindless_resource(uint32_t index, D3dResource *res) { d3di.update_bindless_resource(index, res); }

static inline bool set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip, bool as_uint = false)
{
  return d3di.set_rwtex(shader_stage, slot, tex, face, mip, as_uint);
}
static inline bool clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip)
{
  return d3di.clear_rwtexi(tex, val, face, mip);
}
static inline bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip)
{
  return d3di.clear_rwtexf(tex, val, face, mip);
}
static inline bool clear_rwbufi(Sbuffer *tex, const unsigned val[4]) { return d3di.clear_rwbufi(tex, val); }
static inline bool clear_rwbuff(Sbuffer *tex, const float val[4]) { return d3di.clear_rwbuff(tex, val); }

static inline bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  return d3di.set_buffer(shader_stage, slot, buffer);
}
static inline bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  return d3di.set_rwbuffer(shader_stage, slot, buffer);
}

static inline bool set_cb0_data(unsigned stage, const float *data, unsigned num_regs)
{
  switch (stage)
  {
    case STAGE_CS: return set_cs_const(0, data, num_regs);
    case STAGE_PS: return set_ps_const(0, data, num_regs);
    case STAGE_VS: return set_vs_const(0, data, num_regs);
    default: G_ASSERTF(0, "Stage %d unsupported", stage); return false;
  };
}
static inline void release_cb0_data(unsigned /*stage*/) {}

static inline Sbuffer *create_vb(int sz, int f, const char *name = "")
{
  d3d::validate_sbuffer_flags(f | SBCF_BIND_VERTEX, name);
  return d3di.create_vb(sz, f, name);
}
static inline Sbuffer *create_ib(int size_bytes, int flags, const char *stat_name = "ib")
{
  d3d::validate_sbuffer_flags(flags | SBCF_BIND_INDEX, stat_name);
  return d3di.create_ib(size_bytes, flags, stat_name);
}

static inline Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name = "")
{
  d3d::validate_sbuffer_flags(flags, name);
  return d3di.create_sbuffer(struct_size, elements, flags, texfmt, name);
}

static inline bool set_render_target() { return d3di.set_render_target(); }

static inline bool set_depth(BaseTexture *tex, DepthAccess access) { return d3di.set_depth(tex, access); }
static inline bool set_depth(BaseTexture *tex, int layer, DepthAccess access) { return d3di.set_depth(tex, layer, access); }

static inline bool set_backbuf_depth() { return d3di.set_backbuf_depth(); }

static inline bool set_render_target(int rt_index, BaseTexture *t, int fc, int level)
{
  return d3di.set_render_target(rt_index, t, fc, level);
}
static inline bool set_render_target(int rt_index, BaseTexture *t, int level) { return d3di.set_render_target(rt_index, t, level); }

static inline bool set_render_target(BaseTexture *t, int level) { return set_render_target() && set_render_target(0, t, level); }
static inline bool set_render_target(BaseTexture *t, int fc, int level)
{
  return set_render_target() && set_render_target(0, t, fc, level);
}

static inline void get_render_target(Driver3dRenderTarget &out_rt) { return d3di.get_render_target(out_rt); }
static inline bool set_render_target(const Driver3dRenderTarget &rt) { return d3di.set_render_target(rt); }

static inline bool get_target_size(int &w, int &h) { return d3di.get_target_size(w, h); }
static inline bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev = 0)
{
  return d3di.get_render_target_size(w, h, rt_tex, lev);
}
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors)
{
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (i < colors.size())
      set_render_target(i, colors[i].tex, colors[i].mip_level);
    else
      set_render_target(i, nullptr, 0);
  }
  set_depth(depth.tex, depth_access);
}
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, const std::initializer_list<RenderTarget> colors)
{
  set_render_target(depth, depth_access, dag::ConstSpan<RenderTarget>(colors.begin(), colors.end() - colors.begin()));
}

static inline void set_variable_rate_shading(unsigned rate_x, unsigned rate_y,
  VariableRateShadingCombiner vertex_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH,
  VariableRateShadingCombiner pixel_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH)
{
  d3di.set_variable_rate_shading(rate_x, rate_y, vertex_combiner, pixel_combiner);
}
static inline void set_variable_rate_shading_texture(BaseTexture *rate_texture)
{
  d3di.set_variable_rate_shading_texture(rate_texture);
}

static inline bool settm(int which, const Matrix44 *tm) { return d3di.settm(which, tm); }
static inline bool settm(int which, const TMatrix &tm) { return d3di.settm(which, tm); }
static inline void settm(int which, const mat44f &out_tm) { return d3di.settm(which, out_tm); }

static inline bool gettm(int which, Matrix44 *out_tm) { return d3di.gettm(which, out_tm); }
static inline bool gettm(int which, TMatrix &out_tm) { return d3di.gettm(which, out_tm); }
static inline void gettm(int which, mat44f &out_tm) { return d3di.gettm(which, out_tm); }
static inline const mat44f &gettm_cref(int which) { return d3di.gettm_cref(which); }

static inline void getm2vtm(TMatrix &out_m2v) { return d3di.getm2vtm(out_m2v); }
static inline void getglobtm(Matrix44 &m) { return d3di.getglobtm(m); }
static inline void setglobtm(Matrix44 &m) { return d3di.setglobtm(m); }

static inline void getglobtm(mat44f &m) { return d3di.getglobtm_vec(m); }
static inline void setglobtm(const mat44f &m) { return d3di.setglobtm_vec(m); }

static inline bool setpersp(const Driver3dPerspective &p, TMatrix4 *tm = nullptr) { return d3di.setpersp(p, tm); }

static inline bool getpersp(Driver3dPerspective &p) { return d3di.getpersp(p); }

static inline bool validatepersp(const Driver3dPerspective &p) { return d3di.validatepersp(p); }

static inline bool calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm) { return d3di.calcproj_0(p, proj_tm); }
static inline bool calcproj(const Driver3dPerspective &p, mat44f &proj_tm) { return d3di.calcproj_1(p, proj_tm); }

static inline void calcglobtm(const mat44f &v, const mat44f &p, mat44f &r) { d3di.calcglobtm_0(v, p, r); }
static inline void calcglobtm(const mat44f &v, const Driver3dPerspective &p, mat44f &r) { d3di.calcglobtm_1(v, p, r); }
static inline void calcglobtm(const TMatrix &v, const TMatrix4 &p, TMatrix4 &r) { d3di.calcglobtm_2(v, p, r); }
static inline void calcglobtm(const TMatrix &v, const Driver3dPerspective &p, TMatrix4 &r) { d3di.calcglobtm_3(v, p, r); }

static inline bool setscissor(int x, int y, int w, int h) { return d3di.setscissor(x, y, w, h); }
static inline bool setscissors(dag::ConstSpan<ScissorRect> scissorRects) { return d3di.setscissors(scissorRects); }

static inline bool setview(int x, int y, int w, int h, float minz, float maxz) { return d3di.setview(x, y, w, h, minz, maxz); }
static inline bool setviews(dag::ConstSpan<Viewport> viewports) { return d3di.setviews(viewports); }
static inline bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz) { return d3di.getview(x, y, w, h, minz, maxz); }

static inline bool clearview(int what, E3DCOLOR c, float z, uint32_t stencil) { return d3di.clearview(what, c, z, stencil); }

static inline bool update_screen(bool app_active = true) { return d3di.update_screen(app_active); }

static inline bool setvsrc_ex(int s, Sbuffer *vb, int ofs, int stride_bytes) { return d3di.setvsrc_ex(s, vb, ofs, stride_bytes); }
static inline bool setvsrc(int s, Sbuffer *vb, int stride_bytes) { return setvsrc_ex(s, vb, 0, stride_bytes); }

static inline bool setind(Sbuffer *ib) { return d3di.setind(ib); }

static inline VDECL create_vdecl(VSDTYPE *vsd) { return d3di.create_vdecl(vsd); }
static inline void delete_vdecl(VDECL vdecl) { return d3di.delete_vdecl(vdecl); }
static inline bool setvdecl(VDECL vdecl) { return d3di.setvdecl(vdecl); }

static inline bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.draw_base(type, start, numprim, num_instances, start_instance);
}
inline bool draw(int type, int start, int numprim) { return draw_base(type, start, numprim, 1, 0); }
inline bool draw_instanced(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance = 0)
{
  return draw_base(type, start, numprim, num_instances, start_instance);
}

static inline bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}
inline bool drawind_instanced(int type, int startind, int numprim, int base_vertex, uint32_t num_instances,
  uint32_t start_instance = 0)
{
  return drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}
inline bool drawind(int type, int startind, int numprim, int base_vertex)
{
  return drawind_base(type, startind, numprim, base_vertex, 1, 0);
}

static inline bool draw_up(int type, int numprim, const void *ptr, int stride_bytes)
{
  return d3di.draw_up(type, numprim, ptr, stride_bytes);
}
static inline bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  return d3di.drawind_up(type, minvert, numvert, numprim, ind, ptr, stride_bytes);
}
static inline bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0)
{
  return d3di.draw_indirect(type, args, byte_offset);
}
static inline bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0)
{
  return d3di.draw_indexed_indirect(type, args, byte_offset);
}
static inline bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes,
  uint32_t byte_offset = 0)
{
  return d3di.multi_draw_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}
static inline bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes,
  uint32_t byte_offset = 0)
{
  return d3di.multi_draw_indexed_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}

static inline bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  return d3di.dispatch(thread_group_x, thread_group_y, thread_group_z, gpu_pipeline);
}
static inline bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset = 0, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  return d3di.dispatch_indirect(args, byte_offset, gpu_pipeline);
}

static inline void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  d3di.dispatch_mesh(thread_group_x, thread_group_y, thread_group_z);
}

static inline void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset = 0)
{
  d3di.dispatch_mesh_indirect(args, dispatch_count, stride_bytes, byte_offset);
}

static inline void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  d3di.dispatch_mesh_indirect_count(args, args_stride_bytes, args_byte_offset, count, count_byte_offset, max_count);
}

static inline GPUFENCEHANDLE insert_fence(GpuPipeline gpu_pipeline) { return d3di.insert_fence(gpu_pipeline); }
static inline void insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline)
{
  d3di.insert_wait_on_fence(fence, gpu_pipeline);
}

static inline bool setantialias(int aa_type) { return d3di.setantialias(aa_type); }
static inline int getantialias() { return d3di.getantialias(); }

static inline shaders::DriverRenderStateId create_render_state(const shaders::RenderState &state)
{
  return d3di.create_render_state(state);
}
static inline bool set_render_state(shaders::DriverRenderStateId state_id) { return d3di.set_render_state(state_id); }

static inline void clear_render_states() { d3di.clear_render_states(); }

static inline bool set_blend_factor(E3DCOLOR color) { return d3di.set_blend_factor(color); }
static inline bool setstencil(uint32_t ref) { return d3di.setstencil(ref); }

static inline bool setwire(bool in) { return d3di.setwire(in); }

static inline bool set_srgb_backbuffer_write(bool on) { return d3di.set_srgb_backbuffer_write(on); }

static inline bool set_msaa_pass() { return d3di.set_msaa_pass(); }

static inline bool set_depth_resolve() { return d3di.set_depth_resolve(); }

static inline bool setgamma(float power) { return d3di.setgamma(power); }

static inline bool isVcolRgba() { return d3di.isVcolRgba(); }

static inline float get_screen_aspect_ratio() { return d3di.get_screen_aspect_ratio(); }
static inline void change_screen_aspect_ratio(float ar) { return d3di.change_screen_aspect_ratio(ar); }

static inline void *fast_capture_screen(int &w, int &h, int &stride, int &fmt) { return d3di.fast_capture_screen(w, h, stride, fmt); }
static inline void end_fast_capture_screen() { return d3di.end_fast_capture_screen(); }

static inline TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes) { return d3di.capture_screen(w, h, stride_bytes); }
static inline void release_capture_buffer() { return d3di.release_capture_buffer(); }

static inline void get_screen_size(int &w, int &h) { d3di.get_screen_size(w, h); }

static inline int create_predicate() { return d3di.create_predicate(); }
static inline void free_predicate(int id) { d3di.free_predicate(id); }

static inline bool begin_survey(int index) { return d3di.begin_survey(index); }
static inline void end_survey(int index) { return d3di.end_survey(index); }

static inline void begin_conditional_render(int index) { return d3di.begin_conditional_render(index); }
static inline void end_conditional_render(int id) { return d3di.end_conditional_render(id); }

static inline bool set_depth_bounds(float zmin, float zmax) { return d3di.set_depth_bounds(zmin, zmax); }
static inline bool supports_depth_bounds() { return d3di.supports_depth_bounds(); }

static inline VDECL get_program_vdecl(PROGRAM p) { return d3di.get_program_vdecl(p); }
static inline bool set_vertex_shader(VPROG ps) { return d3di.set_vertex_shader(ps); }
static inline bool set_pixel_shader(FSHADER ps) { return d3di.set_pixel_shader(ps); }

static inline VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n) { return d3di.create_vertex_shader_dagor(p, n); }
static inline FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n) { return d3di.create_pixel_shader_dagor(p, n); }

static inline bool get_vrr_supported() { return d3di.get_vrr_supported(); }
static inline bool get_vsync_enabled() { return d3di.get_vsync_enabled(); }
static inline bool enable_vsync(bool enable) { return d3di.enable_vsync(enable); }
static inline void get_video_modes_list(Tab<String> &list) { d3di.get_video_modes_list(list); }

static inline EventQuery *create_event_query() { return d3di.create_event_query(); }
static inline void release_event_query(EventQuery *q) { return d3di.release_event_query(q); }
static inline bool issue_event_query(EventQuery *q) { return d3di.issue_event_query(q); }
static inline bool get_event_query_status(EventQuery *q, bool force_flush) { return d3di.get_event_query_status(q, force_flush); }
static inline PROGRAM get_debug_program() { return d3di.get_debug_program(); }
static inline void beginEvent(const char *s) { return d3di.beginEvent(s); }
static inline void endEvent() { return d3di.endEvent(); }

static inline Texture *get_backbuffer_tex() { return d3di.get_backbuffer_tex(); }
static inline Texture *get_secondary_backbuffer_tex() { return d3di.get_secondary_backbuffer_tex(); }
static inline Texture *get_backbuffer_tex_depth() { return d3di.get_backbuffer_tex_depth(); }
#include "rayTrace/rayTraceMulti.inl.h"

static inline void resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.resource_barrier(desc, gpu_pipeline);
}

static inline RenderPass *create_render_pass(const RenderPassDesc &rp_desc) { return d3di.create_render_pass(rp_desc); }
static inline void delete_render_pass(RenderPass *rp) { d3di.delete_render_pass(rp); }
static inline void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  d3di.begin_render_pass(rp, area, targets);
}
static inline void next_subpass() { d3di.next_subpass(); }
static inline void end_render_pass() { d3di.end_render_pass(); }

static inline ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc)
{
  return d3di.get_resource_allocation_properties(desc);
}
static inline ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  return d3di.create_resource_heap(heap_group, size, flags);
}
static inline void destroy_resource_heap(ResourceHeap *heap) { d3di.destroy_resource_heap(heap); }
static inline Sbuffer *place_buffere_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return d3di.place_buffere_in_resource_heap(heap, desc, offset, alloc_info, name);
}
static inline BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return d3di.place_texture_in_resource_heap(heap, desc, offset, alloc_info, name);
}
static inline ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  return d3di.get_resource_heap_group_properties(heap_group);
}
static inline void map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  return d3di.map_tile_to_resource(tex, heap, mapping, mapping_count);
}
static inline TextureTilingInfo get_texture_tiling_info(BaseTexture *tex, size_t subresource)
{
  return d3di.get_texture_tiling_info(tex, subresource);
}

static inline void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.activate_buffer(buf, action, value, gpu_pipeline);
}
static inline void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.activate_texture(tex, action, value, gpu_pipeline);
}
static inline void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.deactivate_buffer(buf, gpu_pipeline);
}
static inline void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.deactivate_texture(tex, gpu_pipeline);
}

static inline ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip,
  unsigned dest_slice, unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth)
{
  return d3di.allocate_update_buffer_for_tex_region(dest_base_texture, dest_mip, dest_slice, offset_x, offset_y, offset_z, width,
    height, depth);
}

static inline ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice)
{
  return d3di.allocate_update_buffer_for_tex(dest_tex, dest_mip, dest_slice);
}
static inline void release_update_buffer(ResUpdateBuffer *&rub) { d3di.release_update_buffer(rub); }
static inline char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub) { return d3di.get_update_buffer_addr_for_write(rub); }
static inline size_t get_update_buffer_size(ResUpdateBuffer *rub) { return d3di.get_update_buffer_size(rub); }
static inline size_t get_update_buffer_pitch(ResUpdateBuffer *rub) { return d3di.get_update_buffer_pitch(rub); }
static inline size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub) { return d3di.get_update_buffer_slice_pitch(rub); }
static inline bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub)
{
  return d3di.update_texture_and_release_update_buffer(src_rub);
}


#if _TARGET_PC_WIN
namespace pcwin32
{
static inline void set_present_wnd(void *hwnd) { return d3di.set_present_wnd(hwnd); }

static inline bool set_capture_full_frame_buffer(bool ison) { return d3di.set_capture_full_frame_buffer(ison); }
static inline unsigned get_texture_format(BaseTexture *tex) { return d3di.get_texture_format(tex); }
static inline const char *get_texture_format_str(BaseTexture *tex) { return d3di.get_texture_format_str(tex); }
static inline void *get_native_surface(BaseTexture *tex) { return d3di.get_native_surface(tex); }
} // namespace pcwin32
#endif
}; // namespace d3d
#endif

#include <3d/dag_drv3dCmd.h>
