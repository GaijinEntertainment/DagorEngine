options gen2
options persistent_heap
options gc
require glfw/glfw_boost
require opengl/opengl_boost
require glsl/glsl_opengl
require strings

require daslib/safe_addr
require live/live_gc

var @in @location v_position : float2
var @in @location v_color : float3
var @in @location v_texcoord : float2
var @inout f_color : float3
var @inout f_texcoord : float2
var @uniform f_tex : sampler2D
var @out f_FragColor : float4

[vertex_program]
def vs_main {
    f_color = v_color
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)
}

[fragment_program]
def fs_main {
    f_FragColor = texture(f_tex, f_texcoord) * float4(f_color, 1.0)
}

var window : GLFWwindow?
var program : uint
var vao : uint
var vbo : uint
var ebo : uint
var texture : uint

[vertex_buffer]
struct Vertex {
    xy  : float2
    rgb : float3
    uv  : float2
}

let vertices = [Vertex(
    xy=float2(-0.5, 0.5), rgb=float3(0.0, 0.0, 0.0), uv=float2(0.0, 0.0)), Vertex(
    xy=float2(0.5, 0.5), rgb=float3(1.0, 1.0, 0.0), uv=float2(1.0, 0.0)), Vertex(
    xy=float2(0.5, -0.5), rgb=float3(1.0, 0.0, 0.0), uv=float2(1.0, 1.0)), Vertex(
    xy=float2(-0.5, -0.5), rgb=float3(1.0, 1.0, 0.0), uv=float2(0.0, 1.0)
)];

let indices = fixed_array<int>(0, 1, 2, 2, 3, 0)

def create_gl_objects {
    program = create_shader_program(@@vs_main, @@fs_main)
    // vao
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao)
    // vbo
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<Vertex>)
    // ebo
    glGenBuffers(1, safe_addr(ebo))
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW)
    // texture
    texture = load_image_from_file("image.png",
        "{get_das_root()}/examples/opengl",
        "{get_das_root()}/examples/opengl/media"
    )
}

[export]
def init() {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3)
    window = glfwCreateWindow(1280, 720, "OpenGL - Hello image", null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwMakeContextCurrent(window)
    create_gl_objects()
}

[export]
def update() {
    glfwPollEvents()
    var display_w, display_h : int
    glfwGetFramebufferSize(window, display_w, display_h)
    // render
    glViewport(0, 0, display_w, display_h)
    glClearColor(0.2, 0.2, 0.2, 1.0)
    glClearDepth(1.0lf)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)
    glUseProgram(program)
    f_tex := texture           // bind texture
    vs_main_bind_uniform(program)
    fs_main_bind_uniform(program)
    glBindVertexArray(vao)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
    // swap buffers
    glfwMakeContextCurrent(window)
    glfwSwapBuffers(window)
}

[export]
def shutdown() {
    if (window != null) {
        glfwDestroyWindow(window)
        window = null
    }
    glfwTerminate()
}

var max_frame_limit = 0

def parse_args() {
    let args <- get_command_line_arguments()
    for (i in range(length(args) - 1)) {
        if (args[i] == "--max-frames") {
            max_frame_limit = to_int(args[i + 1])
        }
    }
}

// on web the wasm host auto-drives init/update/shutdown per frame and never calls main
[export]
def main() {
    parse_args()
    init()
    var frame_count = 0
    while (glfwWindowShouldClose(window) == 0) {
        update()
        maybe_collect_gc()
        frame_count ++
        if (max_frame_limit > 0 && frame_count >= max_frame_limit) {
            break
        }
    }
    shutdown()
}
