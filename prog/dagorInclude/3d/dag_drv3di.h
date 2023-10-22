//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <generic/dag_tabFwd.h>

class D3dEventQuery;
namespace d3d
{
typedef D3dEventQuery EventQuery;
}

#include <3d/dag_drv3d.h>

class String;

// function-pointers table that mirrors d3d API
struct D3dInterfaceTable
{
  const char *(*get_driver_name)();
  const char *(*get_device_driver_version)();
  const char *(*get_device_name)();
  const char *(*get_last_error)();
  uint32_t (*get_last_error_code)();

  void *(*get_device)();
  const Driver3dDesc &(*get_driver_desc)();
  int (*driver_command)(int command, void *par1, void *par2, void *par3);
  bool (*device_lost)(bool *can_reset_now);
  bool (*reset_device)();
  bool (*is_in_device_reset_now)();
  bool (*is_window_occluded)();

  bool (*should_use_compute_for_image_processing)(std::initializer_list<unsigned> formats);


  unsigned (*get_texformat_usage)(int cflg, int restype);
  bool (*check_texformat)(int cflg);
  bool (*issame_texformat)(int cflg1, int cflg2);
  bool (*check_cubetexformat)(int cflg);
  bool (*issame_cubetexformat)(int cflg1, int cflg2);
  bool (*check_voltexformat)(int cflg);
  bool (*issame_voltexformat)(int cflg1, int cflg2);
  BaseTexture *(*create_tex_0)(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name);
  BaseTexture *(*create_cubetex_0)(int size, int flg, int levels, const char *stat_name);
  BaseTexture *(*create_voltex)(int w, int h, int d, int flg, int levels, const char *stat_name);
  BaseTexture *(*create_array_tex)(int w, int h, int d, int flg, int levels, const char *stat_name);
  BaseTexture *(*create_cube_array_tex)(int side, int d, int flg, int levels, const char *stat_name);

  BaseTexture *(*create_ddsx_tex)(IGenLoad &crd, int flg, int q, int lev, const char *stat_name);
  BaseTexture *(*alloc_ddsx_tex)(const ddsx::Header &hdr, int flg, int q, int lev, const char *stat_name, int stub_tex_idx);

  BaseTexture *(*alias_tex_0)(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name);
  BaseTexture *(*alias_cubetex_0)(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name);
  BaseTexture *(*alias_voltex_0)(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name);
  BaseTexture *(*alias_array_tex_0)(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name);
  BaseTexture *(*alias_cube_array_tex_0)(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name);

  bool (*set_tex_usage_hint)(int w, int h, int mips, const char *format, unsigned int tex_num);
  void (*discard_managed_textures)();
  bool (*stretch_rect_0)(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst);
  bool (*copy_from_current_render_target)(BaseTexture *to_tex);

  void (*get_texture_statistics)(uint32_t *num_textures, uint64_t *total_mem, String *out_dump);

  d3d::SamplerHandle (*create_sampler)(const d3d::SamplerInfo &sampler_info);
  void (*destroy_sampler)(d3d::SamplerHandle sampler);
  void (*set_sampler)(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler);

  bool (*settex)(int stage, BaseTexture *);
  bool (*settex_vs)(int stage, BaseTexture *);

  PROGRAM (*create_program_0)(VPROG, FSHADER, VDECL, unsigned *, unsigned);
  PROGRAM (*create_program_1)(const uint32_t *, const uint32_t *, VDECL, unsigned *, unsigned);

  PROGRAM (*create_program_cs)(const uint32_t *cs_native);

  bool (*set_program)(PROGRAM);
  void (*delete_program)(PROGRAM);

  VPROG (*create_vertex_shader)(const uint32_t *native_code);
  VPROG (*create_vertex_shader_dagor)(const VPRTYPE *tokens, int len);
  VPROG (*create_vertex_shader_asm)(const char *asm_text);
  VPROG (*create_vertex_shader_hlsl)(const char *hlsl_text);
  void (*delete_vertex_shader)(VPROG vs);

  bool (*set_vertex_shader)(VPROG ps);
  bool (*set_const)(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs);
  bool (*set_immediate_const)(unsigned stage, const uint32_t *data, unsigned num_words);

  FSHADER (*create_pixel_shader)(const uint32_t *native_code);
  FSHADER (*create_pixel_shader_dagor)(const FSHTYPE *tokens, int len);
  FSHADER (*create_pixel_shader_asm)(const char *asm_text);
  FSHADER (*create_pixel_shader_hlsl)(const char *hlsl_text);
  void (*delete_pixel_shader)(FSHADER ps);

  bool (*set_pixel_shader)(FSHADER ps);
  int (*set_vs_constbuffer_size)(int required_size);
  int (*set_cs_constbuffer_size)(int required_size);
  bool (*set_const_buffer)(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size);

  uint32_t (*register_bindless_sampler)(BaseTexture *texture);

  uint32_t (*allocate_bindless_resource_range)(uint32_t resource_type, uint32_t count);
  uint32_t (*resize_bindless_resource_range)(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count);
  void (*free_bindless_resource_range)(uint32_t resource_type, uint32_t index, uint32_t count);
  void (*update_bindless_resource)(uint32_t index, D3dResource *res);
  void (*update_bindless_resources_to_null)(uint32_t resource_type, uint32_t index, uint32_t count);

  bool (*set_tex)(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler);
  bool (*set_rwtex)(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t, uint32_t, bool);
  bool (*clear_rwtexi)(BaseTexture *tex, const unsigned val[4], uint32_t, uint32_t);
  bool (*clear_rwtexf)(BaseTexture *tex, const float val[4], uint32_t, uint32_t);
  bool (*clear_rwbufi)(Sbuffer *tex, const unsigned val[4]);
  bool (*clear_rwbuff)(Sbuffer *tex, const float val[4]);

  bool (*set_buffer)(unsigned shader_stage, unsigned slot, Sbuffer *buffer);
  bool (*set_rwbuffer)(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

  Sbuffer *(*create_vb_0)(int size_bytes, int flags, const char *name);
  Sbuffer *(*create_ib)(int size_bytes, int flags, const char *stat_name);
  Sbuffer *(*create_sbuffer)(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *);

  bool (*set_depth_0)(BaseTexture *, DepthAccess);
  bool (*set_depth_1)(BaseTexture *tex, int layer, DepthAccess);
  bool (*set_backbuf_depth)();
  bool (*set_render_target_0)();
  bool (*set_render_target_1)(int rt_index, BaseTexture *, int level);
  bool (*set_render_target_2)(int rt_index, BaseTexture *, int fc, int level);
  bool (*set_render_target_3)(const Driver3dRenderTarget &rt);
  void (*get_render_target)(Driver3dRenderTarget &out_rt);
  bool (*get_target_size)(int &w, int &h);
  bool (*get_render_target_size)(int &w, int &h, BaseTexture *rt_tex, int lev);
  void (*set_variable_rate_shading)(unsigned rate_x, unsigned rate_y, VariableRateShadingCombiner vertex_combiner,
    VariableRateShadingCombiner pixel_combiner);
  void (*set_variable_rate_shading_texture)(BaseTexture *rate_texture);

  bool (*settm_0)(int which, const Matrix44 *tm);
  bool (*settm_1)(int which, const TMatrix &tm);
  void (*settm_2)(int which, const mat44f &tm);
  bool (*gettm_0)(int which, Matrix44 *out_tm);
  bool (*gettm_1)(int which, TMatrix &out_tm);
  void (*gettm_2)(int which, mat44f &out_tm);
  const mat44f &(*gettm_cref)(int which);
  void (*getm2vtm)(TMatrix &out_m2v);
  void (*setglobtm)(Matrix44 &m);
  void (*getglobtm)(Matrix44 &);
  bool (*setpersp)(const Driver3dPerspective &, TMatrix4 *);
  bool (*getpersp)(Driver3dPerspective &);
  bool (*validatepersp)(const Driver3dPerspective &);
  bool (*calcproj_0)(const Driver3dPerspective &, TMatrix4 &);
  bool (*calcproj_1)(const Driver3dPerspective &, mat44f &);
  void (*calcglobtm_0)(const mat44f &, const mat44f &, mat44f &);
  void (*calcglobtm_1)(const mat44f &, const Driver3dPerspective &, mat44f &);
  void (*calcglobtm_2)(const TMatrix &, const TMatrix4 &, TMatrix4 &);
  void (*calcglobtm_3)(const TMatrix &, const Driver3dPerspective &, TMatrix4 &);

  void (*getglobtm_vec)(mat44f &);
  void (*setglobtm_vec)(const mat44f &);

  bool (*setscissor)(int x, int y, int w, int h);
  bool (*setscissors)(dag::ConstSpan<ScissorRect> scissorRects);

  bool (*setview)(int x, int y, int w, int h, float minz, float maxz);
  bool (*setviews)(dag::ConstSpan<Viewport> viewports);
  bool (*getview)(int &x, int &y, int &w, int &h, float &minz, float &maxz);
  bool (*clearview)(int what, E3DCOLOR, float z, uint32_t stencil);

  bool (*update_screen)(bool app_active);
  bool (*setvsrc_ex)(int stream, Sbuffer *vb, int ofs, int stride_bytes);
  bool (*setind)(Sbuffer *ib);
  VDECL (*create_vdecl)(VSDTYPE *vsd);
  void (*delete_vdecl)(VDECL vdecl);
  bool (*setvdecl)(VDECL vdecl);

  bool (*draw_base)(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance);
  bool (*drawind_base)(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance);
  bool (*draw_up)(int type, int numprim, const void *ptr, int stride_bytes);
  bool (*drawind_up)(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes);

  bool (*draw_indirect)(int type, Sbuffer *buffer, uint32_t offset);
  bool (*draw_indexed_indirect)(int type, Sbuffer *buffer, uint32_t offset);
  bool (*multi_draw_indirect)(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset);
  bool (*multi_draw_indexed_indirect)(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset);

  bool (*dispatch)(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, GpuPipeline gpu_pipeline);
  bool (*dispatch_indirect)(Sbuffer *buffer, uint32_t offset, GpuPipeline gpu_pipeline);
  void (*dispatch_mesh)(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
  void (*dispatch_mesh_indirect)(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset);
  void (*dispatch_mesh_indirect_count)(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
    uint32_t count_byte_offset, uint32_t max_count);

  GPUFENCEHANDLE (*insert_fence)(GpuPipeline gpu_pipeline);
  void (*insert_wait_on_fence)(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline);

  shaders::DriverRenderStateId (*create_render_state)(const shaders::RenderState &);
  bool (*set_render_state)(shaders::DriverRenderStateId);
  void (*clear_render_states)();

  bool (*setantialias)(int aa_type);
  int (*getantialias)();

  bool (*set_blend_factor)(E3DCOLOR color);
  bool (*setstencil)(uint32_t ref);

  bool (*setwire)(bool in);

  bool (*set_srgb_backbuffer_write)(bool on);

  bool (*set_msaa_pass)();
  bool (*set_depth_resolve)();

  bool (*setgamma)(float power);

  bool (*isVcolRgba)();

  float (*get_screen_aspect_ratio)();
  void (*change_screen_aspect_ratio)(float ar);

  void *(*fast_capture_screen)(int &w, int &h, int &stride_bytes, int &format);
  void (*end_fast_capture_screen)();

  TexPixel32 *(*capture_screen)(int &w, int &h, int &stride_bytes);
  void (*release_capture_buffer)();

  void (*get_screen_size)(int &w, int &h);

  bool (*set_depth_bounds)(float zmn, float zmx);
  bool (*supports_depth_bounds)();

  int (*create_predicate)();
  void (*free_predicate)(int id);

  bool (*begin_survey)(int index);
  void (*end_survey)(int index);

  void (*begin_conditional_render)(int index);
  void (*end_conditional_render)(int id);

  VDECL (*get_program_vdecl)(PROGRAM p);

  bool (*get_vrr_supported)();
  bool (*get_vsync_enabled)();
  bool (*enable_vsync)(bool enable);

  d3d::EventQuery *(*create_event_query)();
  void (*release_event_query)(d3d::EventQuery *q);
  bool (*issue_event_query)(d3d::EventQuery *q);
  bool (*get_event_query_status)(d3d::EventQuery *q, bool force_flush);

  void (*set_present_wnd)(void *hwnd);

  bool (*set_capture_full_frame_buffer)(bool ison);
  unsigned (*get_texture_format)(BaseTexture *tex);
  const char *(*get_texture_format_str)(BaseTexture *tex);
  void (*get_video_modes_list)(Tab<String> &list);
  PROGRAM (*get_debug_program)();
  void (*beginEvent)(const char *s);
  void (*endEvent)();

  Texture *(*get_backbuffer_tex)();
  Texture *(*get_secondary_backbuffer_tex)();
  Texture *(*get_backbuffer_tex_depth)();

#include "rayTrace/rayTrace3di.inl.h"

  void (*resource_barrier)(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline /* = GpuPipeline::GRAPHICS*/);

  d3d::RenderPass *(*create_render_pass)(const RenderPassDesc &rp_desc);
  void (*delete_render_pass)(d3d::RenderPass *rp);

  void (*begin_render_pass)(d3d::RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets);
  void (*next_subpass)();
  void (*end_render_pass)();

  ResourceAllocationProperties (*get_resource_allocation_properties)(const ResourceDescription &desc);
  ResourceHeap *(*create_resource_heap)(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags);
  void (*destroy_resource_heap)(ResourceHeap *heap);
  Sbuffer *(*place_buffere_in_resource_heap)(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
    const ResourceAllocationProperties &alloc_info, const char *name);
  BaseTexture *(*place_texture_in_resource_heap)(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
    const ResourceAllocationProperties &alloc_info, const char *name);
  ResourceHeapGroupProperties (*get_resource_heap_group_properties)(ResourceHeapGroup *heap_group);

  void (*map_tile_to_resource)(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

  TextureTilingInfo (*get_texture_tiling_info)(BaseTexture *tex, size_t subresource);

  void (*activate_buffer)(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value,
    GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/);
  void (*activate_texture)(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value,
    GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/);
  void (*deactivate_buffer)(Sbuffer *buf, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/);
  void (*deactivate_texture)(BaseTexture *tex, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/);

  d3d::ResUpdateBuffer *(*allocate_update_buffer_for_tex_region)(BaseTexture *dest_base_texture, unsigned dest_mip,
    unsigned dest_slice, unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth);
  d3d::ResUpdateBuffer *(*allocate_update_buffer_for_tex)(BaseTexture *dest_tex, int dest_mip, int dest_slice);
  void (*release_update_buffer)(d3d::ResUpdateBuffer *&rub);
  char *(*get_update_buffer_addr_for_write)(d3d::ResUpdateBuffer *rub);
  size_t (*get_update_buffer_size)(d3d::ResUpdateBuffer *rub);
  size_t (*get_update_buffer_pitch)(d3d::ResUpdateBuffer *rub);
  size_t (*get_update_buffer_slice_pitch)(d3d::ResUpdateBuffer *rub);
  bool (*update_texture_and_release_update_buffer)(d3d::ResUpdateBuffer *&src_rub);

  void *(*get_native_surface)(BaseTexture *tex);

  // inline wrappers for overloaded function pointers
  Sbuffer *create_vb(int size_bytes, int flags, const char *name = "") { return create_vb_0(size_bytes, flags, name); }
  BaseTexture *create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL)
  {
    return create_tex_0(img, w, h, flg, levels, stat_name);
  }
  bool stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc = NULL, RectInt *rdst = NULL)
  {
    return stretch_rect_0(src, dst, rsrc, rdst);
  }

  inline bool setvsrc(int stream, Sbuffer *vb, int stride_bytes) { return setvsrc_ex(stream, vb, 0, stride_bytes); }
  BaseTexture *create_cubetex(int size, int flg, int levels, const char *stat_name = NULL)
  {
    return create_cubetex_0(size, flg, levels, stat_name);
  }

  BaseTexture *alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL)
  {
    return alias_tex_0(baseTexture, img, w, h, flg, levels, stat_name);
  }

  BaseTexture *alias_cubetex(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name = NULL)
  {
    return alias_cubetex_0(baseTexture, size, flg, levels, stat_name);
  }

  BaseTexture *alias_voltex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name = NULL)
  {
    return alias_voltex_0(baseTexture, w, h, d, flg, levels, stat_name);
  }

  BaseTexture *alias_array_tex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
  {
    return alias_array_tex_0(baseTexture, w, h, d, flg, levels, stat_name);
  }

  BaseTexture *alias_cube_array_tex(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name)
  {
    return alias_cube_array_tex_0(baseTexture, side, d, flg, levels, stat_name);
  }

  bool set_render_target() { return set_render_target_0(); }
  bool set_render_target(int ri, BaseTexture *tex, int fc, int level) { return set_render_target_2(ri, tex, fc, level); }
  bool set_render_target(int rt_index, BaseTexture *tex, int level) { return set_render_target_1(rt_index, tex, level); }
  bool set_render_target(const Driver3dRenderTarget &rt) { return set_render_target_3(rt); }
  inline bool set_depth(BaseTexture *tex, DepthAccess access = DepthAccess::RW) { return set_depth_0(tex, access); }
  inline bool set_depth(BaseTexture *tex, int layer, DepthAccess access = DepthAccess::RW) { return set_depth_1(tex, layer, access); }

  bool settm(int which, const Matrix44 *tm) { return settm_0(which, tm); }
  bool settm(int which, const TMatrix &tm) { return settm_1(which, tm); }
  void settm(int which, const mat44f &tm) { settm_2(which, tm); }
  bool gettm(int which, Matrix44 *out_tm) { return gettm_0(which, out_tm); }
  bool gettm(int which, TMatrix &out_tm) { return gettm_1(which, out_tm); }
  void gettm(int which, mat44f &out_tm) { gettm_2(which, out_tm); }

  DriverCode driverCode;
  const char *driverName;
  const char *driverVer;
  const char *deviceName;
  Driver3dDesc drvDesc;
};

// function-pointers table instance (coud be used for deferred d3d API calls)
extern D3dInterfaceTable d3di;
