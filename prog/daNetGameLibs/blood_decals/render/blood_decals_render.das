require ecs
require danetlibs.blood_decals.render.blood_decals_common
require danetlibs.renderer.includes.render_events
require ProjectiveDecals
require DagorMath
require DagorShaders
require DagorDriver3D
require DagorResPtr
require daProfiler

let FLOAT_SIZEOF = typeinfo sizeof(type<float>)

[es(tag=gameClient, on_appear)]
def blood_decals_init(evt : Event;
                      var blood_decals_manager : RingBufferDecals;
                      var blood_decals_depth_atlas : UniqueTexHolder;
                      var blood_decals_traces_buf : UniqueBufHolder;
                      blood_decals_init_shader : string;
                      blood_decals_generator_shader : string;
                      blood_decals_render_shader : string;
                      blood_decals_culler_shader : string;
                      blood_decals_decals_max_count : int;
                      blood_decals_splash_appear_time : float;
                      blood_decals_splash_stay_fresh_time : float;
                      blood_decals_splash_drying_time : float)


  blood_decals_manager |> init(
    blood_decals_generator_shader,
    blood_decals_render_shader,
    blood_decals_culler_shader,
    blood_decals_init_shader)

  blood_decals_manager |> init_buffer(uint(blood_decals_decals_max_count))

  blood_decals_traces_buf |> create_sbuffer(FLOAT_SIZEOF, TRACE_RESULT_BUFFER_SIZE, SBCF_CB_ONE_FRAME, 0u, "blood_decals_traces_result")

  let depthAtlasFlags = int(TEXFMT_DEPTH16 | TEXCF_RTARGET)
  blood_decals_depth_atlas |> create_tex(DEPTH_ATLAS_SIZE, DEPTH_ATLAS_SIZE, depthAtlasFlags, 1, "blood_decals_depth_atlas_tex")
  let sampler = d3d_request_sampler() <| $(var smpInfo : SamplerInfo#)
    smpInfo.filter_mode = FilterMode Compare
  DagorShaders::set_sampler(get_shader_variable_id("blood_decals_depth_atlas_tex_samplerstate", true), sampler)


  set_color4(get_shader_variable_id("blood_decal_timings", true),
    safeinv(blood_decals_splash_appear_time),
    -(blood_decals_splash_appear_time + blood_decals_splash_stay_fresh_time),
    safeinv(blood_decals_splash_drying_time),
    0f)



[es(tag=gameClient, after=animchar_before_render_es)] //require for execute animchar_before_render_es as early as possible
def blood_decals_prepare_render(stg : UpdateStageInfoBeforeRender;
                                var blood_decals_manager : RingBufferDecals;
                                var blood_decals_traces_result : ecs::FloatList;
                                blood_decals_traces_buf : UniqueBufHolder;
                                var blood_decals_traces_queue : ecs::IPoint2List;
                                blood_decals_depth_shader : PostFxRenderer;
                                blood_decals_depth_atlas : UniqueTexHolder)

  if !blood_decals_traces_queue |> empty

    d3d_set_render_target(null, uint8(0))
    d3d_set_depth(blood_decals_depth_atlas.getTex2D, DepthAccess.RW)
    exec_with_shader_blocks_scope_reset() <| $()
      blood_decals_depth_shader |> setStates()
      var tracesOffset = 0
      let tracesCount = TRACE_RESULT_BUFFER_SIZE
      for atlasPos in blood_decals_traces_queue
        *getBuf(blood_decals_traces_buf) |> updateData(blood_decals_traces_result, tracesOffset, tracesCount, uint(VBLOCK_DISCARD))
        d3d_setview(atlasPos.x, atlasPos.y, DEPTH_TEX_SIZE, DEPTH_TEX_SIZE, 0f, 1f)
        blood_decals_depth_shader |> render()
        tracesOffset += tracesCount
      d3d_resource_barrier(blood_decals_depth_atlas.getTex2D, RB_RO_SRV | RB_STAGE_PIXEL, 0u, 0u)
    clear(blood_decals_traces_result)
    clear(blood_decals_traces_queue)

  blood_decals_manager |> prepareRender(stg.mainCullingFrustum)



[es(tag=gameClient)]
def blood_decals_render(evt : OnRenderDecals; var blood_decals_manager : RingBufferDecals)

  profile_gpu_block("blood_decals_render") <| $()
    blood_decals_manager |> render()


[es(tag=gameClient)]
def blood_decals_after_reset(evt : AfterDeviceReset; var blood_decals_manager : RingBufferDecals; var blood_decals_decals_count : int&)

  blood_decals_decals_count = 0
  blood_decals_manager |> clear()
  blood_decals_manager |> afterReset()