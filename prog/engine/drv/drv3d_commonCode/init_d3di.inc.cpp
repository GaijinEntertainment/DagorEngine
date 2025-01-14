#include <drv/3d/dag_interface_table.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_tiledResource.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_shaderLibrary.h>
#include <drv/3d/dag_capture.h>

#if _TARGET_PC
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_query.h>
#include <debug/dag_fatal.h>

static void na_func() { DAG_FATAL("D3DI function not implemented"); }

#define FILL_ENTRY(X)     d3dit.X = d3d::X
#define FILL_ENTRY2(X, Y) d3dit.X = d3d::Y
#if _TARGET_PC_WIN
#define FILL_ENTRY_PC(X) d3dit.X = d3d::pcwin32::X
#else
#define FILL_ENTRY_PC(X)
#endif
#define FILL_NAMESPACE_ENTRY(nspace, name)                        d3dit.nspace.name = d3d::nspace::name
#define FILL_NAMESPACE_ENTRY2(nspace, internal_name, public_name) d3dit.nspace.internal_name = d3d::nspace::public_name

bool d3d::fill_interface_table(D3dInterfaceTable &d3dit)
{
  // fill table with guard/fatal functions first
  void (**funcTbl)() = (void (**)()) & d3dit;
  for (int i = 0; i < sizeof(d3di) / sizeof(*funcTbl); i++)
    funcTbl[i] = &na_func;

  // fill table with proper function pointers
  FILL_ENTRY(get_driver_name);
  FILL_ENTRY(get_device_driver_version);
  FILL_ENTRY(get_device_name);
  FILL_ENTRY(get_last_error);
  FILL_ENTRY(get_last_error_code);

  FILL_ENTRY(get_device);
  FILL_ENTRY(get_driver_desc);
  FILL_ENTRY(driver_command);
  FILL_ENTRY(device_lost);
  FILL_ENTRY(reset_device);
  FILL_ENTRY(is_in_device_reset_now);
  FILL_ENTRY(is_window_occluded);

  FILL_ENTRY(should_use_compute_for_image_processing);

  FILL_ENTRY(get_texformat_usage);
  FILL_ENTRY(check_texformat);
  FILL_ENTRY(get_max_sample_count);
  FILL_ENTRY(issame_texformat);
  FILL_ENTRY(check_cubetexformat);
  FILL_ENTRY(check_voltexformat);
  FILL_ENTRY2(create_tex_0, create_tex);
  FILL_ENTRY2(create_cubetex_0, create_cubetex);
  FILL_ENTRY(create_voltex);
  FILL_ENTRY(create_array_tex);
  FILL_ENTRY(create_cube_array_tex);
  FILL_ENTRY(create_ddsx_tex);
  FILL_ENTRY(alloc_ddsx_tex);

  FILL_ENTRY2(alias_tex_0, alias_tex);
  FILL_ENTRY2(alias_cubetex_0, alias_cubetex);
  FILL_ENTRY2(alias_voltex_0, alias_voltex);
  FILL_ENTRY2(alias_array_tex_0, alias_array_tex);
  FILL_ENTRY2(alias_cube_array_tex_0, alias_cube_array_tex);

  FILL_ENTRY2(stretch_rect_0, stretch_rect);
  FILL_ENTRY(copy_from_current_render_target);

  FILL_ENTRY(get_texture_statistics);

  FILL_ENTRY(settex);
  FILL_ENTRY(settex_vs);

  FILL_ENTRY2(create_program_0, create_program);
  FILL_ENTRY2(create_program_1, create_program);
  FILL_ENTRY2(create_program_cs, create_program_cs);

  FILL_ENTRY(set_program);
  FILL_ENTRY(delete_program);

  FILL_ENTRY(create_vertex_shader);
  FILL_ENTRY(create_vertex_shader_dagor);
  FILL_ENTRY(delete_vertex_shader);

  FILL_ENTRY(set_vertex_shader);
  FILL_ENTRY(set_const);
  FILL_ENTRY(set_immediate_const);

  FILL_ENTRY(set_vs_constbuffer_size);
  FILL_ENTRY(set_cs_constbuffer_size);

  FILL_ENTRY(create_pixel_shader);
  FILL_ENTRY(create_pixel_shader_dagor);
  FILL_ENTRY(delete_pixel_shader);

  FILL_ENTRY(set_pixel_shader);

  FILL_ENTRY(request_sampler);
  FILL_ENTRY(set_sampler);
  FILL_ENTRY2(register_bindless_sampler_0, register_bindless_sampler);
  FILL_ENTRY2(register_bindless_sampler_1, register_bindless_sampler);

  FILL_ENTRY(allocate_bindless_resource_range);
  FILL_ENTRY(resize_bindless_resource_range);
  FILL_ENTRY(free_bindless_resource_range);
  FILL_ENTRY(update_bindless_resource);
  FILL_ENTRY(update_bindless_resources_to_null);

  FILL_ENTRY(set_tex);
  FILL_ENTRY(set_rwtex);
  FILL_ENTRY(clear_rwtexi);
  FILL_ENTRY(clear_rwtexf);
  FILL_ENTRY(clear_rwbufi);
  FILL_ENTRY(clear_rwbuff);

  FILL_ENTRY(clear_rt);

  FILL_ENTRY(set_buffer);
  FILL_ENTRY(set_rwbuffer);
  FILL_ENTRY(set_const_buffer);

  FILL_ENTRY2(create_vb_0, create_vb);
  FILL_ENTRY(create_ib);
  FILL_ENTRY(create_sbuffer);

  FILL_ENTRY2(set_depth_0, set_depth);
  FILL_ENTRY2(set_depth_1, set_depth);
  FILL_ENTRY2(set_render_target_0, set_render_target);
  FILL_ENTRY2(set_render_target_1, set_render_target);
  FILL_ENTRY2(set_render_target_2, set_render_target);
  FILL_ENTRY2(set_render_target_3, set_render_target);
  FILL_ENTRY(get_render_target);
  FILL_ENTRY(get_target_size);
  FILL_ENTRY(get_render_target_size);
  FILL_ENTRY(set_variable_rate_shading);
  FILL_ENTRY(set_variable_rate_shading_texture);

  FILL_ENTRY2(settm_0, settm);
  FILL_ENTRY2(settm_1, settm);
  FILL_ENTRY2(settm_2, settm);
  FILL_ENTRY2(gettm_0, gettm);
  FILL_ENTRY2(gettm_1, gettm);
  FILL_ENTRY2(gettm_2, gettm);
  FILL_ENTRY(gettm_cref);
  FILL_ENTRY(getm2vtm);
  FILL_ENTRY(setglobtm);
  FILL_ENTRY(getglobtm);
  FILL_ENTRY(setpersp);
  FILL_ENTRY(getpersp);
  FILL_ENTRY(validatepersp);
  FILL_ENTRY2(calcproj_0, calcproj);
  FILL_ENTRY2(calcproj_1, calcproj);
  FILL_ENTRY2(calcglobtm_0, calcglobtm);
  FILL_ENTRY2(calcglobtm_1, calcglobtm);
  FILL_ENTRY2(calcglobtm_2, calcglobtm);
  FILL_ENTRY2(calcglobtm_3, calcglobtm);

  FILL_ENTRY2(getglobtm_vec, getglobtm);
  FILL_ENTRY2(setglobtm_vec, setglobtm);

  FILL_ENTRY(setscissor);
  FILL_ENTRY(setscissors);

  FILL_ENTRY(setview);
  FILL_ENTRY(setviews);
  FILL_ENTRY(getview);
  FILL_ENTRY(clearview);

  FILL_ENTRY(update_screen);
  FILL_ENTRY(wait_for_async_present);
  FILL_ENTRY(gpu_latency_wait);

  FILL_ENTRY(setvsrc_ex);
  FILL_ENTRY(setind);
  FILL_ENTRY(create_vdecl);
  FILL_ENTRY(delete_vdecl);
  FILL_ENTRY(setvdecl);

  FILL_ENTRY(draw_base);
  FILL_ENTRY(drawind_base);
  FILL_ENTRY(draw_up);
  FILL_ENTRY(drawind_up);
  FILL_ENTRY(draw_indirect);
  FILL_ENTRY(draw_indexed_indirect);
  FILL_ENTRY(multi_draw_indirect);
  FILL_ENTRY(multi_draw_indexed_indirect);
  FILL_ENTRY(dispatch);
  FILL_ENTRY(dispatch_indirect);
  FILL_ENTRY(dispatch_mesh);
  FILL_ENTRY(dispatch_mesh_indirect);

  FILL_ENTRY(insert_fence);
  FILL_ENTRY(insert_wait_on_fence);

  FILL_ENTRY(create_render_state);
  FILL_ENTRY(set_render_state);
  FILL_ENTRY(clear_render_states);

  FILL_ENTRY(set_blend_factor);
  FILL_ENTRY(setstencil);

  FILL_ENTRY(setwire);

  FILL_ENTRY(set_srgb_backbuffer_write);

  FILL_ENTRY(setgamma);

  FILL_ENTRY(get_screen_aspect_ratio);
  FILL_ENTRY(change_screen_aspect_ratio);

  FILL_ENTRY(fast_capture_screen);
  FILL_ENTRY(end_fast_capture_screen);

  FILL_ENTRY(capture_screen);
  FILL_ENTRY(release_capture_buffer);

  FILL_ENTRY(get_screen_size);

  FILL_ENTRY(set_depth_bounds);

  FILL_ENTRY(create_predicate);
  FILL_ENTRY(free_predicate);

  FILL_ENTRY(begin_survey);
  FILL_ENTRY(end_survey);

  FILL_ENTRY(begin_conditional_render);
  FILL_ENTRY(end_conditional_render);

  FILL_ENTRY(get_program_vdecl);

  FILL_ENTRY(get_vrr_supported);
  FILL_ENTRY(get_vsync_enabled);
  FILL_ENTRY(enable_vsync);
  FILL_ENTRY(get_video_modes_list);

  FILL_ENTRY(create_event_query);
  FILL_ENTRY(release_event_query);
  FILL_ENTRY(issue_event_query);
  FILL_ENTRY(get_event_query_status);
  FILL_ENTRY(get_debug_program);
  FILL_ENTRY(beginEvent);
  FILL_ENTRY(endEvent);
  FILL_ENTRY(get_backbuffer_tex);
  FILL_ENTRY(get_secondary_backbuffer_tex);

#if D3D_HAS_RAY_TRACING
  // raytrace interface ->
  FILL_ENTRY2(create_raytrace_bottom_acceleration_structure_0, create_raytrace_bottom_acceleration_structure);
  FILL_ENTRY2(create_raytrace_bottom_acceleration_structure_1, create_raytrace_bottom_acceleration_structure);
  FILL_ENTRY(delete_raytrace_bottom_acceleration_structure);
  FILL_ENTRY(create_raytrace_top_acceleration_structure);
  FILL_ENTRY(delete_raytrace_top_acceleration_structure);
  FILL_ENTRY(set_top_acceleration_structure);
  FILL_ENTRY(build_bottom_acceleration_structure);
  FILL_ENTRY(build_bottom_acceleration_structures);
  FILL_ENTRY(build_top_acceleration_structure);
  FILL_ENTRY(build_top_acceleration_structures);
  FILL_ENTRY(write_raytrace_index_entries_to_memory);
  FILL_ENTRY(get_raytrace_acceleration_structure_size);
  FILL_ENTRY(get_raytrace_acceleration_structure_gpu_handle);
  FILL_ENTRY(copy_raytrace_acceleration_structure);

  FILL_NAMESPACE_ENTRY(raytrace, check_vertex_format_support_for_acceleration_structure_build);
  FILL_NAMESPACE_ENTRY(raytrace, create_pipeline);
  FILL_NAMESPACE_ENTRY(raytrace, expand_pipeline);
  FILL_NAMESPACE_ENTRY(raytrace, destroy_pipeline);
  FILL_NAMESPACE_ENTRY(raytrace, get_shader_binding_table_buffer_properties);
  FILL_NAMESPACE_ENTRY(raytrace, dispatch);
  FILL_NAMESPACE_ENTRY(raytrace, dispatch_indirect);
  FILL_NAMESPACE_ENTRY(raytrace, dispatch_indirect_count);
  // <- raytrace interface
#endif

  FILL_ENTRY(resource_barrier);

  FILL_ENTRY(create_render_pass);
  FILL_ENTRY(delete_render_pass);
  FILL_ENTRY(begin_render_pass);
  FILL_ENTRY(next_subpass);
  FILL_ENTRY(end_render_pass);

  FILL_ENTRY(allow_render_pass_target_load);

  FILL_ENTRY(get_resource_allocation_properties);
  FILL_ENTRY(create_resource_heap);
  FILL_ENTRY(destroy_resource_heap);
  FILL_ENTRY(place_buffer_in_resource_heap);
  FILL_ENTRY(place_texture_in_resource_heap);
  FILL_ENTRY(get_resource_heap_group_properties);
  FILL_ENTRY(map_tile_to_resource);
  FILL_ENTRY(get_texture_tiling_info);
  FILL_ENTRY(activate_buffer);
  FILL_ENTRY(activate_texture);
  FILL_ENTRY(deactivate_buffer);
  FILL_ENTRY(deactivate_texture);

  FILL_ENTRY(allocate_update_buffer_for_tex_region);
  FILL_ENTRY(allocate_update_buffer_for_tex);
  FILL_ENTRY(release_update_buffer);
  FILL_ENTRY(get_update_buffer_addr_for_write);
  FILL_ENTRY(get_update_buffer_size);
  FILL_ENTRY(get_update_buffer_pitch);
  FILL_ENTRY(get_update_buffer_slice_pitch);
  FILL_ENTRY(update_texture_and_release_update_buffer);

  FILL_ENTRY(create_shader_library);
  FILL_ENTRY(destroy_shader_library);

  FILL_ENTRY(start_capture);
  FILL_ENTRY(stop_capture);

  FILL_ENTRY_PC(set_present_wnd);

  FILL_ENTRY_PC(set_capture_full_frame_buffer);
  FILL_ENTRY_PC(get_texture_format);
  FILL_ENTRY_PC(get_texture_format_str);

  FILL_ENTRY_PC(get_native_surface);

  d3dit.driverCode = DRV3D_CODE;
  d3dit.driverName = d3d::get_driver_name();
  d3dit.driverVer = d3d::get_device_driver_version();
  d3dit.deviceName = d3d::get_device_name();
  memset(&d3dit.drvDesc, 0, sizeof(d3dit.drvDesc));

  return true;
}

#else
bool d3d::fill_interface_table(D3dInterfaceTable &) { return false; }
#endif
