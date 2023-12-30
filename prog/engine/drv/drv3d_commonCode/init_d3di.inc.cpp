#include <3d/dag_drv3di.h>

#if _TARGET_PC
#include <3d/dag_drv3d_pc.h>
#include <3d/dag_drv3dCmd.h>
#include <debug/dag_fatal.h>

static void na_func() { DAG_FATAL("D3DI function not implemented"); }

#define FILL_ENTRY(X)     d3dit.X = d3d::X
#define FILL_ENTRY2(X, Y) d3dit.X = d3d::Y
#if _TARGET_PC_WIN
#define FILL_ENTRY_PC(X) d3dit.X = d3d::pcwin32::X
#else
#define FILL_ENTRY_PC(X)
#endif

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
  FILL_ENTRY(issame_cubetexformat);
  FILL_ENTRY(check_voltexformat);
  FILL_ENTRY(issame_voltexformat);
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

  FILL_ENTRY(set_tex_usage_hint);
  FILL_ENTRY(discard_managed_textures);
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

  FILL_ENTRY(create_sampler);
  FILL_ENTRY(destroy_sampler);
  FILL_ENTRY(set_sampler);
  FILL_ENTRY(register_bindless_sampler);

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

  FILL_ENTRY(set_buffer);
  FILL_ENTRY(set_rwbuffer);
  FILL_ENTRY(set_const_buffer);

  FILL_ENTRY2(create_vb_0, create_vb);
  FILL_ENTRY(create_ib);
  FILL_ENTRY(create_sbuffer);

  FILL_ENTRY2(set_depth_0, set_depth);
  FILL_ENTRY2(set_depth_1, set_depth);
  FILL_ENTRY(set_backbuf_depth);
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

  FILL_ENTRY(setantialias);
  FILL_ENTRY(getantialias);

  FILL_ENTRY(create_render_state);
  FILL_ENTRY(set_render_state);
  FILL_ENTRY(clear_render_states);

  FILL_ENTRY(set_blend_factor);
  FILL_ENTRY(setstencil);

  FILL_ENTRY(setwire);

  FILL_ENTRY(set_srgb_backbuffer_write);

  FILL_ENTRY(set_msaa_pass);

  FILL_ENTRY(set_depth_resolve);

  FILL_ENTRY(setgamma);

  FILL_ENTRY(isVcolRgba);

  FILL_ENTRY(get_screen_aspect_ratio);
  FILL_ENTRY(change_screen_aspect_ratio);

  FILL_ENTRY(fast_capture_screen);
  FILL_ENTRY(end_fast_capture_screen);

  FILL_ENTRY(capture_screen);
  FILL_ENTRY(release_capture_buffer);

  FILL_ENTRY(get_screen_size);

  FILL_ENTRY(set_depth_bounds);
  FILL_ENTRY(supports_depth_bounds);

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
  FILL_ENTRY(get_backbuffer_tex_depth);

#if D3D_HAS_RAY_TRACING
  // raytrace interface ->
  FILL_ENTRY(create_raytrace_bottom_acceleration_structure);
  FILL_ENTRY(delete_raytrace_bottom_acceleration_structure);
  FILL_ENTRY(create_raytrace_top_acceleration_structure);
  FILL_ENTRY(delete_raytrace_top_acceleration_structure);
  FILL_ENTRY(set_top_acceleration_structure);
  FILL_ENTRY(create_raytrace_program);
  FILL_ENTRY(create_raytrace_shader);
  FILL_ENTRY(delete_raytrace_shader);
  FILL_ENTRY(trace_rays);
  FILL_ENTRY(build_bottom_acceleration_structure);
  FILL_ENTRY(build_top_acceleration_structure);
  FILL_ENTRY(copy_raytrace_shader_handle_to_memory);
  FILL_ENTRY(write_raytrace_index_entries_to_memory);
// <- raytrace interface
#endif

  FILL_ENTRY(resource_barrier);

  FILL_ENTRY(create_render_pass);
  FILL_ENTRY(delete_render_pass);
  FILL_ENTRY(begin_render_pass);
  FILL_ENTRY(next_subpass);
  FILL_ENTRY(end_render_pass);

  FILL_ENTRY(get_resource_allocation_properties);
  FILL_ENTRY(create_resource_heap);
  FILL_ENTRY(destroy_resource_heap);
  FILL_ENTRY(place_buffere_in_resource_heap);
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
