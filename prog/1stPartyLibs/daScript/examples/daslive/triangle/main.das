options gen2

require live/glfw_live
require opengl/opengl_boost
require daslib/decs_boost
require live/decs_live
require live/opengl_live
require live/live_commands
require live/live_api
require daslib/json
require daslib/json_boost
require daslib/safe_addr
require daslib/math_boost
require live_host

// --- Shaders ---

var @in @location v_position : float2
var @in @location v_color : float3
var @inout f_color : float3
var @out f_FragColor : float4

[vertex_program]
def vs_triangle {
    f_color = v_color
    gl_Position = float4(v_position, 0.0, 1.0)
}

[fragment_program]
def fs_triangle {
    f_FragColor = float4(f_color, 1.0)
}

// --- GL objects (re-created every init) ---

var program : uint
var vao : uint
var vbo : uint

[vertex_buffer]
struct Vertex {
    xy : float2
    rgb : float3
}

def create_gl_objects() {
    program = create_shader_program(@@vs_triangle, @@fs_triangle)
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao)
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    // Allocate buffer (will be updated each frame)
    var dummy : Vertex[3]
    glBufferData(GL_ARRAY_BUFFER, dummy, GL_DYNAMIC_DRAW)
    bind_vertex_buffer(null, type<Vertex>)
}

def delete_gl_objects() {
    if (vao != 0u) {
        glDeleteVertexArrays(1, safe_addr(vao))
        vao = 0u
    }
    if (vbo != 0u) {
        glDeleteBuffers(1, safe_addr(vbo))
        vbo = 0u
    }
    // shader program is cached, no need to delete
}

// --- DECS: triangle state ---

def make_triangle() {
    create_entity <| @(eid, cmp) {
        cmp |> set("eid", eid)
        cmp |> set("angle", 0.0f)
        cmp |> set("speed", 1.5f)
        cmp |> set("radius", 0.6f)
    }
    commit()
}

// --- Update ---

def update_triangle(dt : float) {
    query() <| $(var angle : float&; speed : float) {
        angle += speed * dt
    }
}

def get_triangle_vertices(var vertices : Vertex[3]&) {
    query() <| $(angle : float; radius : float) {
        // Aspect ratio correction
        var w, h : int
        live_get_framebuffer_size(w, h)
        let aspect = h > 0 ? float(w) / float(h) : 1.0f
        let scale = float2(1.0f / aspect, 1.0f)
        // Arrow shape: tip (bright red) + two base vertices (dark blue)
        let tip_offset = float2(cos(angle), sin(angle)) * radius
        let perp = float2(-sin(angle), cos(angle)) * radius * 0.25f
        let base = float2(cos(angle), sin(angle)) * radius * (-0.5f)
        vertices[0] = Vertex(xy = tip_offset * scale, rgb = float3(1.0, 0.2, 0.0))
        vertices[1] = Vertex(xy = (base + perp) * scale, rgb = float3(0.1, 0.1, 0.4))
        vertices[2] = Vertex(xy = (base - perp) * scale, rgb = float3(0.1, 0.1, 0.4))
    }
}

// --- Live commands ---

[live_command]
def set_rotation(input : JsonValue?) : JsonValue? {
    var enabled = true
    if (input != null && input.value is _object) {
        let tab & = unsafe(input.value as _object)
        var ev = tab?["enabled"] ?? null
        if (ev != null && ev.value is _bool) {
            enabled = ev.value as _bool
        }
    }
    query() <| $(var speed : float&) {
        speed = enabled ? 1.5f : 0.0f
    }
    return JV("\{\"rotation\": {enabled}}")
}

[live_command]
def set_angle(input : JsonValue?) : JsonValue? {
    var angle_val = 0.0f
    if (input != null && input.value is _object) {
        let tab & = unsafe(input.value as _object)
        var av = tab?["angle"] ?? null
        if (av != null) {
            if (av.value is _number) {
                angle_val = float(av.value as _number)
            } elif (av.value is _longint) {
                angle_val = float(av.value as _longint)
            }
        }
    }
    query() <| $(var angle : float&) {
        angle = angle_val
    }
    return JV("\{\"angle\": {angle_val}}")
}

// --- Lifecycle ---

[export]
def init() {
    live_create_window("Rotating Triangle", 800, 600)
    create_gl_objects()
    // Only create entity on first run (DECS state survives reload)
    if (!is_reload()) {
        decs::restart()
        make_triangle()
    }
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    update_triangle(get_dt())
    // Render
    var w, h : int
    live_get_framebuffer_size(w, h)
    glViewport(0, 0, w, h)
    glClearColor(0.15, 0.15, 0.2, 1.0)
    glClear(GL_COLOR_BUFFER_BIT)
    glUseProgram(program)
    vs_triangle_bind_uniform(program)
    fs_triangle_bind_uniform(program)
    // Upload triangle vertices
    var vertices : Vertex[3]
    get_triangle_vertices(vertices)
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    unsafe {
        glBufferSubData(GL_ARRAY_BUFFER, 0l, int64(typeinfo sizeof(vertices)), addr(vertices[0]))
    }
    glBindVertexArray(vao)
    glDrawArrays(GL_TRIANGLES, 0, 3)
    live_end_frame()
}

[export]
def shutdown() {
    delete_gl_objects()
    live_destroy_window()
}

// Dual-mode: also works with regular daslang.exe
[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
