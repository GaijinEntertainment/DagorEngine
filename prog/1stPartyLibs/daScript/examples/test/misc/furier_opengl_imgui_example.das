options gen2
options persistent_heap = true

require imgui_app
require glfw/glfw_boost
require imgui/imgui_boost
require opengl/opengl_boost
require daslib/math_boost
require daslib/safe_addr
require daslib/strings

var window : GLFWwindow?

let NGRAPH = 1000

var {
    rotating : bool = true
    rps : float = 0.1f
    tt : float = 0.0f
// c0
    c0 : float2 = float2(-0.2f, 0.05f)
// cp1, cn1
    enable_1 : bool = true
    cp1 : float2 = float2(0.27f, 0.0f)
    cn1 : float2 = float2(0.0f, 0.10f)
// cp2, cn2
    enable_2 : bool = false
    cp2 : float2 = float2(-0.07f, 0.08f)
    cn2 : float2 = float2(0.03f, -0.02f)
}

def imgui_app(title : string; blk : block) {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3, false, false)
    window = glfwCreateWindow(1024, 1024, title, null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwMakeContextCurrent(window)
    glfwSwapInterval(1)
    CreateContext(null)
    var io & = unsafe(GetIO())
    io.FontGlobalScale = 1.0   // BBATKIN: note - my monitor is HUGE
    StyleColorsDark(null)
    ImGui_ImplGlfw_InitForOpenGL(window, true)
    ImGui_ImplOpenGL3_Init("#version 330")
    var clear_color = float4(0.85f, 0.85f, 0.90f, 1.00f)
    create_gl_objects()
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents()
        ImGui_ImplOpenGL3_NewFrame()
        ImGui_ImplGlfw_NewFrame()
        invoke(blk)
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        glViewport(0, 0, display_w, display_h)
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w)
        glClear(GL_COLOR_BUFFER_BIT)
        let time = rotating ? glfwGetTime() * double(rps) % 1.0lf : double(tt)
        let t = float(time)
        // compute vectors
        let p0 = c0
        let pp1 = mul_complex(cp1, rot_complex(1.0 * t * 2.0 * PI))
        let pn1 = mul_complex(cn1, rot_complex(-1.0 * t * 2.0 * PI))
        let pp2 = mul_complex(cp2, rot_complex(2.0 * t * 2.0 * PI))
        let pn2 = mul_complex(cn2, rot_complex(-2.0 * t * 2.0 * PI))
        // c0
        draw_arrow(float2(0.0f, 0.0f), p0, float3(1.0f, 0.0f, 0.0f))
        if (enable_1) {
            // cp1
            var p = p0
            draw_circle(p, length(cp1), float3(0.0f, 0.0f, 0.0f))
            draw_arrow(p, pp1, float3(1.0f, 0.0f, 0.0f))
            p += pp1
            // cn1
            draw_circle(p, length(cn1), float3(0.0f, 0.0f, 0.0f))
            draw_arrow(p, pn1, float3(1.0f, 0.0f, 0.0f))
            p += pn1
            if (enable_2) {
                // cp2
                draw_circle(p, length(cp2), float3(0.0f, 0.0f, 0.0f))
                draw_arrow(p, pp2, float3(1.0f, 0.0f, 0.0f))
                p += pp2
                // cn2
                draw_circle(p, length(cn2), float3(0.0f, 0.0f, 0.0f))
                draw_arrow(p, pn2, float3(1.0f, 0.0f, 0.0f))
                p += pn2
            }
        }
        // graph
        draw_graph(float3(1.0f, 1.0f, 0.0f))
        // and done
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData())
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
    }
    ImGui_ImplOpenGL3_Shutdown()
    ImGui_ImplGlfw_Shutdown()
    DestroyContext(null)
    glfwDestroyWindow(window)
    glfwTerminate()
}

def angle(c : float2) {
    let C = normalize(c)
    return atan2(C.y, C.x)
}

def format_angle(c : float2) {
    return fmt(":.2f", angle(c) / PI) + "*PI"
}

def format_length(c : float2) {
    return fmt(":.2f", length(c))
}

def editCoefficientsWindow(p_open : bool? implicit) {
    if (!Begin("Setup vectors and circles", p_open, ImGuiWindowFlags.None)) {
        End()
        return
    }
    Text("Speed of PHI, rotations per second")
    Checkbox("Rotating", unsafe(addr(rotating)))
    InputFloat("time", unsafe(addr(tt)), 0.1f)
    InputFloat("RPS", unsafe(addr(rps)), 0.1f)
    Separator()
    Text("C0 A={format_length(c0)} PHI={format_angle(c0)}")
    InputFloat("C0.x", unsafe(addr(c0.x)), 0.1f)
    InputFloat("C0.y", unsafe(addr(c0.y)), 0.1f)
    Separator()
    Checkbox("Enable C1,C-1", unsafe(addr(enable_1)))
    if (enable_1) {
        Text("C1  A={format_length(cp1)} PHI={format_angle(cp1)}")
        InputFloat("C1.x", unsafe(addr(cp1.x)), 0.1f)
        InputFloat("C1.y", unsafe(addr(cp1.y)), 0.1f)
        Text("C-1 A={format_length(cn1)} PHI={format_angle(cn1)}")
        InputFloat("C-1.x", unsafe(addr(cn1.x)), 0.1f)
        InputFloat("C-1.y", unsafe(addr(cn1.y)), 0.1f)
        Separator()
        Checkbox("Enable C2,C-2", unsafe(addr(enable_2)))
        if (enable_2) {
            Text("C2  A={format_length(cp2)} PHI={format_angle(cp2)}")
            InputFloat("C2.x", unsafe(addr(cp2.x)), 0.1f)
            InputFloat("C2.y", unsafe(addr(cp2.y)), 0.1f)
            Text("C-2 A={format_length(cn2)} PHI={format_angle(cn2)}")
            InputFloat("C-2.x", unsafe(addr(cn2.x)), 0.1f)
            InputFloat("C-2.y", unsafe(addr(cn2.y)), 0.1f)
        }
    }
    End()
}

[export]
def main {
    var f = 0.0
    imgui_app("Vectors & Circles") <| $() {
        NewFrame()
        editCoefficientsWindow(null)
        Render()
    }
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
    v_scale = float2(1., 1.)
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
    if (true) {
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
    if (true) {
        glGenVertexArrays(1, safe_addr(vao_arrow))
        glBindVertexArray(vao_arrow)
        glGenBuffers(1, safe_addr(vbo_arrow))
        glBindBuffer(GL_ARRAY_BUFFER, vbo_arrow)
        var vertices <- [Vertex(
            xy = float2(0.0f, 0.0f)), Vertex(
            xy = float2(1.0f, 0.0f)), Vertex(
            xy = float2(0.95f, 0.025f)), Vertex(
            xy = float2(0.95f, -0.025f)), Vertex(
            xy = float2(1.0f, 0.0f)
        )]
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
    // (a.x+i*a.y)*(b.x+i*b.y) = a.x*b.x - a.y*b.y + i*(a.x*b.y + a.y*b.x)
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)
}

def rot_complex(phi : float) {
    return float2(cos(phi), sin(phi))
}

def compute_fn(t : float) {// 0..1
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