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
var [[out]] f_FragColor : float4
var [[uniform]] c_frame : int = 0

[vertex_program]
def vs_effect
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)

def linear_to_RGB ( color : float3 )
    let x = float4(max(color, float3(0)), 1.0)
    return max(1.055 * pow(x, float4(0.416666667)) - float4(0.055), float4(0))

[fragment_program]
def fs_effect
    init_path_tracer()
    let storePos = int2(f_texcoord.xy * float2(1024.))
    random_seed2D(RAND_SEED,storePos,c_frame)
    random_float4(RAND_SEED)
    let rgb = draw_ray(storePos.x,TRACE_HEIGHT-storePos.y,TRACE_WIDTH,TRACE_HEIGHT)
    f_FragColor = linear_to_RGB(rgb).xyzz

var display_program : uint
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

[export]
def main
    if glfwInit()==0
		panic("can't init glfw")
    defer <|
        glfwTerminate()
    glfwInitOpenGL(3,3)
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
        glViewport(0, 0, display_w, display_h)
        glUseProgram(display_program)
        c_frame++
        vs_effect_bind_uniform(display_program)
        fs_effect_bind_uniform(display_program)
        glBindVertexArray(vao)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
