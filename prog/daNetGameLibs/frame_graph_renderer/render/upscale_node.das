options persistent_heap

require DagorShaders
require danetlibs/renderer/includes/upscale_sampling
require ecs
require daFrameGraph.fg_ecs

let upscale_tex_gen_node_entity = "upscale_node"

[fg_ecs_node(on_appear)]
def register_upscale_tex_gen_node(var upscale_tex_gen_node : NodeHandle&)
  upscale_tex_gen_node <- root() |> registerNode("upscale_tex_gen_node") <| @(var registry : Registry)
    registry |> read("gbuf_depth") |> texture |> atStage(Stage.POST_RASTER) |> bindToShaderVar("depth_gbuf")
    registry |> read("gbuf_1") |> texture |> optional |> atStage(Stage.POST_RASTER) |> bindToShaderVar("normal_gbuf")

    registry |> read("checkerboard_depth") |> texture |> optional |> atStage(Stage.POST_RASTER) |> bindToShaderVar("downsampled_checkerboard_depth_tex")
    registry |> read("checkerboard_depth_sampler") |> blob(type<SamplerHandle>) |> optional |> bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")

    registry |> read("close_depth") |> texture |> optional |> atStage(Stage.POST_RASTER) |> bindToShaderVar("downsampled_close_depth_tex")
    registry |> read("close_depth_sampler") |> blob(type<SamplerHandle>) |> optional |> bindToShaderVar("downsampled_close_depth_tex_samplerstate")
    registry |> read("downsampled_normals") |> texture |> optional |> atStage(Stage.POST_RASTER) |> bindToShaderVar("downsampled_normals")
    registry |> read("downsampled_normals_sampler") |> blob(type<SamplerHandle>) |> optional |> bindToShaderVar("downsampled_normals_samplerstate")

    let upscaleInfo = Texture2dCreateInfo(
      creationFlags = TEXCF_RTARGET | TEXFMT_R8UI,
      resolution = TextureResolution2(autoRes = registry |> getResolution`2("main_view")),
      mipLevels = 1u
    )
    var upscaleTexReq = registry |> create("upscale_sampling_tex", History.No) |> texture(upscaleInfo)
    registry |> requestRenderPass |> color(fixed_array(upscaleTexReq))

    return <- @ <|
      query() <| $ [es] (upscale_shader : PostFxRenderer)
        upscale_shader |> render()

[cpp_event(broadcast)]
struct OnRenderSettingsReady {}

[cpp_event(broadcast)]
struct AfterDeviceReset
  fullReset : bool

[es(on_event=AfterDeviceReset)]
def restoreWeights(evt : Event)
  query() <| $ [es] (upscale_weights_buffer : UniqueBufHolder)
    upload_weights(upscale_weights_buffer)

[es(on_event=OnRenderSettingsReady, track=render_settings__upscaleSamplingTex)]
def init_upscale_node(evt : Event; render_settings__upscaleSamplingTex : bool; render_settings__fullDeferred : bool; render_settings__bare_minimum : bool)
  if render_settings__upscaleSamplingTex
    createEntitySync(upscale_tex_gen_node_entity) //we should create this entity sync, because framegraph requere to know about this node immediately

    query() <| $ [es] (var upscale_weights_buffer : UniqueBufHolder)
      init_upscale(upscale_weights_buffer)
      upload_weights(upscale_weights_buffer)
  else
    destroyEntity(getSingletonEntity(upscale_tex_gen_node_entity))
    if render_settings__fullDeferred && !render_settings__bare_minimum
      logerr("upscale_node is mandatory for all presets except bareMinimum, nswitch, mobile")
