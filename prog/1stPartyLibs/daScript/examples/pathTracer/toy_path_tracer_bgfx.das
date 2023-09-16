require glfw/glfw_boost
require dasbgfx/bgfx_boost
require daslib/defer
require daslib/math_boost
require daslib/safe_addr
require daslib/random

require path_tracer

let TRACE_WIDTH = 1024
let TRACE_HEIGHT = 1024

var [[in]] a_position : float2
var [[in]] a_texcoord0 : float2
var [[inout]] v_texcoord0 : float2
var [[uniform, stage=0]] f_tex : sampler2D
var [[uniform, format=rgba32f, stage=0, readwrite]] c_destTex : image2D
var [[uniform]] c_frame : float4

[bgfx_vertex_program]
def vs_effect
    v_texcoord0 = a_texcoord0
    gl_Position = float4(a_position, 0.0, 1.0)

def linear_to_RGB ( color : float3 )
    let x = float4(max(color, float3(0)), 1.0)
    return max(1.055 * pow(x, float4(0.416666667)) - float4(0.055), float4(0))

[bgfx_fragment_program]
def fs_effect
    gl_FragColor = linear_to_RGB(texture2D(f_tex,v_texcoord0).xyz).zyxx

[bgfx_compute_program(local_size_x=16,local_size_y=16)]
def compute_effect
    if gl_LocalInvocationIndex==0u
        init_path_tracer()
    memoryBarrierShared()
    let cframe = float_bits_to_int(c_frame.y)
    let storePos = int2(gl_GlobalInvocationID.xy)
    random_seed2D(RAND_SEED,storePos,cframe)
    random_float4(RAND_SEED)
    var rgb = draw_ray(storePos.x,TRACE_HEIGHT-storePos.y,TRACE_WIDTH,TRACE_HEIGHT)
    if true
        var nrgb = rgb.zyxx
        if cframe!=0
            let orgb = imageLoad(c_destTex, storePos)
            let wf = float(cframe)
            nrgb = (orgb*wf + nrgb) / (wf + 1.)
        imageStore(c_destTex,storePos, nrgb)
    else
        imageStore(c_destTex,storePos, rgb.zyxx)

[bgfx_vertex_buffer]
struct EffectVertex
    position  : float2
    texcoord0 : float2

let effect_vertices = [[EffectVertex
    position=float2(-1.0, 1.0), texcoord0=float2(0.0,0.0);
    position=float2( 1.0, 1.0), texcoord0=float2(1.0,0.0);
    position=float2( 1.0,-1.0), texcoord0=float2(1.0,1.0);
    position=float2(-1.0,-1.0), texcoord0=float2(0.0,1.0)
]]

let effect_indices = [[int 0; 2; 1; 2; 0; 3]]

[export]
def main
    if glfwInit()==0
		panic("can't init glfw")
    defer <|
        glfwTerminate()
    var window = glfwCreateWindow(TRACE_WIDTH, TRACE_HEIGHT, "BGFX Path tracer", null, null)
    if window==null
		panic("can't create window")
    defer <|
        glfwDestroyWindow(window)
    glfwSetWindowSize(window,TRACE_WIDTH,TRACE_HEIGHT)
    var pd: bgfx_platform_data_s
    pd.nwh = glfwGetNativeWindow(window)
    bgfx_set_platform_data(safe_addr(pd))
    // bgfx_init(TRACE_WIDTH,TRACE_WIDTH,bgfx_renderer_type OPENGL)
    // bgfx_init(TRACE_WIDTH,TRACE_WIDTH,bgfx_renderer_type DIRECT3D12)
    bgfx_init(TRACE_WIDTH,TRACE_WIDTH)
    glfwSetWindowTitle(window, "BGFX Path tracer - {bgfx_get_renderer_type()}")
    var compute_program = bgfx_create_compute_shader_program(@@compute_effect)
    var display_program = bgfx_create_shader_program(@@vs_effect,@@fs_effect)
    var pcvl_decl = bgfx_create_vertex_layout(type<EffectVertex>)
    var vbh = bgfx_create_vertex_buffer(bgfx_make_ref(effect_vertices), safe_addr(pcvl_decl), BGFX_BUFFER_NONE)
    var ibh = bgfx_create_index_buffer(bgfx_make_ref(effect_indices), BGFX_BUFFER_INDEX32)
    var texture = bgfx_create_texture_2d(uint(TRACE_WIDTH), uint(TRACE_HEIGHT), false, 1u,
        bgfx_texture_format RGBA32F, BGFX_TEXTURE_COMPUTE_WRITE, null)
    if texture.idx==uint16(0xffff)
        panic("can't create texture")
    let v_ortho = bgfx_mat_ortho(-1.,1.,-1.,1.,-100.,100.,0.,bgfx_get_caps().homogeneousDepth,false)
    bgfx_set_view_clear(0u, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, RGBA_TO_BGFX_UCOLOR(0.2,0.2,0.2,1.0), 1.0f, 0u)
    var cframe = 0
    while glfwWindowShouldClose(window)==0
        glfwPollEvents()
        var display_w, display_h : int
        glfwGetWindowSize(window, safe_addr(display_w), safe_addr(display_h))
        bgfx_reset(uint(display_w), uint(display_h), BGFX_RESET_VSYNC, bgfx_texture_format COUNT)
        bgfx_set_view_rect(0u, 0u, 0u, uint(display_w), uint(display_h))
        c_frame.x = float(cframe)
        c_frame.y = int_bits_to_float(cframe)
        cframe ++
        c_destTex := texture
        compute_effect_bind_uniform()
        bgfx_dispatch(0u, compute_program, uint(TRACE_WIDTH/16), uint(TRACE_HEIGHT/16), 1u, 0u)
        bgfx_set_vertex_buffer(0u, vbh)
        bgfx_set_index_buffer(ibh)
        f_tex := texture
        vs_effect_bind_uniform()
        fs_effect_bind_uniform()
        bgfx_set_state(BGFX_STATE_DEFAULT, 0u)
        bgfx_submit(0u, display_program, 1u, BGFX_DISCARD_NONE)
        bgfx_frame(true)
    bgfx_shutdown()