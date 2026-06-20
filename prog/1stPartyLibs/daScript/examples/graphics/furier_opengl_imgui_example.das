options gen2
options indenting = 4
options persistent_heap = true
options _allow_glfw_calls = true

require imgui/imgui_harness
require opengl/opengl_boost
require glfw/glfw_boost
require live/glfw_live
require imgui_app
require daslib/safe_addr
require daslib/math_boost
require daslib/strings

// Custom 3D + ImGui hybrid: the harness gives us the boost-v2 widget surface
// (window/edit_*/text) and the daslang theme via live_imgui_init, but we
// split harness_end_frame manually because our Fourier viz draws between
// the GL clear and ImGui's RenderDrawData. _allow_glfw_calls = true opts
// out of imgui_harness_lint for the OpenGL/GLFW calls we own.

let NGRAPH = 1000

var {
    rotating : bool = true
    rps : float = 0.1f
    tt : float = 0.0f
    c0 : float2 = float2(-0.2f, 0.05f)
    enable_1 : bool = true
    cp1 : float2 = float2(0.27f, 0.0f)
    cn1 : float2 = float2(0.0f, 0.10f)
    enable_2 : bool = false
    cp2 : float2 = float2(-0.07f, 0.08f)
    cn2 : float2 = float2(0.03f, -0.02f)
}

def angle(c : float2) {
    let C = normalize(c)
    return atan2(C.y, C.x)
}

def format_angle(c : float2) : string {
    return fmt(":.2f", angle(c) / PI) + "*PI"
}

def format_length(c : float2) : string {
    return fmt(":.2f", length(c))
}

def edit_coefficients_window() {
    window(SETUP_WIN, (text = "Setup vectors and circles", closable = false,
                       flags = ImGuiWindowFlags.None)) {
        text("Speed of PHI, rotations per second")
        edit_checkbox(safe_addr(rotating), (id = "ROTATING", text = "Rotating"))
        edit_input_float(safe_addr(tt), (id = "TIME", text = "time", step = 0.1f))
        edit_input_float(safe_addr(rps), (id = "RPS", text = "RPS", step = 0.1f))
        separator(SEP_C0)
        text("C0 A={format_length(c0)} PHI={format_angle(c0)}")
        edit_input_float2(safe_addr(c0), (id = "C0", text = "C0 (x,y)"))
        separator(SEP_C1)
        edit_checkbox(safe_addr(enable_1), (id = "ENABLE_1", text = "Enable C1,C-1"))
        if (enable_1) {
            text("C1  A={format_length(cp1)} PHI={format_angle(cp1)}")
            edit_input_float2(safe_addr(cp1), (id = "CP1", text = "C1 (x,y)"))
            text("C-1 A={format_length(cn1)} PHI={format_angle(cn1)}")
            edit_input_float2(safe_addr(cn1), (id = "CN1", text = "C-1 (x,y)"))
            separator(SEP_C2)
            edit_checkbox(safe_addr(enable_2), (id = "ENABLE_2", text = "Enable C2,C-2"))
            if (enable_2) {
                text("C2  A={format_length(cp2)} PHI={format_angle(cp2)}")
                edit_input_float2(safe_addr(cp2), (id = "CP2", text = "C2 (x,y)"))
                text("C-2 A={format_length(cn2)} PHI={format_angle(cn2)}")
                edit_input_float2(safe_addr(cn2), (id = "CN2", text = "C-2 (x,y)"))
            }
        }
    }
}

def draw_fourier() {
    let time = rotating ? glfwGetTime() * double(rps) % 1.0lf : double(tt)
    let t = float(time)
    let p0 = c0
    let pp1 = mul_complex(cp1, rot_complex(1.0 * t * 2.0 * PI))
    let pn1 = mul_complex(cn1, rot_complex(-1.0 * t * 2.0 * PI))
    let pp2 = mul_complex(cp2, rot_complex(2.0 * t * 2.0 * PI))
    let pn2 = mul_complex(cn2, rot_complex(-2.0 * t * 2.0 * PI))
    draw_arrow(float2(0.0f, 0.0f), p0, float3(1.0f, 0.0f, 0.0f))
    if (enable_1) {
        var p = p0
        draw_circle(p, length(cp1), float3(0.357f, 0.333f, 0.278f))
        draw_arrow(p, pp1, float3(1.0f, 0.0f, 0.0f))
        p += pp1
        draw_circle(p, length(cn1), float3(0.357f, 0.333f, 0.278f))
        draw_arrow(p, pn1, float3(1.0f, 0.0f, 0.0f))
        p += pn1
        if (enable_2) {
            draw_circle(p, length(cp2), float3(0.357f, 0.333f, 0.278f))
            draw_arrow(p, pp2, float3(1.0f, 0.0f, 0.0f))
            p += pp2
            draw_circle(p, length(cn2), float3(0.357f, 0.333f, 0.278f))
            draw_arrow(p, pn2, float3(1.0f, 0.0f, 0.0f))
            p += pn2
        }
    }
    draw_graph(float3(1.0f, 1.0f, 0.0f))
}

[export]
def init() {
    harness_init("Vectors & Circles", 1024, 1024)
    create_gl_objects()
}

[export]
def update() {
    if (!harness_begin_frame()) return
    harness_new_frame()

    edit_coefficients_window()

    var display_w, display_h : int
    live_get_framebuffer_size(display_w, display_h)
    glViewport(0, 0, display_w, display_h)
    // Match the daslang theme's bg (#0d0c0a) so the GL viewport blends
    // seamlessly with the ImGui windows that sit on top.
    glClearColor(0.051f, 0.047f, 0.039f, 1.0f)
    glClear(GL_COLOR_BUFFER_BIT)
    draw_fourier()

    end_of_frame()
    Render()
    ImGui_ImplOpenGL3_RenderDrawData(GetDrawData())
    live_end_frame()
}

[export]
def shutdown() {
    harness_shutdown()
}

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}

var @in @location = 0 v_position : float2
var @in @location = 1 v_color : float3
var @out f_FragColor : float4
var @uniform f_color : float3
var @uniform v_offset : float2
var @uniform v_scale : float2
var @uniform v_rot : float2

[vertex_program]
def vs_main {
    let pos = mul_complex(v_position, v_rot)
    gl_Position = float4(pos * v_scale + v_offset, 0.0, 1.0)
}

[fragment_program]
def fs_main {
    f_FragColor = float4(f_color, 1.0)
}

var program : uint
var vao_circle : uint
var vbo_circle : uint
var vao_arrow : uint
var vbo_arrow : uint
var vao_graph : uint
var vbo_graph : uint

[vertex_buffer]
struct Vertex {
    xy : float2
}

def draw_circle(origin : float2; radius : float; color : float3) {
    glUseProgram(program)
    glEnable(GL_LINE_SMOOTH)
    f_color = color
    v_offset = origin
    v_scale = float2(radius, radius)
    v_rot = float2(1.0f, 0.0f)
    vs_main_bind_uniform(program)
    fs_main_bind_uniform(program)
    glBindVertexArray(vao_circle)
    glDrawArrays(GL_LINE_LOOP, 0, 360)
    glDisable(GL_LINE_SMOOTH)
}

def draw_arrow(origin : float2; c : float2; color : float3) {
    glUseProgram(program)
    glEnable(GL_LINE_SMOOTH)
    f_color = color
    v_offset = origin
    v_rot = c
    v_scale = float2(1.0f, 1.0f)
    vs_main_bind_uniform(program)
    fs_main_bind_uniform(program)
    glBindVertexArray(vao_arrow)
    glDrawArrays(GL_LINE_LOOP, 0, 360)
    glDisable(GL_LINE_SMOOTH)
}

def draw_graph(color : float3) {
    glUseProgram(program)
    glEnable(GL_LINE_SMOOTH)
    f_color = color
    v_offset = float2(0.0f, 0.0f)
    v_rot = float2(1.0f, 0.0f)
    v_scale = float2(1.0f, 1.0f)
    vs_main_bind_uniform(program)
    fs_main_bind_uniform(program)
    glBindVertexArray(vao_graph)
    glBindBuffer(GL_ARRAY_BUFFER, vbo_graph)
    compute_graph()
    glDrawArrays(GL_LINE_LOOP, 0, NGRAPH)
    glDisable(GL_LINE_SMOOTH)
}

def create_gl_objects {
    program = create_shader_program(@@vs_main, @@fs_main)
    // circle
    {
        glGenVertexArrays(1, safe_addr(vao_circle))
        glBindVertexArray(vao_circle)
        glGenBuffers(1, safe_addr(vbo_circle))
        glBindBuffer(GL_ARRAY_BUFFER, vbo_circle)
        var vertices : array<Vertex>
        for (t in range(360)) {
            let T = float(t) * 2.0 * PI / 360.0
            vertices |> push(Vertex(xy = float2(cos(T), sin(T))))
        }
        glBufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW)
        bind_vertex_buffer(null, type<Vertex>)
    }
    // arrow
    {
        glGenVertexArrays(1, safe_addr(vao_arrow))
        glBindVertexArray(vao_arrow)
        glGenBuffers(1, safe_addr(vbo_arrow))
        glBindBuffer(GL_ARRAY_BUFFER, vbo_arrow)
        var vertices <- [
            Vertex(xy = float2(0.0f, 0.0f)),
            Vertex(xy = float2(1.0f, 0.0f)),
            Vertex(xy = float2(0.95f, 0.025f)),
            Vertex(xy = float2(0.95f, -0.025f)),
            Vertex(xy = float2(1.0f, 0.0f))
        ]
        glBufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW)
        bind_vertex_buffer(null, type<Vertex>)
        // graph
        glGenVertexArrays(1, safe_addr(vao_graph))
        glBindVertexArray(vao_graph)
        glGenBuffers(1, safe_addr(vbo_graph))
        glBindBuffer(GL_ARRAY_BUFFER, vbo_graph)
        compute_graph()
        bind_vertex_buffer(null, type<Vertex>)
    }
}

def mul_complex(a, b : float2) {
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)
}

def rot_complex(phi : float) {
    return float2(cos(phi), sin(phi))
}

def compute_fn(t : float) {
    let p0 = c0
    let pp1 = mul_complex(cp1, rot_complex(1.0 * t * 2.0 * PI))
    let pn1 = mul_complex(cn1, rot_complex(-1.0 * t * 2.0 * PI))
    let pp2 = mul_complex(cp2, rot_complex(2.0 * t * 2.0 * PI))
    let pn2 = mul_complex(cn2, rot_complex(-2.0 * t * 2.0 * PI))
    var p = p0
    if (enable_1) {
        p += pp1 + pn1
        if (enable_2) {
            p += pp2 + pn2
        }
    }
    return p
}

def compute_graph {
    var vertices : array<Vertex>
    for (t in range(NGRAPH)) {
        let T = float(t) / float(NGRAPH)
        let p = compute_fn(T)
        vertices |> push(Vertex(xy = p))
    }
    glBufferData(GL_ARRAY_BUFFER, vertices, GL_DYNAMIC_DRAW)
}
