options gen2
require glfw/glfw_boost
require opengl/opengl_boost
require daslib/defer
require math

require daslib/safe_addr

var @in @location v_position : float2
var @in @location v_color : float3
var @inout f_color : float3
var @out f_FragColor : float4

[vertex_program]
def vs_main {
    f_color = v_color
    gl_Position = float4(v_position, 0.0, 1.0)
}

[fragment_program]
def fs_main {
    f_FragColor = float4(f_color, 1.0)
}

var program : uint
var vao : uint
var vbo : uint

[vertex_buffer]
struct Vertex {
    xy : float2
    rgb : float3
}

let vertices = [Vertex(
    xy=float2(-0.5, 0.5), rgb=float3(0.0, 0.0, 0.0)), Vertex(
    xy=float2(0.5, 0.5), rgb=float3(1.0, 1.0, 0.0)), Vertex(
    xy=float2(0.5, -0.5), rgb=float3(1.0, 0.0, 0.0)
)];

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
}

[export]
def main {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    defer <| $() {
        glfwTerminate()
    }
    glfwInitOpenGL(3, 3)
    var window = glfwCreateWindow(1280, 720, "OpenGL - Hello triangle", null, null)
    if (window == null) {
        panic("can't create window")
    }
    defer <| $() {
        glfwDestroyWindow(window)
    }
    glfwMakeContextCurrent(window)
    create_gl_objects()
    eval_main_loop <| $() {
        if (glfwWindowShouldClose(window) != 0) {
            return false
        }
        glfwPollEvents()
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        // render
        glViewport(0, 0, display_w, display_h)
        glClearColor(0.2, 0.2, 0.2, 1.0)
        glClearDepth(1.0lf)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glDepthFunc(GL_LEQUAL)
        glUseProgram(program)
        vs_main_bind_uniform(program)
        fs_main_bind_uniform(program)
        glBindVertexArray(vao)
        glDrawArrays(GL_TRIANGLES, 0, 3)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
        return true
    }
}
