#include <3d/dag_drv3d.h>
#include <3d/dag_drv3di.h>
#include <3d/dag_drv3d_res.h>
#include <math/dag_Point2.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_span.h>
#if _TARGET_PC
#include <3d/dag_drv3d_pc.h>
#endif

#if _TARGET_PC_WIN
#include <3d/dag_drv3d_pc.h>
bool d3d::set_pixel_shader(FSHADER) { return true; }
bool d3d::set_vertex_shader(VPROG) { return true; }
VDECL d3d::get_program_vdecl(PROGRAM) { return 1; }
#endif
PROGRAM d3d::get_debug_program() { return 1; }

static Driver3dDesc stub_desc = {0};

bool d3d::init_driver() { return false; }
bool d3d::is_inited() { return false; }
bool d3d::init_video(void *hinst, main_wnd_f *, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb)
{
  return false;
}
void d3d::release_driver() {}

DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::null); }
const char *d3d::get_driver_name() { return "d3d/stub"; }
const char *d3d::get_device_driver_version() { return "1.0"; }
const char *d3d::get_device_name() { return "device/stub"; }
const char *d3d::get_last_error() { return "n/a"; }
uint32_t d3d::get_last_error_code() { return 0; }
void d3d::window_destroyed(void *hwnd) {}

namespace d3d
{
GpuAutoLock::GpuAutoLock() {}
GpuAutoLock ::~GpuAutoLock() {}
} // namespace d3d

void *d3d::get_device() { return NULL; }

/// returns driver description (pointer to static object)
const Driver3dDesc &d3d::get_driver_desc() { return stub_desc; }

int d3d::driver_command(int command, void *par1, void *par2, void *par3) { return 0; }
bool d3d::device_lost(bool *can_reset_now) { return false; }
bool d3d::reset_device() { return false; }
bool d3d::is_in_device_reset_now() { return false; }
bool d3d::is_window_occluded() { return false; }

bool d3d::should_use_compute_for_image_processing(std::initializer_list<unsigned>) { return false; }

D3dEventQuery *d3d::create_event_query() { return nullptr; }
void d3d::release_event_query(D3dEventQuery *q) {}
bool d3d::issue_event_query(D3dEventQuery *q) { return false; }
bool d3d::get_event_query_status(D3dEventQuery *q, bool force_flush) { return false; }

unsigned d3d::get_texformat_usage(int cflg, int restype) { return 0; }
bool d3d::check_texformat(int cflg) { return false; }
bool d3d::issame_texformat(int cflg1, int cflg2) { return false; }
bool d3d::check_cubetexformat(int cflg) { return false; }
bool d3d::issame_cubetexformat(int cflg1, int cflg2) { return false; }
bool d3d::check_voltexformat(int cflg) { return false; }
bool d3d::issame_voltexformat(int cflg1, int cflg2) { return false; }
Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name) { return NULL; }
BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels, const char *, int) { return NULL; }
BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *) { return NULL; }
CubeTexture *d3d::create_cubetex(int size, int flg, int levels, const char *) { return NULL; }
VolTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char *) { return NULL; }
ArrayTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char *name) { return NULL; }
BaseTexture *d3d::create_cube_array_tex(int, int, int, int, char const *) { return nullptr; }
void d3d::resource_barrier(ResourceBarrierDesc, GpuPipeline) {}

bool d3d::set_tex_usage_hint(int w, int h, int mips, const char *format, unsigned int tex_num) { return false; }

void d3d::discard_managed_textures() {}
bool d3d::stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst) { return false; }
bool d3d::copy_from_current_render_target(BaseTexture * /*to_tex*/) { return false; }

// Texture states setup
VPROG d3d::create_vertex_shader(const uint32_t *native_code) { return BAD_VPROG; }
void d3d::delete_vertex_shader(VPROG vs) {}

bool d3d::set_const(unsigned, unsigned reg_base, const void *data, unsigned num_regs) { return false; }
bool d3d::set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words) { return false; }

int d3d::set_cs_constbuffer_size(int) { return 0; }
int d3d::set_vs_constbuffer_size(int /*required_size*/) { return 256; }
FSHADER d3d::create_pixel_shader(const uint32_t *native_code) { return BAD_FSHADER; }
void d3d::delete_pixel_shader(FSHADER ps) {}

/*VPROG d3d::create_vertex_shader_dagor(const VPRTYPE *tokens, int len) { return BAD_VPROG; }
VPROG d3d::create_vertex_shader_asm(const char *asm_text) { return BAD_VPROG; }
VPROG d3d::create_vertex_shader_hlsl(const char *hlsl_text, unsigned len,
  const char *entry, const char *profile)
{
  return BAD_VPROG;
}
FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE *tokens, int len) { return BAD_FSHADER; }
FSHADER d3d::create_pixel_shader_asm(const char *asm_text) { return BAD_FSHADER; }
FSHADER d3d::create_pixel_shader_hlsl(const char *hlsl_text, unsigned len,
  const char *entry, const char *profile)
{
  return BAD_FSHADER;
}*/

PROGRAM d3d::create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides, unsigned streams) { return BAD_PROGRAM; }
// if strides & streams are unset, will get them from VDECL
//  should be deleted externally

PROGRAM d3d::create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides, unsigned streams)
{
  return BAD_PROGRAM;
}

PROGRAM d3d::create_program_cs(const uint32_t *cs_native) { return BAD_PROGRAM; }

bool d3d::set_program(PROGRAM) { return false; }
void d3d::delete_program(PROGRAM) {}

Vbuffer *d3d::create_vb(int size_bytes, int flags, const char *) { return NULL; }
Ibuffer *d3d::create_ib(int size_bytes, int flags, const char *) { return NULL; }
Sbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char *name) { return NULL; }

bool d3d::draw_indirect(int type, Sbuffer *args, uint32_t byte_offset) { return false; }
bool d3d::draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset) { return false; }
bool d3d::multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return false;
}
bool d3d::multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return false;
}
bool d3d::dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline) { return false; }
void d3d::dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  G_UNUSED(thread_group_x);
  G_UNUSED(thread_group_y);
  G_UNUSED(thread_group_z);
}
void d3d::dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_UNUSED(args);
  G_UNUSED(dispatch_count);
  G_UNUSED(stride_bytes);
  G_UNUSED(byte_offset);
}
void d3d::dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  G_UNUSED(args);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(count);
  G_UNUSED(count_byte_offset);
  G_UNUSED(max_count);
}
bool d3d::set_const_buffer(uint32_t, uint32_t, Sbuffer *, uint32_t, uint32_t) { return false; }
bool d3d::set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs) { return false; }

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline /*gpu_pipeline*/) { return BAD_GPUFENCEHANDLE; }
void d3d::insert_wait_on_fence(GPUFENCEHANDLE & /*fence*/, GpuPipeline /*gpu_pipeline*/) {}

bool d3d::set_depth(Texture *tex, DepthAccess) { return false; }
bool d3d::set_depth(BaseTexture *tex, int, DepthAccess) { return false; }
bool d3d::set_backbuf_depth() { return false; }
bool d3d::set_render_target() { return false; }
bool d3d::set_render_target(int, Texture *, int level) { return false; }
bool d3d::set_render_target(int, BaseTexture *, int fc, int level) { return false; }

void d3d::get_render_target(Driver3dRenderTarget &out_rt) {}
bool d3d::set_render_target(const Driver3dRenderTarget &rt) { return false; }
bool d3d::get_target_size(int &w, int &h) { return false; }
bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev) { return false; }

bool d3d::settm(int which, const Matrix44 *tm) { return false; }
bool d3d::settm(int which, const TMatrix &tm) { return false; }
void d3d::settm(int which, const mat44f &m) {}
void d3d::setglobtm(Matrix44 &) {}
void d3d::getglobtm(mat44f &) {}
void d3d::setglobtm(const mat44f &) {}

bool d3d::gettm(int which, Matrix44 *out_tm) { return false; }
bool d3d::gettm(int which, TMatrix &out_tm) { return false; }
void d3d::gettm(int which, mat44f &out_m) { v_mat44_ident(out_m); }
const mat44f &d3d::gettm_cref(int which) { return ZERO<mat44f>(); }

void d3d::getm2vtm(TMatrix &out_m2v) {}
void d3d::getglobtm(Matrix44 &) {}

bool d3d::setpersp(const Driver3dPerspective &, TMatrix4 *) { return false; }
bool d3d::getpersp(Driver3dPerspective &) { return false; }
bool d3d::validatepersp(const Driver3dPerspective &) { return false; }
bool d3d::calcproj(const Driver3dPerspective &, TMatrix4 &) { return false; }
bool d3d::calcproj(const Driver3dPerspective &, mat44f &) { return false; }

bool d3d::setview(int x, int y, int w, int h, float minz, float maxz) { return false; }
bool d3d::setviews(dag::ConstSpan<Viewport> viewports) { return false; }
bool d3d::setscissors(dag::ConstSpan<ScissorRect> scissorRects) { return false; }

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz) { return false; }

bool d3d::clearview(int what, E3DCOLOR, float z, uint32_t stencil) { return false; }

bool d3d::update_screen(bool app_active) { return false; }

/// set vertex stream source
bool d3d::setvsrc_ex(int stream, Vbuffer *, int ofs, int stride_bytes) { return false; }

/// set indices
bool d3d::setind(Ibuffer *ib) { return false; }

VDECL d3d::create_vdecl(VSDTYPE *vsd) { return BAD_VDECL; }
void d3d::delete_vdecl(VDECL vdecl) {}
bool d3d::setvdecl(VDECL vdecl) { return false; }

bool d3d::draw_base(int type, int start, int numprim, uint32_t num, uint32_t start_instance) { return false; }
bool d3d::drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num, uint32_t start_instance) { return false; }
bool d3d::draw_up(int type, int numprim, const void *ptr, int stride_bytes) { return false; }
bool d3d::drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  return false;
}

bool d3d::dispatch(uint32_t /*thread_group_x*/, uint32_t /*thread_group_y*/, uint32_t /*thread_group_z*/, GpuPipeline /*gpu_pipeline*/)
{
  return false;
}

bool d3d::setantialias(int aa_type) { return false; }
int d3d::getantialias() { return 0; }

bool d3d::set_blend_factor(E3DCOLOR c) { return false; }

bool d3d::setstencil(uint32_t ref) { return false; }

bool d3d::setwire(bool in) { return false; }

bool d3d::set_msaa_pass() { return true; }

bool d3d::set_depth_resolve() { return true; }

bool d3d::setgamma(float p) { return false; }
bool d3d::isVcolRgba() { return false; }

float d3d::get_screen_aspect_ratio() { return 4.0f / 3.0f; }
void d3d::change_screen_aspect_ratio(float) {}

void *d3d::fast_capture_screen(int &w, int &h, int &stride_bytes, int &format) { return NULL; }
void d3d::end_fast_capture_screen() {}

TexPixel32 *d3d::capture_screen(int &w, int &h, int &stride_bytes) { return NULL; }
void d3d::release_capture_buffer() {}

void d3d::get_screen_size(int &w, int &h)
{
  w = 1280;
  h = 720;
}

bool d3d::fill_interface_table(D3dInterfaceTable &p)
{
  memset(&p, 0, sizeof(p));
  return false;
}

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *)
{
  if (num_textures)
    *num_textures = 1;
  if (total_mem)
    *total_mem = 100;
}

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib,
  int max_stblk)
{}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}

int d3d::create_predicate() { return -1; }
void d3d::free_predicate(int) {}
bool d3d::begin_survey(int) { return false; }
void d3d::end_survey(int) {}
void d3d::begin_conditional_render(int) {}
void d3d::end_conditional_render(int) {}

bool d3d::set_depth_bounds(float zmin, float zmax) { return false; }
bool d3d::supports_depth_bounds()
{
  return false;
}; // returns true if hardware supports depth bounds. same as get_driver_desc().caps.hasDepthBoundsTest
#if !(_TARGET_C1 | _TARGET_C2)
const bool d3d::HALF_TEXEL_OFS = false;
#endif

#if _TARGET_PC
bool d3d::get_vrr_supported() { return false; }
bool d3d::get_vsync_enabled() { return false; }
bool d3d::enable_vsync(bool enable) { return !enable; }
#endif

static bool srgb_bb_on = false;
bool d3d::set_srgb_backbuffer_write(bool on)
{
  bool old = srgb_bb_on;
  srgb_bb_on = on;
  return old;
}

d3d::SamplerHandle d3d::create_sampler(const d3d::SamplerInfo &sampler_info) { return 0; }
void d3d::destroy_sampler(d3d::SamplerHandle sampler) {}
void d3d::set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler) {}
uint32_t d3d::register_bindless_sampler(BaseTexture *) { return 0; }

uint32_t d3d::allocate_bindless_resource_range(uint32_t, uint32_t) { return 0; }
uint32_t d3d::resize_bindless_resource_range(uint32_t, uint32_t, uint32_t, uint32_t) { return 0; }
void d3d::free_bindless_resource_range(uint32_t, uint32_t, uint32_t) {}
void d3d::update_bindless_resource(uint32_t, D3dResource *) {}
void d3d::update_bindless_resources_to_null(uint32_t, uint32_t, uint32_t) {}

bool d3d::set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler) { return false; }
bool d3d::set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t, uint32_t, bool) { return false; }
bool d3d::clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level) { return false; }
bool d3d::clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level) { return false; }
bool d3d::clear_rwbufi(Sbuffer *tex, const unsigned val[4]) { return false; }
bool d3d::clear_rwbuff(Sbuffer *tex, const float val[4]) { return false; }


bool d3d::set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer) { return false; }
bool d3d::set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer) { return false; }

void d3d::beginEvent(const char *) {}
void d3d::endEvent() {}

Texture *d3d::get_backbuffer_tex() { return nullptr; }
Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }
Texture *d3d::get_backbuffer_tex_depth() { return nullptr; }

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &) { return shaders::DriverRenderStateId(); }
bool d3d::set_render_state(shaders::DriverRenderStateId) { return false; }
void d3d::clear_render_states() {}

Texture *d3d::alias_tex(Texture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  return nullptr;
}
CubeTexture *d3d::alias_cubetex(CubeTexture *baseTexture, int size, int flg, int levels, const char *stat_name) { return nullptr; }
VolTexture *d3d::alias_voltex(VolTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}
ArrayTexture *d3d::alias_array_tex(ArrayTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}
ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}

#include "../drv3d_commonCode/rayTracingStub.inc.cpp"
