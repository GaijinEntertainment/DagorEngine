require glfw/glfw_boost
require opengl/opengl_boost
require glsl/glsl_opengl
require daslib/defer
require math

require daslib/safe_addr
require daslib/random

require path_tracer

let TRACE_WIDTH = 1024
let TRACE_HEIGHT = 1024

var [[in, location=0]] v_position : float2
var [[in, location=1]] v_texcoord : float2
var [[inout]] f_texcoord : float2
var [[uniform]] f_tex : sampler2D
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
    f_FragColor = linear_to_RGB(texture2D(f_tex,f_texcoord).xyz).zyxx

[compute_program(local_size_x=16,local_size_y=16)]
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
        imageStore(c_destTex,storePos, srgb)

var compute_program, display_program : uint
var vao : uint
var vbo : uint
var ebo : uint
var texture : uint

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)

[export]
def main
    if glfwInit()==0
		panic("can't init glfw")
    defer <|
        glfwTerminate()
    glfwInitOpenGL(4,3)
    var window = glfwCreateWindow(TRACE_WIDTH, TRACE_HEIGHT, "OpenGL Path tracer", null, null)
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
        // display it
        glViewport(0, 0, display_w, display_h)
        glUseProgram(display_program)
        f_tex := texture
        vs_effect_bind_uniform(display_program)
        fs_effect_bind_uniform(display_program)
        glBindVertexArray(vao)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
