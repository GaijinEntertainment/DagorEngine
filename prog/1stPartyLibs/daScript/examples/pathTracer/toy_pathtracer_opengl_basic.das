options gen2
options persistent_heap
options gc
require glfw/glfw_boost
require opengl/opengl_boost
require glsl/glsl_opengl
require math

require daslib/safe_addr
require daslib/random

require path_tracer
require daslib/clargs
require live/live_gc

let TRACE_WIDTH = 1024
let TRACE_HEIGHT = 1024

[CommandLineArgs]
struct PathTracerArgs {
    @clarg_short = "f"
    @clarg_name = "max-frames"
    @clarg_doc = "Render this many frames, then exit 0 (0 = run until the window closes). Lets the demo double as a headless smoke test."
    frames : int

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

var @in @location = 0 v_position : float2
var @in @location = 1 v_texcoord : float2
var @inout f_texcoord : float2
var @out f_FragColor : float4
var @uniform c_frame : int = 0

[vertex_program]
def vs_effect {
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)
}

def linear_to_RGB(color : float3) {
    let x = float4(max(color, float3(0)), 1.0)
    return max(1.055 * pow(x, float4(0.416666667)) - float4(0.055), float4(0))
}

[fragment_program]
def fs_effect {
    init_path_tracer()
    let storePos = int2(f_texcoord.xy * float2(1024.))
    random_seed2D(RAND_SEED, storePos, c_frame)
    random_float4(RAND_SEED)
    let rgb = draw_ray(storePos.x, TRACE_HEIGHT - storePos.y, TRACE_WIDTH, TRACE_HEIGHT)
    f_FragColor = linear_to_RGB(rgb).xyzz
}

var display_program : uint
var vao : uint
var vbo : uint
var ebo : uint
var texture : uint
var window : GLFWwindow?

[vertex_buffer]
struct EffectVertex {
    xy : float2
    uv : float2
}

let effect_vertices = fixed_array(EffectVertex(
    xy = float2(-1.0, 1.0), uv = float2(0.0, 0.0)), EffectVertex(
    xy = float2(1.0, 1.0), uv = float2(1.0, 0.0)), EffectVertex(
    xy = float2(1.0, -1.0), uv = float2(1.0, 1.0)), EffectVertex(
    xy = float2(-1.0, -1.0), uv = float2(0.0, 1.0)
))

let effect_indices = fixed_array<int>(0, 1, 2, 2, 3, 0)

def create_gl_objects {
    display_program = create_shader_program(@@vs_effect, @@fs_effect)
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao)
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, effect_vertices, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<EffectVertex>)
    glGenBuffers(1, safe_addr(ebo))
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, effect_indices, GL_STATIC_DRAW)
}

[export]
def init {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3)
    window = glfwCreateWindow(TRACE_WIDTH, TRACE_HEIGHT, "OpenGL Path tracer", null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwSetWindowSize(window, TRACE_WIDTH, TRACE_HEIGHT)
    glfwMakeContextCurrent(window)
    create_gl_objects()
}

[export]
def update {
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
}

[export]
def shutdown {
    if (window != null) {
        glfwDestroyWindow(window)
        window = null
    }
    glfwTerminate()
}

[export]
def main {
    var r <- parse_args(type<PathTracerArgs>)
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<PathTracerArgs>), "toy_pathtracer_opengl_basic")
        return
    }
    let cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<PathTracerArgs>), "toy_pathtracer_opengl_basic")
        return
    }
    init()
    var frame_count = 0
    while (glfwWindowShouldClose(window) == 0) {
        update()
        maybe_collect_gc()
        frame_count++
        if (cfg.frames > 0 && frame_count >= cfg.frames) {
            break   // headless smoke / test mode: stop after the requested frame count
        }
    }
    shutdown()
}
