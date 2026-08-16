options gen2
options indenting = 4
options persistent_heap = true
options gc
options _allow_glfw_calls = true
options _allow_imgui_legacy = true

// Physarum Lab — an interactive showcase of daslang's threading on top of OpenGL. Millions of
// slime-mold agents each sense the trail field a few cells ahead (left / centre / right), steer
// toward the strongest scent, step forward and deposit a little trail of their own; the field is
// then diffused and decayed. Out of those local rules emerge the organic transport networks the
// real Physarum polycephalum builds. The simulation never converges, so the window stays alive.
//
// The worker-thread slider is the star: it partitions the agents across N background threads, and
// the agents/sec readout climbs as you add threads. The agent pass is dispatched from update() on
// the main thread and polled non-blocking next frame — never joined on the main thread — because
// emscripten cannot service a main-thread block on Web Workers in the browser (the same reason the
// Path Tracer Lab dispatches from update()). The pass is race-free by construction: a worker only
// READS the shared field (sensing) and writes its OWN slice of the agent array; the deposit
// (scatter) and the diffuse/decay run on the main thread after the workers join, so no two threads
// ever write the same memory. Uses the daslang-live harness so it live-reloads on desktop and runs
// in the wasm examples pipeline + the playground.

require math
require strings
require imgui/imgui_harness
require imgui/imgui_live
require opengl/opengl_boost
require glsl/glsl_opengl
require live/glfw_live
require stbimage
require daslib/safe_addr
require daslib/jobque_boost
require daslib/json_boost
require audio/audio_boost
require strudel/strudel
require strudel/strudel_player

let {
    TW = 512         // trail field resolution (toroidal). 512^2 keeps the networks detailed while
    TH = 512         // the diffuse pass stays a few ms on the cross-compiled card.
    WIN_W = 1024     // display window — a clean 2x NEAREST upscale of the field; CSS-scales on the web card.
    WIN_H = 1024
    MAX_AGENTS = 2000000   // slider ceiling — drag up into the millions on a fast (cross-compiled) build
    // Worker-thread ceiling. Decoupled from get_total_hw_threads() on purpose so you can oversubscribe
    // the real cores and watch the throughput curve. 16 == the wasm64 build's PTHREAD_POOL_SIZE
    // (daspkg), so the bands map 1:1 onto pre-spawned Web Workers.
    MAX_BANDS = 16
    DEG2RAD = 0.017453292f
    // tunable defaults (shared by init + the "reset to defaults" button)
    DEF_SENSOR_ANGLE = 22.0f
    DEF_SENSOR_DIST = 9.0f
    DEF_TURN_ANGLE = 40.0f
    DEF_STEP = 1.0f
    DEF_DEPOSIT = 0.20f
    DEF_DECAY = 0.92f
    DEF_DIFFUSE = 0.10f
    DEF_EXPOSURE = 1.4f
    DEF_AGENTS = 60000
    DEF_AGENTS_WEB_INTERP = 10000   // playground opening load — the interpreted-on-wasm runner is ~15x slower
    DEF_COL_LOW = float3(0.0, 0.0, 0.0)
    DEF_COL_MID = float3(1.0, 0.35, 0.0)
    DEF_COL_HIGH = float3(1.0, 0.96, 0.78)
}

struct Agent {
    x : float          // position in field space [0, TW) x [0, TH)
    y : float
    heading : float    // radians
}

// The per-pass simulation parameters, snapshotted on the main thread and passed BY VALUE into each
// worker. Workers run in their own contexts, so they must not read the main thread's mutable globals
// (each context has its own copy) — only compile-time consts (TW/TH) are safe to read directly.
struct SimParams {
    sensor_angle : float   // radians, L/R sensor offset from heading
    sensor_dist : float    // cells ahead the sensors sample
    turn_angle : float     // radians, steer step per frame
    step_size : float      // cells moved per frame
    frame : int            // varies the per-worker RNG between passes
}

// ----- simulation state (main-thread-owned; update() drives the passes) -----
var agents : array<Agent>
var field : array<float>              // 2*TW*TH — double-buffered (flip g_field_idx), toroidal
var g_field_idx = 0                   // which half of `field` is the live field

var g_agent_count = 0                 // agents currently allocated (matches agents |> length)
var g_target_agents = DEF_AGENTS      // slider target; applied when no workers are live
var g_threads = 1                     // worker-thread slider (1..MAX_BANDS; default set in init)
var g_reseed_pending = false          // reset button — clear the field and re-scatter the agents

// tunables (UI sliders) — all float so safe_addr matches SliderFloat's float? parameter
var g_sensor_angle_deg = DEF_SENSOR_ANGLE
var g_sensor_dist = DEF_SENSOR_DIST
var g_turn_angle_deg = DEF_TURN_ANGLE
var g_step_size = DEF_STEP
var g_deposit = DEF_DEPOSIT
var g_decay = DEF_DECAY
var g_diffuse = DEF_DIFFUSE
var g_exposure = DEF_EXPOSURE

// colormap gradient stops (live ImGui color pickers). Defaults reproduce the warm "fire" ramp.
var g_col_low = DEF_COL_LOW        // the decayed background (void)
var g_col_mid = DEF_COL_MID        // active trails (orange-red)
var g_col_high = DEF_COL_HIGH      // the brightest veins (warm white)

// ----- one non-blocking agent pass in flight, dispatched from update() -----
var g_status : JobStatus?             // wait-group for the in-flight pass (null = idle)
var g_pass_t0 = 0l                    // dispatch tick of the in-flight pass (update-time stat)
var g_prev_pass_tick = 0l             // tick of the previous pass completion (step-rate stat)

// ----- display stats -----
var g_last_pass_us = 0                // wall time of the last agent pass (workers), microseconds
var g_smooth_steps = 0.0lf            // exponentially-smoothed simulation steps/sec
var g_sim_frame = 0                       // completed passes

// ----- audio: strudel on the main thread (strudel_tick() in update). Each layer's per-ORBIT gain is
// faded live via strudel_fade_orbit from plain globals — no pattern rebuild (track gain stays at 1.0;
// strudel_tick auto-removes a track whose gain reaches 0). Ambient always plays; the lead swells
// while the mouse attracts the slime; the percussion swells while a slider is being dragged.
var g_asch : AudioSystemChannels
var g_audio_owned = false             // true only if WE created the audio system (else the host owns it)
var g_track_ambient = -1
var g_track_lead = -1
var g_track_perc = -1
var g_mouse_audio = false             // lead layer currently swelled (mouse held on the canvas)
var g_tweak_audio = false             // perc layer currently swelled (slider being dragged)
var g_tweak_hold = 0                  // frames the perc layer stays up after the last slider change
var g_ui_tweaked = false              // a slider/picker moved this frame (set by build_ui)

var g_audio_enabled = true            // gates the audio-owned paths below (audio runs on desktop and web)

// ----- gl objects -----
var display_program : uint
var vao, vbo, ebo : uint
var texture : uint                    // the field uploaded here for display
var upload_rgba : array<float4>       // RGBA staging for the upload (WebGL2 wants RGBA/FLOAT for RGBA32F)
var g_field_dirty = true              // re-stage the texture only when the field changed (the 512^2
                                      // staging loop is the main per-frame cost on the interpreted runner)

// ----- display shader: textured fullscreen quad, tonemap + colormap -----
var @in @location v_position : float2
var @in @location v_texcoord : float2
var @inout f_texcoord : float2
var @uniform f_tex : sampler2D
// no initializers: GLSL ES 3.00 (WebGL2) forbids uniform inits; the host sets them each frame
var @uniform u_exposure : float
var @uniform u_col_low : float3
var @uniform u_col_mid : float3
var @uniform u_col_high : float3
var @out f_FragColor : float4

[vertex_program]
def vs_effect {
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)
}

// A two-segment gradient through the three picked stops (low -> mid -> high), smoothstepped. The
// dark default low end lets the bright transport veins stand out against the decayed background.
def colormap(t : float; lo, mid, hi : float3) {
    let x = clamp(t, 0.0, 1.0)
    let c = lo + (mid - lo) * smoothstep(0.0, 0.5, x)   // == mid once x >= 0.5
    return c + (hi - mid) * smoothstep(0.5, 1.0, x)     // adds the mid->high segment for x > 0.5
}

[fragment_program]
def fs_effect {
    let v = texture(f_tex, f_texcoord).x
    let t = 1.0 - exp(-v * u_exposure)       // tonemap the unbounded field into [0,1)
    f_FragColor = float4(colormap(t, u_col_low, u_col_mid, u_col_high), 1.0)
}

[vertex_buffer]
struct EffectVertex {
    xy : float2
    uv : float2
}

let effect_vertices = [EffectVertex(
    xy = float2(-1.0, 1.0), uv = float2(0.0, 1.0)), EffectVertex(
    xy = float2(1.0, 1.0), uv = float2(1.0, 1.0)), EffectVertex(
    xy = float2(1.0, -1.0), uv = float2(1.0, 0.0)), EffectVertex(
    xy = float2(-1.0, -1.0), uv = float2(0.0, 0.0)
)]

let effect_indices = fixed_array<int>(0, 1, 2, 2, 3, 0)

// RGBA32F field texture. NEAREST is exact for the 2x upscale and needs no float-linear extension on WebGL2.
def make_surface_texture : uint {
    var tex : uint
    glGenTextures(1, safe_addr(tex))
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexImage2D(GL_TEXTURE_2D, 0, int(GL_RGBA32F), TW, TH, 0, GL_RGBA, GL_FLOAT, null)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
    return tex
}

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
    texture = make_surface_texture()
}

// ===== helpers =====

def read_off : int {
    return g_field_idx * TW * TH
}

def wrap_i(v, n : int) : int {
    var r = v % n
    if (r < 0) {
        r += n
    }
    return r
}

def wrap_f(v, n : float) : float {
    return v - floor(v / n) * n
}

// A tiny self-contained xorshift so workers need no shared RNG state (each has its own local seed).
def rand_next(var s : uint&) : uint {
    s ^= s << 13u
    s ^= s >> 17u
    s ^= s << 5u
    return s
}

def rand_f(var s : uint&) : float {
    return float(rand_next(s) >> 8u) * (1.0f / 16777216.0f)
}

// Nearest-cell read of the live field (called from workers; the deref is the only unsafe op).
def sense(pfield : array<float>?; off : int; px, py : float) : float {
    let ix = wrap_i(int(floor(px)), TW)
    let iy = wrap_i(int(floor(py)), TH)
    return (*pfield)[off + iy * TW + ix]
}

// ===== simulation =====

def reseed_agents {
    agents |> resize(g_target_agents)
    g_agent_count = g_target_agents
    var seed = 0x9e3779b9u + uint(g_sim_frame) * 2654435761u
    for (i in range(g_agent_count)) {
        let px = rand_f(seed) * float(TW)
        let py = rand_f(seed) * float(TH)
        let h = rand_f(seed) * 6.2831855f
        agents[i] = Agent(x = px, y = py, heading = h)
    }
}

def clear_field {
    for (c in field) {
        c = 0.0f
    }
    g_field_idx = 0
}

def current_params : SimParams {
    return SimParams(
        sensor_angle = g_sensor_angle_deg * DEG2RAD,
        sensor_dist = g_sensor_dist,
        turn_angle = g_turn_angle_deg * DEG2RAD,
        step_size = g_step_size,
        frame = g_sim_frame
    )
}

// One worker: advance agents [lo, hi). Reads the shared field (sensing) and writes only its own
// agent slice — no shared writes, so the pass is race-free across bands.
def update_agents_band(var pagents : array<Agent>?; pfield : array<float>?; read_offset, lo, hi : int; prm : SimParams) {
    var seed = uint(lo + 1) * 747796405u + uint(prm.frame) * 2891336453u + 1u
    for (i in range(lo, hi)) {
        var a = (*pagents)[i]
        let la = a.heading + prm.sensor_angle
        let ra = a.heading - prm.sensor_angle
        let left = sense(pfield, read_offset, a.x + cos(la) * prm.sensor_dist, a.y + sin(la) * prm.sensor_dist)
        let center = sense(pfield, read_offset, a.x + cos(a.heading) * prm.sensor_dist, a.y + sin(a.heading) * prm.sensor_dist)
        let right = sense(pfield, read_offset, a.x + cos(ra) * prm.sensor_dist, a.y + sin(ra) * prm.sensor_dist)
        if (center >= left && center >= right) {
            // strongest straight ahead — hold heading
        } elif (center < left && center < right) {
            a.heading += rand_f(seed) < 0.5f ? prm.turn_angle : -prm.turn_angle   // local minimum — turn randomly
        } elif (left > right) {
            a.heading += prm.turn_angle
        } else {
            a.heading -= prm.turn_angle
        }
        a.x = wrap_f(a.x + cos(a.heading) * prm.step_size, float(TW))
        a.y = wrap_f(a.y + sin(a.heading) * prm.step_size, float(TH))
        (*pagents)[i] = a
    }
}

// Dispatch n worker bands over the agent array and return immediately; advance_sim polls the
// wait-group next frame. SingleThread is just n == 1.
def start_update_pass(n : int) {
    var pagents : array<Agent>?
    var pfield : array<float>?
    unsafe {
        pagents = addr(agents)
        pfield = addr(field)
    }
    let read_offset = read_off()
    let prm = current_params()
    g_pass_t0 = ref_time_ticks()
    var status = job_status_create()
    status |> append(n)
    g_status = status
    let chunk = (g_agent_count + n - 1) / n
    for (c in range(n)) {
        let lo = c * chunk
        let hi = min((c + 1) * chunk, g_agent_count)
        var st = status
        if (lo >= hi) {
            st |> notify_and_release   // more bands than agents — balance the wait-group, skip the thread
            continue
        }
        // new_thread per band (the Path Tracer pattern), NOT new_job: the persistent jobque wedges
        // the interpreted wasm runner (playground), while per-pass threads work on every runner and
        // map onto the same pre-spawned Web Worker pool in the compiled build.
        new_thread() <| @capture(= pagents, = pfield, = read_offset, = lo, = hi, = prm, = st) {
            update_agents_band(pagents, pfield, read_offset, lo, hi, prm)
            st |> notify_and_release
        }
    }
}

// Scatter every agent's deposit into the live field (serial on the main thread — race-free).
def deposit_agents {
    let off = read_off()
    let dep = g_deposit          // hoisted: the interpreter re-reads a global on every iteration
    for (i in range(g_agent_count)) {
        let a = agents[i]
        let ix = wrap_i(int(floor(a.x)), TW)
        let iy = wrap_i(int(floor(a.y)), TH)
        field[off + iy * TW + ix] += dep
    }
}

// 3x3 blur toward the neighbourhood mean (by g_diffuse), then decay; read live -> write the other
// half and flip. Toroidal at the edges.
def diffuse_decay {
    let so = read_off()
    let dofs = (1 - g_field_idx) * TW * TH
    let dif = g_diffuse          // hoisted out of the 512^2 loop — see deposit_agents
    let dec = g_decay
    for (y in range(TH)) {
        let ym = wrap_i(y - 1, TH) * TW
        let yc = y * TW
        let yp = wrap_i(y + 1, TH) * TW
        for (x in range(TW)) {
            let xm = wrap_i(x - 1, TW)
            let xp = wrap_i(x + 1, TW)
            let sum = (field[so + ym + xm] + field[so + ym + x] + field[so + ym + xp] +
                field[so + yc + xm] + field[so + yc + x] + field[so + yc + xp] +
                field[so + yp + xm] + field[so + yp + x] + field[so + yp + xp])
            let c = field[so + yc + x]
            let mean = sum * (1.0f / 9.0f)
            field[dofs + yc + x] = (c + (mean - c) * dif) * dec
        }
    }
    g_field_idx = 1 - g_field_idx
}

// Drag on the canvas to paint a strong attractant blob; agents sense it and tendrils reach toward it.
// Runs on the main thread between passes (workers are joined), so it never races the sensing reads.
def apply_mouse_seed {
    if (GetIO().WantCaptureMouse || !IsMouseDown(ImGuiMouseButton.Left)) {
        return
    }
    let mp = GetMousePos()
    // GetMousePos is in ImGui DisplaySize (logical) coordinates, so normalise by DisplaySize -- NOT the
    // framebuffer size. Under HiDPI (SCALE_TO_MONITOR) the framebuffer is DisplaySize x devicePixelRatio,
    // so dividing by it would land the seed at 1/DPR of the cursor (half the click on a 2x display).
    let ds = GetIO().DisplaySize
    if (ds.x <= 0.0f || ds.y <= 0.0f) {
        return
    }
    let cx = wrap_i(int(mp.x / ds.x * float(TW)), TW)
    let cy = wrap_i(int((1.0f - mp.y / ds.y) * float(TH)), TH)   // flip Y: GL origin is bottom-left
    let off = read_off()
    let R = 9
    let r2 = float(R * R)
    let dep8 = g_deposit * 8.0f
    for (dy in range(-R, R + 1)) {
        for (dx in range(-R, R + 1)) {
            let d2 = float(dx * dx + dy * dy)
            if (d2 > r2) {
                continue
            }
            let ix = wrap_i(cx + dx, TW)
            let iy = wrap_i(cy + dy, TH)
            field[off + iy * TW + ix] += dep8 * exp(-d2 * 0.04f)
        }
    }
    g_field_dirty = true
}

// The pass state machine, driven once per frame from update() on the main thread.
def advance_sim {
    if (g_status != null) {
        return if (!g_status.isReady)
        var st = g_status
        unsafe {
            job_status_remove(st)
        }
        g_status = null
        g_last_pass_us = get_time_usec(g_pass_t0)
        if (g_prev_pass_tick != 0l) {
            let period = get_time_usec(g_prev_pass_tick)
            if (period > 0) {
                g_smooth_steps = g_smooth_steps * 0.9lf + (1.0e6lf / double(period)) * 0.1lf
            }
        }
        g_prev_pass_tick = ref_time_ticks()
        deposit_agents()
        diffuse_decay()
        g_field_dirty = true
        g_sim_frame++
    }
    // structural changes are safe here: no worker is touching agents/field
    if (g_reseed_pending) {
        clear_field()
        reseed_agents()
        g_reseed_pending = false
        g_field_dirty = true
    } elif (g_target_agents != g_agent_count) {
        reseed_agents()
    }
    apply_mouse_seed()
    start_update_pass(clamp(g_threads, 1, MAX_BANDS))
}

// ===== display + ui =====

def upload_field {
    return if (!g_field_dirty)
    g_field_dirty = false
    let off = read_off()
    for (i in range(TW * TH)) {
        upload_rgba[i] = float4(field[off + i], 0.0f, 0.0f, 1.0f)
    }
    glBindTexture(GL_TEXTURE_2D, texture)
    unsafe {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TW, TH, GL_RGBA, GL_FLOAT, addr(upload_rgba[0]))
    }
}

def draw_surface {
    var w, h : int
    live_get_framebuffer_size(w, h)
    glViewport(0, 0, w, h)
    glClearColor(0.04f, 0.04f, 0.05f, 1.0f)
    glClear(GL_COLOR_BUFFER_BIT)
    glUseProgram(display_program)
    f_tex := texture
    u_exposure = g_exposure
    u_col_low = g_col_low
    u_col_mid = g_col_mid
    u_col_high = g_col_high
    vs_effect_bind_uniform(display_program)
    fs_effect_bind_uniform(display_program)
    glBindVertexArray(vao)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
}

// ===== audio (strudel on the main thread) =====

def setup_audio {
    if (get_audio_command_stream() == null) {
        g_asch = audio_system_create()
        g_audio_owned = true
    }
    strudel_create_channel()
    strudel_set_look_ahead(4.0)
    strudel_set_cps(0.5lf)
    // ambient pad + sub bass — always present, slow and evolving
    g_track_ambient = strudel_add_track(stack([
        note("<[a2,e3,a3] [f2,c3,f3] [c3,g3,c4] [e2,b2,e3]>", "triangle") |> slow(2) |> gain(0.4) |> room(0.6),
        note("<a1 f1 c2 e1>", "sine") |> slow(2) |> gain(0.5)
    ]))
    // lead arpeggio — swells in while the mouse attracts the slime
    g_track_lead = strudel_add_track(
        note("a4 c5 e5 a5 g5 e5 c5 e5", "sawtooth") |> gain(0.45) |> room(0.4)
    )
    // percussion + bell — swells while a slider/picker is being dragged
    g_track_perc = strudel_add_track(stack([
        s("hh*8") |> gain(0.28),
        note("a5 e5 c6 e5", "square") |> gain(0.16) |> room(0.3)
    ]))
    // Mute the two interactive layers via their ORBIT bus, NOT track gain: strudel_tick auto-removes
    // any track whose gain (and target) reach 0, so fading the track to 0 deletes it. The orbit gain
    // silences the voices while the track stays alive at gain 1.0, ready to swell back in.
    strudel_set_orbit_gain(g_track_lead, 0, 0.0f)
    strudel_set_orbit_gain(g_track_perc, 0, 0.0f)
}

// Drive the live per-track gains from mouse/UI state (called inside the ImGui frame, so the io
// queries are valid). strudel_tick reads the faded gains on the main thread — no pattern rebuild.
def drive_audio {
    let mouse_active = !GetIO().WantCaptureMouse && IsMouseDown(ImGuiMouseButton.Left)
    if (mouse_active != g_mouse_audio) {
        g_mouse_audio = mouse_active
        strudel_fade_orbit(g_track_lead, 0, mouse_active ? 1.0f : 0.0f, mouse_active ? 0.12f : 0.8f)
    }
    if (g_ui_tweaked) {
        g_tweak_hold = 24            // hold the perc layer ~0.4s past the last change (debounce)
    } elif (g_tweak_hold > 0) {
        g_tweak_hold --
    }
    let want_tweak = g_tweak_hold > 0
    if (want_tweak != g_tweak_audio) {
        g_tweak_audio = want_tweak
        strudel_fade_orbit(g_track_perc, 0, want_tweak ? 1.0f : 0.0f, want_tweak ? 0.06f : 0.5f)
    }
}

// The interpreted playground runs the sim ~30x slower than the compiled wasm64 card, so it opens
// with a lighter agent load (the slider ceiling stays the same). MEASURED, not sniffed: the card
// is minted by the --jit-target=wasm64 exe rail, not AOT codegen, so is_in_aot() is false on
// BOTH wasm runtimes — a timed spin is the only signal that separates them. Web-only: desktop
// keeps the full defaults whatever its speed.
var g_light_defaults = false

def private measure_slow_runtime : bool {
    // pure-ALU spin, measured on daslang.io 2026-08-07: 2ms compiled wasm64, 140ms
    // interpreted — a 70x gap the 20ms threshold splits with x7 margin on either side.
    // (A builtin-heavy loop like sin narrows the gap enough to misread; don't.)
    let t0 = ref_time_ticks()
    var acc = 0u
    for (_i in range(10000000)) {
        acc = acc * 1664525u + 1013904223u
    }
    // acc feeds the result so the loop cannot fold away; the right arm is always true
    return get_time_usec(t0) > 20000 && acc != 12345u
}

def default_agents : int {
    return g_light_defaults ? DEF_AGENTS_WEB_INTERP : DEF_AGENTS
}

def reset_to_defaults {
    g_sensor_angle_deg = DEF_SENSOR_ANGLE
    g_sensor_dist = DEF_SENSOR_DIST
    g_turn_angle_deg = DEF_TURN_ANGLE
    g_step_size = DEF_STEP
    g_deposit = DEF_DEPOSIT
    g_decay = DEF_DECAY
    g_diffuse = DEF_DIFFUSE
    g_exposure = DEF_EXPOSURE
    g_col_low = DEF_COL_LOW
    g_col_mid = DEF_COL_MID
    g_col_high = DEF_COL_HIGH
    g_target_agents = default_agents()
    g_threads = clamp(get_total_hw_threads(), 4, MAX_BANDS)
}

def build_ui {
    Begin("Physarum Lab")
    // each `Widget(...) || changed` keeps the widget as the left operand so it always renders; the
    // accumulated `changed` swells the percussion layer while a control is being dragged.
    var changed = false
    changed = SliderInt("worker threads", safe_addr(g_threads), 1, MAX_BANDS) || changed
    changed = SliderInt("agents", safe_addr(g_target_agents), 1000, MAX_AGENTS) || changed
    if (Button("reset")) {
        g_reseed_pending = true
    }
    if (Button("reset to defaults")) {
        reset_to_defaults()
    }
    Separator()
    changed = SliderFloat("sensor angle", safe_addr(g_sensor_angle_deg), 2.0, 80.0) || changed
    changed = SliderFloat("sensor distance", safe_addr(g_sensor_dist), 1.0, 32.0) || changed
    changed = SliderFloat("turn angle", safe_addr(g_turn_angle_deg), 2.0, 90.0) || changed
    changed = SliderFloat("step size", safe_addr(g_step_size), 0.2, 3.0) || changed
    changed = SliderFloat("deposit", safe_addr(g_deposit), 0.01, 1.0) || changed
    changed = SliderFloat("decay", safe_addr(g_decay), 0.5, 0.99) || changed
    changed = SliderFloat("diffuse", safe_addr(g_diffuse), 0.0, 1.0) || changed
    changed = SliderFloat("exposure", safe_addr(g_exposure), 0.1, 4.0) || changed
    changed = ColorEdit3("low color", unsafe(addr(g_col_low.x))) || changed
    changed = ColorEdit3("mid color", unsafe(addr(g_col_mid.x))) || changed
    changed = ColorEdit3("high color", unsafe(addr(g_col_high.x))) || changed
    g_ui_tweaked = changed
    Separator()
    let agents_per_sec = double(g_agent_count) * g_smooth_steps / 1.0e6lf
    Text("agents: {g_agent_count}   threads: {g_threads}")
    Text("steps/s: {fmt(":.1f", g_smooth_steps)}   agents/s: {fmt(":.1f", agents_per_sec)}M")
    Text("update: {fmt(":.2f", double(g_last_pass_us) / 1000.0lf)} ms")
    Text("drag on the canvas to attract the slime")
    End()
}

// Read back the framebuffer and write a PNG (flips vertically — GL origin is bottom-left). Used by
// the --max-frames headless smoke path; live sessions use the built-in `screenshot` command.
def save_screenshot(fname : string) {
    var w, h : int
    live_get_framebuffer_size(w, h)
    if (w <= 0 || h <= 0) {
        return
    }
    var pixels : array<uint8>
    pixels |> resize(w * h * 4)
    unsafe {
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, addr(pixels[0]))
    }
    let row_bytes = w * 4
    var temp : array<uint8>
    temp |> resize(row_bytes)
    for (y in range(h / 2)) {
        let top = y * row_bytes
        let bot = (h - 1 - y) * row_bytes
        unsafe {
            memcpy(addr(temp[0]), addr(pixels[top]), row_bytes)
            memcpy(addr(pixels[top]), addr(pixels[bot]), row_bytes)
            memcpy(addr(pixels[bot]), addr(temp[0]), row_bytes)
        }
    }
    unsafe {
        stbi_write_png(fname, w, h, 4, addr(pixels[0]), w * 4)
    }
    print("wrote {fname} ({w}x{h})\n")
}

struct CmdThreadsArgs {
    threads : int
}

[live_command(description="Set worker thread count")]
def cmd_threads(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdThreadsArgs>)
    g_threads = clamp(args.threads, 1, MAX_BANDS)
    return JV(args)
}

[export]
def init() {
    harness_init("Physarum Lab", WIN_W, WIN_H)
    create_gl_objects()
    field |> resize(2 * TW * TH)
    upload_rgba |> resize(TW * TH)
    clear_field()
    if (get_running_platform_name() == "emscripten") {
        g_light_defaults = measure_slow_runtime()
    }
    g_target_agents = default_agents()
    reseed_agents()
    // Open at the detected core count, floored to 4 so it never looks single-threaded on a browser
    // that clamps navigator.hardwareConcurrency, and capped at MAX_BANDS. Drag the slider up to oversubscribe.
    g_threads = clamp(get_total_hw_threads(), 4, MAX_BANDS)
    if (g_audio_enabled) {
        setup_audio()
    }
}

[export]
def update() {
    if (!harness_begin_frame()) {
        return
    }
    if (g_audio_enabled) {
        strudel_tick()       // advance the strudel scheduler on the main thread
    }
    harness_new_frame()
    build_ui()
    if (g_audio_enabled) {
        drive_audio()        // fade the lead/perc layers from mouse + slider state (reads ImGui io)
    }
    advance_sim()
    if (g_audio_enabled) {
        strudel_tick()       // top up audio after the heavy collect/diffuse, while the workers run
    }
    upload_field()
    draw_surface()
    end_of_frame()
    Render()
    live_imgui_render()
    live_end_frame()
}

[export]
def shutdown() {
    if (g_status != null) {
        g_status |> join
        unsafe {
            job_status_remove(g_status)
        }
        g_status = null
    }
    if (g_audio_enabled) {
        strudel_shutdown()
        // Only finalize the audio system if WE created it — an embedded host / playground that already
        // owns the audio stream keeps it (and g_asch is unset, so finalizing it would deref a null command).
        if (g_audio_owned) {
            audio_system_finalize(g_asch.command, g_asch.next_sid)
            g_audio_owned = false
        }
    }
    harness_shutdown()
}

[export]
def main {
    var max_frames = 0
    let args <- get_command_line_arguments()
    for (i in range(length(args) - 1)) {
        if (args[i] == "--max-frames") {
            max_frames = to_int(args[i + 1])
        }
    }
    init()
    var frame = 0
    while (!exit_requested()) {
        update()
        maybe_collect_gc()
        frame++
        if (max_frames > 0) {
            if (frame == max_frames - 1) {
                save_screenshot("{get_das_root()}/examples/graphics/physarum_lab_proof.png")
            }
            if (frame >= max_frames) {
                break
            }
        }
    }
    shutdown()
}
