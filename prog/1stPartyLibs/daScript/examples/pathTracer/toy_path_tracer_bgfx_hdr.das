require glfw/glfw_boost
require dasbgfx/bgfx_boost
require daslib/defer
require daslib/math_boost
require daslib/safe_addr
require daslib/random
require daslib/unroll

require path_tracer

let TRACE_WIDTH = 1024
let TRACE_HEIGHT = 1024
let BLOOM_SCALE = 4

var [[in]] a_position : float2
var [[in]] a_texcoord0 : float2
var [[inout]] v_texcoord0 : float2
var [[uniform, stage=0]] f_tex : sampler2D
var [[uniform, stage=1]] f_bloom : sampler2D
var [[uniform, format=rgba32f, stage=0]] c_destTex : image2D
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
    let color = texture2D(f_tex,v_texcoord0)
    let bloom = texture2D(f_bloom,v_texcoord0)
    let res = bloom + color
    gl_FragColor = linear_to_RGB(res.xyz).zyxx

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

var [[uniform, format=rgba32f, stage=0]] c_bloom_source : image2D
var [[uniform, format=rgba32f, stage=1]] c_bloom_vtex : image2D
var [[uniform, format=rgba32f, stage=2]] c_bloom_htex : image2D
var [[uniform, need_init=false]] c_bloom_weights : float4[9]
var [[uniform]] c_bloom_iweights_summ : float4

def gaussian(x, sigma:float)
    return ( 1. / ( sqrt(2.*PI) * sigma ) ) * exp( -(x*x) / (2.*sigma*sigma) )

[bgfx_compute_program(version=430,local_size_x=16,local_size_y=16)]
def bloom_effect_copy
    let xy = int2(gl_GlobalInvocationID.xy)
    // copy
    let XY = xy * int2(BLOOM_SCALE)
    var orgb : float4
    unroll <|
        for by in range(BLOOM_SCALE)
            unroll <|
                for bx in range(BLOOM_SCALE)
                    orgb = max(orgb, imageLoad(c_bloom_source, XY + int2(bx,by)))
    orgb = pow ( orgb, float4(3.))
    orgb = saturate(orgb)
    imageStore(c_bloom_vtex,xy,orgb)

[bgfx_compute_program(local_size_x=16,local_size_y=16)]
def bloom_effect_v
    let xy = int2(gl_GlobalInvocationID.xy)
    // blur v
    var n : float4[9]
    unroll <|
        for i in range(9)
	        n[i] = imageLoad(c_bloom_vtex, xy + int2(i-4,0))
    var hrgb : float4
    for a,w in n,c_bloom_weights
        hrgb += a * w
    hrgb *= c_bloom_iweights_summ
    imageStore(c_bloom_htex,xy,hrgb)

[bgfx_compute_program(local_size_x=16,local_size_y=16)]
def bloom_effect_h
    let xy = int2(gl_GlobalInvocationID.xy)
    // blur h
    var n : float4[9]
    unroll <|
        for i in range(9)
	        n[i] = imageLoad(c_bloom_htex, xy + int2(0,i-4))
    var vrgb : float4
    for a,w in n,c_bloom_weights
        vrgb += a * w
    vrgb *= c_bloom_iweights_summ
    imageStore(c_bloom_vtex,xy,vrgb)

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
    var window = glfwCreateWindow(TRACE_WIDTH, TRACE_HEIGHT, "BGFX Path tracer with bloom", null, null)
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
    glfwSetWindowTitle(window, "BGFX Path tracer with bloom - {bgfx_get_renderer_type()}")
    var compute_program = bgfx_create_compute_shader_program(@@compute_effect)
    var display_program = bgfx_create_shader_program(@@vs_effect,@@fs_effect)
    var pcvl_decl = bgfx_create_vertex_layout(type<EffectVertex>)
    var vbh = bgfx_create_vertex_buffer(bgfx_make_ref(effect_vertices), safe_addr(pcvl_decl), BGFX_BUFFER_NONE)
    var ibh = bgfx_create_index_buffer(bgfx_make_ref(effect_indices), BGFX_BUFFER_INDEX32)
    var texture = bgfx_create_texture_2d(uint(TRACE_WIDTH), uint(TRACE_HEIGHT), false, 1u,
        bgfx_texture_format RGBA32F, BGFX_TEXTURE_COMPUTE_WRITE, null)
    if texture.idx==uint16(0xffff)
        panic("can't create texture")
    var bloom_program_copy = bgfx_create_compute_shader_program(@@bloom_effect_copy)
    var bloom_program_h = bgfx_create_compute_shader_program(@@bloom_effect_h)
    var bloom_program_v = bgfx_create_compute_shader_program(@@bloom_effect_v)
    var bloom_texture : bgfx_texture_handle_s[2]
    for b in bloom_texture
        b = bgfx_create_texture_2d(uint(TRACE_WIDTH/BLOOM_SCALE), uint(TRACE_HEIGHT/BLOOM_SCALE), false, 1u,
            bgfx_texture_format RGBA32F, BGFX_TEXTURE_COMPUTE_WRITE, null)
        if b.idx==uint16(0xffff)
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
        // copmute next iteration of tracing
        c_frame.x = float(cframe)
        c_frame.y = int_bits_to_float(cframe)
        cframe ++
        c_destTex := texture
        compute_effect_bind_uniform()
        bgfx_dispatch(0u, compute_program, uint(TRACE_WIDTH/16), uint(TRACE_HEIGHT/16), 1u, 0u)
        // bloom weights
        let sigma = 4.0
        for t in range(9)
            c_bloom_weights[t] = float4(gaussian(float(t-4),sigma))
        c_bloom_iweights_summ = float4(0.)
        for w in c_bloom_weights
            c_bloom_iweights_summ += w
        c_bloom_iweights_summ = 1.0 / c_bloom_iweights_summ
        // bloom copy
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_copy_bind_uniform()
        bgfx_dispatch(0u, bloom_program_copy, uint(TRACE_WIDTH/16/BLOOM_SCALE), uint(TRACE_HEIGHT/16/BLOOM_SCALE), 1u, 0u)
        // bloom v
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_v_bind_uniform()
        bgfx_dispatch(0u, bloom_program_v, uint(TRACE_WIDTH/16/BLOOM_SCALE), uint(TRACE_HEIGHT/16/BLOOM_SCALE), 1u, 0u)
        // bloom h
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_h_bind_uniform()
        bgfx_dispatch(0u, bloom_program_h, uint(TRACE_WIDTH/16/BLOOM_SCALE), uint(TRACE_HEIGHT/16/BLOOM_SCALE), 1u, 0u)
        // display it
        bgfx_set_vertex_buffer(0u, vbh)
        bgfx_set_index_buffer(ibh)
        f_tex := texture
        f_bloom := c_bloom_vtex
        vs_effect_bind_uniform()
        fs_effect_bind_uniform()
        bgfx_set_state(BGFX_STATE_DEFAULT, 0u)
        bgfx_submit(0u, display_program, 1u, BGFX_DISCARD_NONE)
        bgfx_frame(true)
    bgfx_shutdown()
