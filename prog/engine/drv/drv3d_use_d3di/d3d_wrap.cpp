// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_interface_table.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

namespace d3d
{
DriverCode get_driver_code() { return d3di.driverCode; }

const char *get_driver_name() { return d3di.driverName; }
const char *get_device_driver_version() { return d3di.driverVer; }
const char *get_device_name() { return d3di.deviceName; }
const char *get_last_error() { return d3di.get_last_error(); }

void *get_device() { return nullptr; }
const Driver3dDesc &get_driver_desc() { return d3di.drvDesc; }

int driver_command(Drv3dCommand cmd, void *p1, void *p2, void *p3) { return d3di.driver_command(cmd, p1, p2, p3); }

bool device_lost(bool *can_reset_now) { return d3di.device_lost(can_reset_now); }
bool reset_device() { return d3di.reset_device(); }
bool is_in_device_reset_now() { return d3di.is_in_device_reset_now(); }
bool is_window_occluded() { return d3di.is_window_occluded(); }

bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats)
{
  return d3di.should_use_compute_for_image_processing(formats);
}

bool check_texformat(int cflg) { return d3di.check_texformat(cflg); }
int get_max_sample_count(int cflg) { return d3di.get_max_sample_count(cflg); }
unsigned get_texformat_usage(int cflg, int restype) { return d3di.get_texformat_usage(cflg, restype); }
bool issame_texformat(int cflg1, int cflg2) { return d3di.issame_texformat(cflg1, cflg2); }
bool check_cubetexformat(int cflg) { return d3di.check_cubetexformat(cflg); }
bool check_voltexformat(int cflg) { return d3di.check_voltexformat(cflg); }

Texture *create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  return d3di.create_tex(img, w, h, flg, levels, stat_name);
}
CubeTexture *create_cubetex(int size, int flg, int levels, const char *stat_name)
{
  return d3di.create_cubetex(size, flg, levels, stat_name);
}
VolTexture *create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return d3di.create_voltex(w, h, d, flg, levels, stat_name);
}
ArrayTexture *create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return d3di.create_array_tex(w, h, d, flg, levels, stat_name);
}
ArrayTexture *create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  return d3di.create_cube_array_tex(side, d, flg, levels, stat_name);
}

BaseTexture *create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *stat_name)
{
  return d3di.create_ddsx_tex(crd, flg, quality_id, levels, stat_name);
}

BaseTexture *alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels, const char *stat_name, int stub_tex_idx)
{
  return d3di.alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name, stub_tex_idx);
}

Texture *alias_tex(Texture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  return d3di.alias_tex(baseTexture, img, w, h, flg, levels, stat_name);
}

CubeTexture *alias_cubetex(CubeTexture *baseTexture, int size, int flg, int levels, const char *stat_name)
{
  return d3di.alias_cubetex(baseTexture, size, flg, levels, stat_name);
}

VolTexture *alias_voltex(VolTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return d3di.alias_voltex(baseTexture, w, h, d, flg, levels, stat_name);
}

ArrayTexture *alias_array_tex(ArrayTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return d3di.alias_array_tex(baseTexture, w, h, d, flg, levels, stat_name);
}

ArrayTexture *alias_cube_array_tex(ArrayTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name)
{
  return d3di.alias_cube_array_tex(baseTexture, side, d, flg, levels, stat_name);
}

bool stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst) { return d3di.stretch_rect(src, dst, rsrc, rdst); }
bool copy_from_current_render_target(BaseTexture *to_tex) { return d3di.copy_from_current_render_target(to_tex); }

void get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  d3di.get_texture_statistics(num_textures, total_mem, out_dump);
}

PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides, unsigned streams)
{
  return d3di.create_program_0(vprog, fsh, vdecl, strides, streams);
}
PROGRAM create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides, unsigned streams)
{
  return d3di.create_program_1(vpr_native, fsh_native, vdecl, strides, streams);
}

PROGRAM create_program_cs(const uint32_t *cs_native, CSPreloaded preloaded) { return d3di.create_program_cs(cs_native, preloaded); }

bool set_program(PROGRAM p) { return d3di.set_program(p); }
void delete_program(PROGRAM p) { return d3di.delete_program(p); }

VPROG create_vertex_shader(const uint32_t *native_code) { return d3di.create_vertex_shader(native_code); }
void delete_vertex_shader(VPROG vs) { return d3di.delete_vertex_shader(vs); }

bool set_const(unsigned stage, unsigned b, const float *data, unsigned num_regs) { return d3di.set_const(stage, b, data, num_regs); }

FSHADER create_pixel_shader(const uint32_t *native_code) { return d3di.create_pixel_shader(native_code); }
void delete_pixel_shader(FSHADER ps) { return d3di.delete_pixel_shader(ps); }

int set_vs_constbuffer_size(int required_size) { return d3di.set_vs_constbuffer_size(required_size); }
int set_cs_constbuffer_size(int required_size) { return d3di.set_cs_constbuffer_size(required_size); }

bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size)
{
  return d3di.set_const_buffer(stage, slot, buffer, consts_offset, consts_size);
}

d3d::SamplerHandle request_sampler(const d3d::SamplerInfo &sampler_info) { return d3di.request_sampler(sampler_info); }

void set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler) { d3di.set_sampler(shader_stage, slot, sampler); }

uint32_t register_bindless_sampler(BaseTexture *texture) { return d3di.register_bindless_sampler(texture); }
uint32_t register_bindless_sampler(SamplerHandle sampler) { return d3di.register_bindless_sampler(sampler); }

uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  return d3di.allocate_bindless_resource_range(resource_type, count);
}
uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  return d3di.resize_bindless_resource_range(resource_type, index, current_count, new_count);
}
void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  d3di.free_bindless_resource_range(resource_type, index, count);
}
void update_bindless_resource(uint32_t index, D3dResource *res) { d3di.update_bindless_resource(index, res); }
void update_bindless_resource_sto_null(uint32_t resource_type, uint32_t index, uint32_t count)
{
  d3di.update_bindless_resources_to_null(resource_type, index, count);
}

bool set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler)
{
  return d3di.set_tex(shader_stage, slot, tex, use_sampler);
}
bool set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip, bool as_uint)
{
  return d3di.set_rwtex(shader_stage, slot, tex, face, mip, as_uint);
}
bool clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip)
{
  return d3di.clear_rwtexi(tex, val, face, mip);
}
bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip) { return d3di.clear_rwtexf(tex, val, face, mip); }
bool clear_rwbufi(Sbuffer *tex, const unsigned val[4]) { return d3di.clear_rwbufi(tex, val); }
bool clear_rwbuff(Sbuffer *tex, const float val[4]) { return d3di.clear_rwbuff(tex, val); }

bool clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val) { return d3di.clear_rt(rt, clear_val); }

bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer) { return d3di.set_buffer(shader_stage, slot, buffer); }
bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer) { return d3di.set_rwbuffer(shader_stage, slot, buffer); }

Vbuffer *create_vb(int sz, int f, const char *name) { return d3di.create_vb(sz, f, name); }
Ibuffer *create_ib(int size_bytes, int flags, const char *stat_name) { return d3di.create_ib(size_bytes, flags, stat_name); }

Vbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name)
{
  return d3di.create_sbuffer(struct_size, elements, flags, texfmt, name);
}

bool set_render_target() { return d3di.set_render_target(); }

bool set_depth(BaseTexture *tex, DepthAccess access) { return d3di.set_depth(tex, access); }
bool set_depth(BaseTexture *tex, int layer, DepthAccess access) { return d3di.set_depth(tex, layer, access); }

bool set_render_target(int rt_index, BaseTexture *t, int fc, uint8_t level) { return d3di.set_render_target(rt_index, t, fc, level); }
bool set_render_target(int rt_index, BaseTexture *t, uint8_t level) { return d3di.set_render_target(rt_index, t, level); }

void get_render_target(Driver3dRenderTarget &out_rt) { return d3di.get_render_target(out_rt); }
bool set_render_target(const Driver3dRenderTarget &rt) { return d3di.set_render_target(rt); }

bool get_target_size(int &w, int &h) { return d3di.get_target_size(w, h); }
bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, uint8_t level)
{
  return d3di.get_render_target_size(w, h, rt_tex, level);
}

bool settm(int which, const Matrix44 *tm) { return d3di.settm(which, tm); }
bool settm(int which, const TMatrix &tm) { return d3di.settm(which, tm); }
void settm(int which, const mat44f &out_tm) { return d3di.settm(which, out_tm); }

bool gettm(int which, Matrix44 *out_tm) { return d3di.gettm(which, out_tm); }
bool gettm(int which, TMatrix &out_tm) { return d3di.gettm(which, out_tm); }
void gettm(int which, mat44f &out_tm) { return d3di.gettm(which, out_tm); }
const mat44f &gettm_cref(int which) { return d3di.gettm_cref(which); }

void getm2vtm(TMatrix &out_m2v) { return d3di.getm2vtm(out_m2v); }
void getglobtm(Matrix44 &m) { return d3di.getglobtm(m); }
void setglobtm(Matrix44 &m) { return d3di.setglobtm(m); }

void getglobtm(mat44f &m) { return d3di.getglobtm_vec(m); }
void setglobtm(const mat44f &m) { return d3di.setglobtm_vec(m); }

bool setpersp(const Driver3dPerspective &p, TMatrix4 *tm) { return d3di.setpersp(p, tm); }
bool getpersp(Driver3dPerspective &p) { return d3di.getpersp(p); }
bool validatepersp(const Driver3dPerspective &p) { return d3di.validatepersp(p); }

bool setscissor(int x, int y, int w, int h) { return d3di.setscissor(x, y, w, h); }

bool setview(int x, int y, int w, int h, float minz, float maxz) { return d3di.setview(x, y, w, h, minz, maxz); }
bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz) { return d3di.getview(x, y, w, h, minz, maxz); }

bool clearview(int what, E3DCOLOR c, float z, uint32_t stencil) { return d3di.clearview(what, c, z, stencil); }

bool update_screen(bool app_active) { return d3di.update_screen(app_active); }
void wait_for_async_present(bool force) { return d3di.wait_for_async_present(force); }
void gpu_latency_wait() { d3di.gpu_latency_wait(); }

bool setvsrc_ex(int s, Vbuffer *vb, int ofs, int stride_bytes) { return d3di.setvsrc_ex(s, vb, ofs, stride_bytes); }

bool setind(Ibuffer *ib) { return d3di.setind(ib); }

VDECL create_vdecl(VSDTYPE *vsd) { return d3di.create_vdecl(vsd); }
void delete_vdecl(VDECL vdecl) { return d3di.delete_vdecl(vdecl); }
bool setvdecl(VDECL vdecl) { return d3di.setvdecl(vdecl); }

bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.draw_base(type, start, numprim, num_instances, start_instance);
}
bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}
bool draw_up(int type, int numprim, const void *ptr, int stride_bytes) { return d3di.draw_up(type, numprim, ptr, stride_bytes); }
bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  return d3di.drawind_up(type, minvert, numvert, numprim, ind, ptr, stride_bytes);
}
bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset) { return d3di.draw_indirect(type, args, byte_offset); }
bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset)
{
  return d3di.draw_indexed_indirect(type, args, byte_offset);
}
bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return d3di.multi_draw_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}
bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return d3di.multi_draw_indexed_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}

bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, GpuPipeline gpu_pipeline)
{
  return d3di.dispatch(thread_group_x, thread_group_y, thread_group_z, gpu_pipeline);
}
bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline)
{
  return d3di.dispatch_indirect(args, byte_offset, gpu_pipeline);
}

void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  d3di.dispatch_mesh(thread_group_x, thread_group_y, thread_group_z);
}

void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  d3di.dispatch_mesh_indirect(args, dispatch_count, stride_bytes, byte_offset);
}

void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  d3di.dispatch_mesh_indirect_count(args, args_stride_bytes, args_byte_offset, count, count_byte_offset, max_count);
}

bool set_blend_factor(E3DCOLOR color) { return d3di.set_blend_factor(color); }
bool setstencil(uint32_t ref) { return d3di.setstencil(ref); }

bool setwire(bool in) { return d3di.setwire(in); }

bool set_srgb_backbuffer_write(bool on) { return d3di.set_srgb_backbuffer_write(on); }

bool setgamma(float power) { return d3di.setgamma(power); }

float get_screen_aspect_ratio() { return d3di.get_screen_aspect_ratio(); }
void change_screen_aspect_ratio(float ar) { return d3di.change_screen_aspect_ratio(ar); }

void *fast_capture_screen(int &w, int &h, int &stride, int &fmt) { return d3di.fast_capture_screen(w, h, stride, fmt); }
void end_fast_capture_screen() { return d3di.end_fast_capture_screen(); }

TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes) { return d3di.capture_screen(w, h, stride_bytes); }
void release_capture_buffer() { return d3di.release_capture_buffer(); }

void get_screen_size(int &w, int &h) { d3di.get_screen_size(w, h); }

int create_predicate() { return d3di.create_predicate(); }
void free_predicate(int id) { d3di.free_predicate(id); }

bool begin_survey(int index) { return d3di.begin_survey(index); }
void end_survey(int index) { return d3di.end_survey(index); }

void begin_conditional_render(int index) { return d3di.begin_conditional_render(index); }
void end_conditional_render(int id) { return d3di.end_conditional_render(id); }

bool set_depth_bounds(float zmin, float zmax) { return d3di.set_depth_bounds(zmin, zmax); }

VDECL get_program_vdecl(PROGRAM p) { return d3di.get_program_vdecl(p); }
bool set_vertex_shader(VPROG ps) { return d3di.set_vertex_shader(ps); }
bool set_pixel_shader(FSHADER ps) { return d3di.set_pixel_shader(ps); }

VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n) { return d3di.create_vertex_shader_dagor(p, n); }
FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n) { return d3di.create_pixel_shader_dagor(p, n); }

bool get_vrr_supported() { return d3di.get_vrr_supported(); }
bool get_vsync_enabled() { return d3di.get_vsync_enabled(); }
bool enable_vsync(bool enable) { return d3di.enable_vsync(enable); }
void get_video_modes_list(Tab<String> &list) { d3di.get_video_modes_list(list); }

EventQuery *create_event_query() { return d3di.create_event_query(); }
void release_event_query(EventQuery *q) { return d3di.release_event_query(q); }
bool issue_event_query(EventQuery *q) { return d3di.issue_event_query(q); }
bool get_event_query_status(EventQuery *q, bool force_flush) { return d3di.get_event_query_status(q, force_flush); }
PROGRAM get_debug_program() { return d3di.get_debug_program(); }
void beginEvent(const char *s) { return d3di.beginEvent(s); }
void endEvent() { return d3di.endEvent(); }

#if _TARGET_PC_WIN
namespace pcwin32
{
void set_present_wnd(void *hwnd) { return d3di.set_present_wnd(hwnd); }

bool set_capture_full_frame_buffer(bool ison) { return d3di.set_capture_full_frame_buffer(ison); }
unsigned get_texture_format(BaseTexture *tex) { return d3di.get_texture_format(tex); }
const char *get_texture_format_str(BaseTexture *tex) { return d3di.get_texture_format_str(tex); }
void *get_native_surface(BaseTexture *tex) { return d3di.get_native_surface(tex); }
} // namespace pcwin32
#endif
}; // namespace d3d
