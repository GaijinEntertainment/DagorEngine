options gen2
options persistent_heap
options no_global_variables = false
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require glfw/glfw_boost
require opengl/opengl_boost
require opengl/opengl_gen
require audio/audio_boost
require daslib/defer
require daslib/fio
require daslib/safe_addr
require daslib/math_boost
require daslib/jobque_boost
require math
require strings

// -- Shaders --

var @in @location v_position : float3
var @in @location v_normal : float3
var @in @location v_texcoord : float2
var @uniform v_model : float4x4
var @uniform v_view : float4x4
var @uniform v_projection : float4x4
var @inout f_normal : float3
var @inout f_world_pos : float3
var @uniform f_color : float3
var @out f_FragColor : float4

[vertex_program]
def vs_solid {
    f_normal = normalize(float3x3(v_model) * v_normal)
    let tpos = v_model * float4(v_position, 1.0)
    f_world_pos = tpos.xyz
    gl_Position = v_projection * v_view * tpos
}

[fragment_program]
def fs_solid {
    let normal = normalize(f_normal)
    let light_dir = normalize(float3(-1.0, -2.0, -3.0))
    let ambient = 0.15
    let diffuse = saturate(-dot(light_dir, normal))
    let lit = ambient + diffuse * 0.85
    f_FragColor = float4(f_color * lit, 1.0)
}

// -- Line shader for grid --

var @in @location v_line_pos : float3
var @uniform v_line_mvp : float4x4
var @uniform f_line_color : float4
var @out f_line_FragColor : float4

[vertex_program]
def vs_line {
    gl_Position = v_line_mvp * float4(v_line_pos, 1.0)
}

[fragment_program]
def fs_line {
    f_line_FragColor = f_line_color
}

// -- Camera --

var camera_pos = float3(0.0, 2.0, 5.0)
var camera_yaw = -PI * 0.5  // face -Z
var camera_pitch = 0.0
var cursor_captured = false
var last_mouse_x = 0.0
var last_mouse_y = 0.0

def camera_forward() : float3 {
    return normalize(float3(cos(camera_yaw) * cos(camera_pitch), sin(camera_pitch), sin(camera_yaw) * cos(camera_pitch)))
}

def camera_right() : float3 {
    return normalize(cross(camera_forward(), float3(0.0, 1.0, 0.0)))
}

// -- Sound sources --

struct SoundSource {
    position : float3
    sid : uint64
    color : float3
}

var sources : array<SoundSource>

let SOURCE_COLORS = fixed_array(
    float3(1.0, 0.3, 0.3),   // red
    float3(0.3, 1.0, 0.3),   // green
    float3(0.3, 0.3, 1.0),   // blue
    float3(1.0, 1.0, 0.3),   // yellow
    float3(1.0, 0.3, 1.0)    // magenta
)

// HRTF/simulated routing budget — cycle on B key.
let HRTF_BUDGETS = fixed_array(32, 0, 999)
let HRTF_BUDGET_LABELS = fixed_array("mixed top-32", "all simulated", "all HRTF")
var hrtf_budget_idx = 0

var sound_data : array<float>
var sound_channels = 1
var sound_rate = MA_SAMPLE_RATE

// -- Grid geometry (GL_LINES) --

var grid_vao : uint
var grid_vbo : uint
var grid_vertex_count : int

def create_grid(half_extent : int; spacing : float) {
    var verts : array<float3>
    let fext = float(half_extent) * spacing
    verts |> reserve((2 * half_extent + 1) * 4)
    for (i in range(-half_extent, half_extent + 1)) {
        let p = float(i) * spacing
        verts |> push(float3(p, 0.0, -fext))
        verts |> push(float3(p, 0.0, fext))
        verts |> push(float3(-fext, 0.0, p))
        verts |> push(float3(fext, 0.0, p))
    }
    grid_vertex_count = length(verts)
    glGenVertexArrays(1, safe_addr(grid_vao))
    glBindVertexArray(grid_vao)
    glGenBuffers(1, safe_addr(grid_vbo))
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo)
    glBufferData(GL_ARRAY_BUFFER, verts, GL_STATIC_DRAW)
    glEnableVertexAttribArray(0u)
    glVertexAttribPointer(0u, 3, GL_FLOAT, false, 12, 0)
    glBindVertexArray(0u)
}

def draw_grid(line_program : uint; view, proj : float4x4) {
    glUseProgram(line_program)
    v_line_mvp = proj * view
    f_line_color = float4(0.4, 0.4, 0.4, 1.0)
    vs_line_bind_uniform(line_program)
    fs_line_bind_uniform(line_program)
    glBindVertexArray(grid_vao)
    glDrawArrays(GL_LINES, 0, grid_vertex_count)
    glBindVertexArray(0u)
}

// -- Audio file loading --

def load_sound_data() {
    let fname = "{get_das_root()}/examples/media/audio/music_cc0.mp3"
    var f = fopen(fname, "rb")
    if (f == null) {
        // Fallback: generate a 440Hz sine tone
        sound_data <- [for (x in range(MA_SAMPLE_RATE * 2)); sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))]
        sound_channels = 1
        sound_rate = MA_SAMPLE_RATE
        return
    }
    fmap(f) $(data) {
        sound_data <- decode_audio(data, sound_channels, sound_rate)
    }
    fclose(f)
}

// Audio uses XY horizontal + Z elevation; OpenGL uses XZ horizontal + Y up
// Audio uses XY horizontal + Z elevation; OpenGL uses XZ horizontal + Y up
// Audio: -Y = forward, +X = right.  OpenGL: -Z = forward, +X = right.
def gl_to_audio(p : float3) : float3 {
    return float3(p.x, p.z, p.y)
}

def add_sound_source(pos : float3; volume : float = 1.0) {
    let idx = length(sources) % 5
    var src = SoundSource(position = pos, color = SOURCE_COLORS[idx])
    var samples <- clone(sound_data)
    src.sid = play_3d_sound_loop_from_pcm(gl_to_audio(pos), linear_attenuation(10.0), sound_rate, sound_channels, samples)
    src.sid |> set_volume(volume)
    sources |> push(src)
}

// -- Main --

// Parse --max-frames N from argv. Useful for headless repro and shutdown-leak debugging.
def parse_max_frames {
    var maxFrames = 0
    let args <- get_command_line_arguments()
    for (i in range(length(args))) {
        if (args[i] == "--max-frames" && i + 1 < length(args)) {
            maxFrames = to_int(args[i + 1])
        }
    }
    return maxFrames
}

[export]
def main {
    let maxFrames = parse_max_frames()
    // GLFW init
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    defer() {
        glfwTerminate()
    }
    glfwInitOpenGL(3, 3)
    var window = glfwCreateWindow(1280, 720, "HRTF 3D Audio Demo", null, null)
    if (window == null) {
        panic("can't create window")
    }
    defer() {
        glfwDestroyWindow(window)
    }
    glfwMakeContextCurrent(window)

    // Shaders
    let solid_program = create_shader_program(@@vs_solid, @@fs_solid)
    let line_program = create_shader_program(@@vs_line, @@fs_line)

    // Geometry
    let sphere <- gen_sphere(16, 12, false) |> create_geometry_fragment
    create_grid(20, 1.0)

    // Mouse callbacks
    unsafe {
        glfwSetMouseButtonCallback(window) <| @(w, button, action, mods) {
            if (button == int(GLFW_MOUSE_BUTTON_1) && action == int(GLFW_PRESS)) {
                cursor_captured = !cursor_captured
                if (cursor_captured) {
                    glfwSetInputMode(window, int(GLFW_CURSOR), int(GLFW_CURSOR_DISABLED))
                } else {
                    glfwSetInputMode(window, int(GLFW_CURSOR), int(GLFW_CURSOR_NORMAL))
                }
                let cxy = glfwGetCursorPos(window)
                last_mouse_x = cxy.x
                last_mouse_y = cxy.y
            }
        }
        glfwSetCursorPosCallback(window) <| @(w, x, y) {
            if (cursor_captured) {
                let fx = float(x)
                let fy = float(y)
                let dx = fx - last_mouse_x
                let dy = fy - last_mouse_y
                last_mouse_x = fx
                last_mouse_y = fy
                let sensitivity = 0.003
                camera_yaw += dx * sensitivity
                camera_pitch -= dy * sensitivity
                camera_pitch = clamp(camera_pitch, -PI * 0.45, PI * 0.45)
            }
        }
    }

    // Load sound
    load_sound_data()

    // Stats LockBox for per-second utilization + HRTF/simulated split readout.
    // Lifecycle wraps the audio system: the audio thread releases its share-ref during
    // audio_system_finalize (inside with_audio_system below), THEN this outer defer
    // calls lock_box_remove to do the final delete. `release` alone never deletes.
    var stats_box <- lock_box_create()
    defer() {
        unsafe(lock_box_remove(stats_box))
    }

    // Audio system
    with_audio_system() {
        // Start with one source in front
        add_sound_source(float3(0.0, 0.0, -3.0))

        set_audio_stats_box(stats_box)

        print("HRTF 3D Audio Demo\n")
        print("  Click to capture mouse, click again to release\n")
        print("  WASD to move, mouse to look\n")
        print("  N to add a sound source, M to add 30, ESC to quit\n")
        print("  B to cycle HRTF budget (mixed top-32 / all simulated / all HRTF)\n")

        var last_time = glfwGetTime()
        var n_was_pressed = false
        var m_was_pressed = false
        var b_was_pressed = false
        var since_stats_print = 0.0
        var frameCount = 0

        eval_main_loop() {
            if (glfwWindowShouldClose(window) != 0 || glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || (maxFrames > 0 && frameCount >= maxFrames)) return false
            frameCount ++
            glfwPollEvents()

            // Delta time
            let now = glfwGetTime()
            let dt = float(now - last_time)
            last_time = now

            // Movement
            let speed = 5.0 * dt
            let fwd = camera_forward()
            let rgt = camera_right()
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera_pos += fwd * speed
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera_pos -= fwd * speed
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera_pos -= rgt * speed
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera_pos += rgt * speed
            }

            // Add source on N press
            let n_pressed = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS
            if (n_pressed && !n_was_pressed) {
                let spawn_pos = camera_pos + fwd * 3.0
                add_sound_source(spawn_pos)
                print("Added source #{length(sources)} at ({spawn_pos.x}, {spawn_pos.y}, {spawn_pos.z})\n")
            }
            n_was_pressed = n_pressed

            // Add 30 sources around camera on M press
            let m_pressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS
            if (m_pressed && !m_was_pressed) {
                for (i in range(30)) {
                    let angle = 2.0 * PI * float(i) / 30.0
                    let radius = 3.0 + float(i % 3)
                    let spawn_pos = camera_pos + float3(cos(angle) * radius, 0.0, sin(angle) * radius)
                    add_sound_source(spawn_pos, 0.1)
                }
                print("Added 30 sources around camera\n")
            }
            m_was_pressed = m_pressed

            // Cycle HRTF budget on B press
            let b_pressed = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS
            if (b_pressed && !b_was_pressed) {
                hrtf_budget_idx = (hrtf_budget_idx + 1) % 3
                let newBudget = HRTF_BUDGETS[hrtf_budget_idx]
                set_hrtf_budget(newBudget)
                print("HRTF budget: {newBudget} ({HRTF_BUDGET_LABELS[hrtf_budget_idx]})\n")
            }
            b_was_pressed = b_pressed

            // Per-second audio-system stats line. `get` is read-only — keeps the box alive across reads.
            // `grab` would consume the notification and break the periodic publish/poll loop.
            since_stats_print += dt
            if (since_stats_print >= 1.0) {
                stats_box |> get() $(s : AudioSystemStats#) {
                    print("audio: util={s.utilization_pct}%, hrtf={s.hrtf_count}/{s.total_3d} (budget={HRTF_BUDGETS[hrtf_budget_idx]})\n")
                }
                since_stats_print = 0.0
            }

            // Update audio listener
            set_head_position(gl_to_audio(camera_pos), gl_to_audio(fwd))

            // Update source positions (they don't move, but we could animate here)
            for (src in sources) {
                src.sid |> set_position(gl_to_audio(src.position))
            }

            // -- Render --
            var display_w, display_h : int
            glfwGetFramebufferSize(window, display_w, display_h)
            let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.0

            glViewport(0, 0, display_w, display_h)
            glClearColor(0.1, 0.1, 0.15, 1.0)
            glClearDepth(1.0lf)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            glEnable(GL_DEPTH_TEST)
            glDepthFunc(GL_LEQUAL)

            let view = look_at_rh(camera_pos, camera_pos + fwd, float3(0.0, 1.0, 0.0))
            let proj = perspective_rh_opengl(60.0 * PI / 180.0, aspect, 0.1, 100.0)
            v_view = view
            v_projection = proj

            // Grid
            draw_grid(line_program, view, proj)

            // Sound source spheres
            glUseProgram(solid_program)
            glEnable(GL_CULL_FACE)
            glCullFace(GL_BACK)
            for (src in sources) {
                v_model = compose(src.position, float4(0.0, 0.0, 0.0, 1.0), float3(0.3))
                f_color = src.color
                vs_solid_bind_uniform(solid_program)
                fs_solid_bind_uniform(solid_program)
                draw_geometry_fragment(sphere)
            }
            glUseProgram(0u)

            glfwSwapBuffers(window)
            return true
        }
    }
}
