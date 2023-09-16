require glfw/glfw_boost

require opengl/opengl_boost

require glsl/glsl_opengl

require daslib/defer
require daslib/unroll
require math

require daslib/safe_addr
require daslib/random

require path_tracer

let TRACE_WIDTH = 1024
let TRACE_HEIGHT = 1024
let BLOOM_SCALE = 4

var [[in, location=0]] v_position : float2
var [[in, location=1]] v_texcoord : float2
var [[inout]] f_color : float3
var [[inout]] f_texcoord : float2
var [[uniform, stage=0]] f_tex : sampler2D
var [[uniform, stage=1]] f_bloom : sampler2D
var [[uniform, format=rgba32f, stage=0]] c_destTex : image2D
var [[uniform]] c_frame : int = 0
var [[out]] f_FragColor : float4

[vertex_program]
def vs_effect
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)

def linear_to_RGB ( color : float3 )
    let x = float4(max(color, float3(0)), 1.0)
    return max(1.055 * pow(x, float4(0.416666667)) - float4(0.055), float4(0))

[fragment_program]
def fs_effect
    let color = texture2D(f_tex,f_texcoord)
    let bloom = texture2D(f_bloom,f_texcoord)
    let res = bloom + color
    f_FragColor = linear_to_RGB(res.xyz).zyxx

[compute_program(version=430,local_size_x=16,local_size_y=16)]
def compute_effect
    if gl_LocalInvocationIndex==0u
        init_path_tracer()
    memoryBarrierShared()
    let storePos = int2(gl_GlobalInvocationID.xy)
    random_seed2D(RAND_SEED,storePos,c_frame)
    random_float4(RAND_SEED)
    let rgb = draw_ray(storePos.x,TRACE_HEIGHT-storePos.y,TRACE_WIDTH,TRACE_HEIGHT)
    if true
        var nrgb = rgb.zyxx
        if c_frame!=0
            let orgb = imageLoad(c_destTex, storePos)
            let wf = float(c_frame)
            nrgb = (orgb*wf + nrgb) / (wf + 1.)
        imageStore(c_destTex,storePos, nrgb)
    else
        imageStore(c_destTex,storePos, rgb.zyxx)

var compute_program, display_program : uint
var vao : uint
var vbo : uint
var ebo : uint
var texture : uint

var [[uniform, format=rgba32f, stage=0]] c_bloom_source : image2D
var [[uniform, format=rgba32f, stage=1]] c_bloom_vtex : image2D
var [[uniform, format=rgba32f, stage=2]] c_bloom_htex : image2D
var [[uniform]] c_bloom_weights : float[9]
var [[uniform]] c_bloom_iweights_summ : float

def gaussian(x, sigma:float)
    return ( 1. / ( sqrt(2.*PI) * sigma ) ) * exp( -(x*x) / (2.*sigma*sigma) )

[compute_program(local_size_x=16,local_size_y=16)]
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

[compute_program(local_size_x=16,local_size_y=16)]
def bloom_effect_v
    let xy = int2(gl_GlobalInvocationID.xy)
    // blur v
    var n : float4[9]
    unroll <|
        for i in range(9)
	        n[i] = imageLoad(c_bloom_vtex, xy + int2(i-4,0))
    var hrgb : float4
    for a,w in n,c_bloom_weights
        hrgb += a * float4(w)
    hrgb *= c_bloom_iweights_summ
    imageStore(c_bloom_htex,xy,hrgb)

[compute_program(local_size_x=16,local_size_y=16)]
def bloom_effect_h
    let xy = int2(gl_GlobalInvocationID.xy)
    // blur h
    var n : float4[9]
    unroll <|
        for i in range(9)
	        n[i] = imageLoad(c_bloom_htex, xy + int2(0,i-4))
    var vrgb : float4
    for a,w in n,c_bloom_weights
        vrgb += a * float4(w)
    vrgb *= c_bloom_iweights_summ
    imageStore(c_bloom_vtex,xy,vrgb)

var bloom_program_copy, bloom_program_h, bloom_program_v : uint
var bloom_texture : uint[2]

[vertex_buffer]
struct EffectVertex
    xy : float2
    uv : float2

let effect_vertices = [[EffectVertex
    xy=float2(-1.0, 1.0), uv=float2(0.0,0.0);
    xy=float2( 1.0, 1.0), uv=float2(1.0,0.0);
    xy=float2( 1.0,-1.0), uv=float2(1.0,1.0);
    xy=float2(-1.0,-1.0), uv=float2(0.0,1.0)
]]

let effect_indices = [[int 0; 1; 2; 2; 3; 0]]

def create_gl_objects
    compute_program = create_compute_shader_program(@@compute_effect)
    display_program = create_shader_program(@@vs_effect,@@fs_effect)
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao);
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, effect_vertices, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<EffectVertex>)
    glGenBuffers(1, safe_addr(ebo))
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, effect_indices, GL_STATIC_DRAW)
    // image TRACE_WIDTH*TRACE_HEIGHT RGBA32F
    glGenTextures(1, safe_addr(texture))
    glBindTexture(GL_TEXTURE_2D, texture)
    glTexImage2D(GL_TEXTURE_2D, 0, int(GL_RGBA32F), TRACE_WIDTH, TRACE_HEIGHT, 0, GL_RGBA, GL_FLOAT, null)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    bloom_program_copy = create_compute_shader_program(@@bloom_effect_copy)
    bloom_program_h = create_compute_shader_program(@@bloom_effect_h)
    bloom_program_v = create_compute_shader_program(@@bloom_effect_v)
    // bloom texture
    for btexture in bloom_texture
        glGenTextures(1, safe_addr(btexture))
        glBindTexture(GL_TEXTURE_2D, btexture)
        glTexImage2D(GL_TEXTURE_2D, 0, int(GL_RGBA32F), TRACE_WIDTH/BLOOM_SCALE, TRACE_HEIGHT/BLOOM_SCALE, 0, GL_RGBA, GL_FLOAT, null)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)

[export]
def main
    if glfwInit()==0
		panic("can't init glfw")
    defer <|
        glfwTerminate()
    glfwInitOpenGL(4,3)
    var window = glfwCreateWindow(TRACE_WIDTH, TRACE_HEIGHT, "OpenGL Path tracer with bloom", null, null)
    if window==null
		panic("can't create window")
    defer <|
        glfwDestroyWindow(window)
    glfwSetWindowSize(window,TRACE_WIDTH,TRACE_HEIGHT)
    glfwMakeContextCurrent(window)
    create_gl_objects()
    while glfwWindowShouldClose(window)==0
        glfwPollEvents()
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        // copmute next iteration of tracing
        glUseProgram(compute_program)
        c_destTex := texture
        compute_effect_bind_uniform(compute_program)
        glDispatchCompute(uint(TRACE_WIDTH/16),uint(TRACE_HEIGHT/16),1u)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)
        c_frame++
        // bloom weights
        let sigma = 4.0
        c_bloom_weights = [[float[9]
            gaussian(-4., sigma);   gaussian(-3., sigma);   gaussian(-2., sigma);   gaussian(-1., sigma);
            gaussian( 0., sigma);
            gaussian( 1., sigma);   gaussian( 2., sigma);   gaussian( 3., sigma);   gaussian( 4., sigma)
        ]]
        c_bloom_iweights_summ = 0.
        for w in c_bloom_weights
            c_bloom_iweights_summ += w
        c_bloom_iweights_summ = 1.0 / c_bloom_iweights_summ
        // bloom copy
        glUseProgram(bloom_program_copy)
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_copy_bind_uniform(bloom_program_copy)
        glDispatchCompute(uint(TRACE_WIDTH/16/BLOOM_SCALE),uint(TRACE_HEIGHT/16/BLOOM_SCALE),1u)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)
        // bloom v
        glUseProgram(bloom_program_v)
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_v_bind_uniform(bloom_program_v)
        glDispatchCompute(uint(TRACE_WIDTH/16/BLOOM_SCALE),uint(TRACE_HEIGHT/16/BLOOM_SCALE),1u)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)
        // bloom h
        glUseProgram(bloom_program_h)
        c_bloom_source := texture
        c_bloom_vtex := bloom_texture[0]
        c_bloom_htex := bloom_texture[1]
        bloom_effect_h_bind_uniform(bloom_program_h)
        glDispatchCompute(uint(TRACE_WIDTH/16/BLOOM_SCALE),uint(TRACE_HEIGHT/16/BLOOM_SCALE),1u)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)
        // display it
        glViewport(0, 0, display_w, display_h)
        glUseProgram(display_program)
        f_tex := texture
        f_bloom := c_bloom_vtex
        vs_effect_bind_uniform(display_program)
        fs_effect_bind_uniform(display_program)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
