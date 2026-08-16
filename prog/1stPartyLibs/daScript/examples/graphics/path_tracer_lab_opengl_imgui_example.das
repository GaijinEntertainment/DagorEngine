options gen2
options indenting = 4
options persistent_heap = true
options gc
options _allow_glfw_calls = true
options _allow_imgui_legacy = true

// Path Tracer Lab — an interactive showcase of daslang's threading on top of OpenGL. The window
// displays the path tracer's accumulation surface; an ImGui panel switches the render backend live:
// CPU single-thread, CPU threads, or the OpenGL (GPU) path. update() dispatches the CPU passes to
// background band-threads it polls non-blocking — one thread for single-thread, several for threads —
// so the UI never freezes (a main-thread crawl would: one traced row is hundreds of ms in the wasm
// interpreter). Dispatching from the main thread (which yields between frames) — not a background
// coordinator — is what makes the threading work in the wasm interpreter, where emscripten can't
// service worker-initiated thread creation while main() is busy. Switching a CPU mode
// clears the surface so you watch it fill, then refine. The GL display loop never blocks. Uses the
// daslang-live harness so it live-reloads on desktop and runs in the wasm examples pipeline + the
// playground. The path tracer header is reused by relative require.
//
// (No JobQue mode: with_job_que creates a worker pool whose pthread spawn can't be serviced while
// main() is busy in the wasm interpreter, and it is perf-equivalent to the Threads mode here anyway.)

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
require daslib/random
require daslib/json_boost

require ../pathTracer/path_tracer.das

let {
    TRACE_W = 256    // trace surface: kept modest so the CPU fill is watchable on the web (the
    TRACE_H = 256    // 4-thread cap means a 512^2 single-thread pass is ~6 s; 256^2 is a snappy ~1.6 s)
    WIN_W = 512      // display window — CSS-scales to the examples-page card; 2x NEAREST upscale of the trace
    WIN_H = 512
    // Threads-mode band ceiling. Decoupled from get_total_hw_threads() on purpose: the lab lets you
    // oversubscribe the real cores to watch starved threads. 16 == the wasm64 build's PTHREAD_POOL_SIZE
    // (daspkg), so the bands map 1:1 onto pre-spawned Web Workers — going higher trips emscripten's
    // pool-exhausted warning. Privacy browsers (Vivaldi clamps navigator.hardwareConcurrency to 2) can
    // still reach 16 here even though their detected count is 1.
    MAX_BANDS = 16
}

enum Mode {
    SingleThread
    Threads
    OpenGL
}

// ----- render state (all main-thread-owned; update() drives the passes) -----
// Threading is dispatched FROM update() (the main thread), never from a background coordinator:
// emscripten can't service worker-initiated thread creation while main() is busy, so dispatching the
// band threads from a background worker would deadlock. The main thread yields between frames (harness
// rAF), so it can both create threads and stay responsive. Band threads share the surface by raw
// pointer (shared wasm memory under -pthread; shared address space on desktop).

var g_mode = int(Mode.SingleThread)   // UI render mode
var g_epoch = 0                       // bumped on mode switch; resets the pass
var g_render_epoch = -1               // epoch the render state machine is synced to (detects switches)
var g_pass = -1                       // completed accumulation passes (frameCount-1)
var g_last_rays = 0                   // rays of the last completed full-image pass (stats)
var g_last_pass_us = 0                // microseconds of the last completed pass (stats)
var g_max_threads = 1                 // Threads-mode band count (slider 1..MAX_BANDS; default set in init)

var backbuffer : array<float3>        // TRACE_W*TRACE_H accumulation surface (linear color)
var upload_rgba : array<float4>       // RGBA staging for the CPU->texture upload (WebGL2 requires RGBA/FLOAT for an RGBA32F texture, not RGB)

// ----- Threads: one non-blocking pass in flight, dispatched from update() (SingleThread = n==1) -----
var g_threads_status : JobStatus?     // wait-group for the in-flight threaded pass (null = idle)
var g_threads_rays = 0                // ray accumulator for the in-flight pass (benign race; read after isReady)
var g_threads_t0 = 0l                 // start tick of the in-flight pass
var g_threads_epoch = 0               // epoch the in-flight pass was dispatched at (discard if it moved)

// ----- main-thread-only display state -----
var g_smooth_rps = 0.0lf              // exponentially-smoothed rays/sec (the raw number is far too jumpy under JIT)
var g_gpu_us = 0                      // wall time of the last GPU frame in microseconds (frame-to-frame delta)
var g_gpu_prev_tick = 0l             // tick of the previous GPU dispatch (0 = none yet, e.g. just switched into GPU mode)

// ----- gl objects -----
var display_program : uint
var pt_program : uint
var vao, vbo, ebo : uint
var texture : uint                    // CPU modes upload the accumulation surface here
var accum_tex : uint[2]               // GPU mode: ping-pong RGBA32F accumulation textures
var accum_fbo : uint                  // GPU mode: render-to-texture target
var g_accum_idx = 0                   // index of accum_tex holding the latest accumulation (the one we display)

// ----- display shader: textured fullscreen quad, linear -> sRGB -----
var @in @location v_position : float2
var @in @location v_texcoord : float2
var @inout f_texcoord : float2
var @uniform f_tex : sampler2D
var @out f_FragColor : float4

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
    f_FragColor = float4(linear_to_RGB(texture(f_tex, f_texcoord).xyz).xyz, 1.0)
}

// ----- GPU path tracer: a fragment shader that traces one sample per pixel and accumulates across
// frames via a ping-pong FBO (reads the previous accumulation, blends by sample count). WebGL2 has
// no compute shaders, so doing the GPU path as a fragment shader makes it one code path on both
// desktop GL and the web. -----
var @uniform pt_prev : sampler2D      // the previous accumulation (ping-pong source)
var @uniform c_frame : int            // samples accumulated so far (0 = first sample, ignore prev). No initializer: GLSL ES 3.00 (WebGL2) forbids uniform initializers; the host sets it each frame.
var @out pt_FragColor : float4

[fragment_program]
def pt_effect {
    init_path_tracer()
    let storePos = int2(f_texcoord * float2(float(TRACE_W), float(TRACE_H)))
    random_seed2D(RAND_SEED, storePos, c_frame)
    random_float4(RAND_SEED)
    var nrgb = draw_ray(storePos.x, storePos.y, TRACE_W, TRACE_H)
    if (c_frame != 0) {
        let orgb = texture(pt_prev, f_texcoord).xyz
        let wf = float(c_frame)
        nrgb = (orgb * wf + nrgb) / (wf + 1.)
    }
    pt_FragColor = float4(nrgb, 1.0)
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

// An RGBA32F texture sized to the trace surface. Used for the CPU upload target and both GPU
// ping-pong accumulators. NEAREST filtering is exact for the ping-pong read (f_texcoord lands on
// texel centers) and, unlike LINEAR, needs no OES_texture_float_linear extension on WebGL2.
def make_surface_texture : uint {
    var tex : uint
    glGenTextures(1, safe_addr(tex))
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexImage2D(GL_TEXTURE_2D, 0, int(GL_RGBA32F), TRACE_W, TRACE_H, 0, GL_RGBA, GL_FLOAT, null)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
    return tex
}

def create_gl_objects {
    display_program = create_shader_program(@@vs_effect, @@fs_effect)
    pt_program = create_shader_program(@@vs_effect, @@pt_effect)
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
    accum_tex[0] = make_surface_texture()
    accum_tex[1] = make_surface_texture()
    glGenFramebuffers(1, safe_addr(accum_fbo))
}

// ===== render (driven from update() on the main thread) =====

def clear_surface {
    for (i in range(TRACE_W * TRACE_H)) {
        backbuffer[i] = float3(0.0)
    }
}

// Trace rows [ymin, ymax) for one pass into the shared surface, aborting promptly if the epoch
// changes (mode switch). Runs in whatever context calls it — the main thread (single-thread crawl)
// or a band thread. The path tracer globals are this context's own, so init them here.
def render_rows(var pbb : array<float3>?; var pepoch : int?; my_epoch, pidx, ymin, ymax : int) : int {
    init_path_tracer()
    var rays = 0
    for (y in range(ymin, ymax)) {
        if (*pepoch != my_epoch) {
            break
        }
        var rc = 0
        unsafe {
            draw(pidx, TRACE_W, TRACE_H, y, y + 1, *pbb, rc)
        }
        rays += rc
    }
    return rays
}

// A worker (one band thread) traces its band and adds its rays to the shared counter (benign
// race; read only after join). The captured wait-group handle is released by the lambda itself.
def worker_band(var pbb : array<float3>?; var pepoch, pray : int?; my_epoch, pidx, ymin, ymax : int) {
    let rays = render_rows(pbb, pepoch, my_epoch, pidx, ymin, ymax)
    unsafe {
        *pray += rays
    }
}

// Threads mode: dispatch n band-threads from the main thread and return immediately — the threads
// fill their bands in the background and signal a persistent wait-group we poll next frame. This is
// the non-blocking half: you watch n bands fill at once while the UI stays live.
def start_threads_pass(pidx, n : int) {
    var pbb : array<float3>?; var pepoch, pray : int ?
    unsafe {
        pbb = addr(backbuffer)
        pepoch = addr(g_epoch)
        pray = addr(g_threads_rays)
    }
    g_threads_rays = 0
    g_threads_t0 = ref_time_ticks()
    g_threads_epoch = g_epoch
    var status = job_status_create()
    status |> append(n)
    g_threads_status = status
    let chunk = (TRACE_H + n - 1) / n
    let my_epoch = g_epoch
    for (c in range(n)) {
        let ymin = c * chunk
        let ymax = min((c + 1) * chunk, TRACE_H)
        var st = status
        new_thread() <| @capture(= pbb, = pepoch, = pray, = my_epoch, = pidx, = ymin, = ymax, = st) {
            worker_band(pbb, pepoch, pray, my_epoch, pidx, ymin, ymax)
            st |> notify_and_release
        }
    }
}

// The pass state machine, driven once per frame from update() on the main thread. Dispatching the
// band-threads here — rather than from a background coordinator — is what makes it work under
// emscripten pthreads (the main thread yields between frames, so it can service thread creation).
def advance_render {
    // 1. Drain an in-flight threaded pass first, whatever the current mode, so its wait-group is
    //    always released. On a mode switch the band threads bail via the epoch check, so isReady
    //    flips quickly; discard the pass if the epoch moved under it.
    if (g_threads_status != null) {
        return if (!g_threads_status.isReady)
        if (g_threads_epoch == g_epoch) {
            g_pass++
            g_last_rays = g_threads_rays
            g_last_pass_us = get_time_usec(g_threads_t0)
        }
        var st = g_threads_status
        unsafe {
            job_status_remove(st)
        }
        g_threads_status = null
    }
    // 2. Mode/epoch switch → reset the accumulation (watch it clear, then fill).
    if (g_render_epoch != g_epoch) {
        g_render_epoch = g_epoch
        g_pass = -1
        clear_surface()
    }
    // 3. Dispatch for the current mode.
    let mode = g_mode
    if (mode == int(Mode.SingleThread)) {
        start_threads_pass(g_pass + 1, 1)        // one background band-thread; UI stays free (no main-thread crawl)
    } elif (mode == int(Mode.Threads)) {
        start_threads_pass(g_pass + 1, clamp(g_max_threads, 1, MAX_BANDS))
    }
}

// ===== display + ui =====

def upload_surface {
    // Pad the float3 surface to RGBA: WebGL2 only accepts a (RGBA, FLOAT) upload into an RGBA32F
    // texture (desktop GL tolerates the bare RGB upload, but the web path rejects it -> black).
    for (i in range(TRACE_W * TRACE_H)) {
        upload_rgba[i] = float4(backbuffer[i], 1.0)
    }
    glBindTexture(GL_TEXTURE_2D, texture)
    unsafe {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TRACE_W, TRACE_H, GL_RGBA, GL_FLOAT, addr(upload_rgba[0]))
    }
}

// GPU mode: one fragment-shader pass per frame traces a sample per pixel into the ping-pong
// destination texture, blending with the previous accumulation (c_frame == 0 starts fresh).
// Throughput is rays-per-dispatch / wall-between-dispatches: a WebGL2 glFinish does NOT actually
// block until the GPU finishes, so a single dispatch can't be timed (that gave absurd rays/sec on
// the web); the frame-to-frame wall is the honest achieved rate, and is correct on desktop too.
def run_gpu_pass {
    let src = g_accum_idx
    let dst = 1 - g_accum_idx
    glBindFramebuffer(GL_FRAMEBUFFER, accum_fbo)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accum_tex[dst], 0)
    glViewport(0, 0, TRACE_W, TRACE_H)
    glUseProgram(pt_program)
    pt_prev := accum_tex[src]
    pt_effect_bind_uniform(pt_program)
    glBindVertexArray(vao)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
    glBindFramebuffer(GL_FRAMEBUFFER, 0u)
    if (g_gpu_prev_tick != 0l) {
        g_gpu_us = get_time_usec(g_gpu_prev_tick)
    }
    g_gpu_prev_tick = ref_time_ticks()
    g_accum_idx = dst
    c_frame++
}

def draw_surface(src_tex : uint) {
    var w, h : int
    live_get_framebuffer_size(w, h)
    glViewport(0, 0, w, h)
    glClearColor(0.051f, 0.047f, 0.039f, 1.0f)
    glClear(GL_COLOR_BUFFER_BIT)
    glUseProgram(display_program)
    f_tex := src_tex
    vs_effect_bind_uniform(display_program)
    fs_effect_bind_uniform(display_program)
    glBindVertexArray(vao)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
}

// Only the single-thread and OpenGL modes use this; Threads shows its parallelism slider.
def mode_label(mode : int) : string {
    if (mode == int(Mode.OpenGL)) {
        return "GPU fragment shader"
    }
    return "1 thread"
}

def build_ui {
    Begin("Path Tracer Lab")
    Text("Render mode")
    let prev = g_mode
    RadioButton("CPU - single thread", safe_addr(g_mode), int(Mode.SingleThread))
    RadioButton("CPU - threads", safe_addr(g_mode), int(Mode.Threads))
    RadioButton("OpenGL (GPU)", safe_addr(g_mode), int(Mode.OpenGL))
    if (g_mode != prev) {
        g_epoch++            // tell advance_render to abort + clear + restart in the new mode
        g_smooth_rps = 0.0lf // rays/sec magnitude differs wildly per mode — ramp fresh, don't lag-blend
        g_gpu_prev_tick = 0l // no previous GPU frame to delta against after a switch
    }
    Separator()

    // per-mode control (the slider value doubles as the parallelism readout)
    if (g_mode == int(Mode.Threads)) {
        SliderInt("max threads", safe_addr(g_max_threads), 1, MAX_BANDS)
    } else {
        Text("{mode_label(g_mode)}")
    }

    // stats — samples, smoothed rays/sec, and the pass/dispatch time
    var samples = 0
    var instant_rps = 0.0lf
    var time_text = ""
    if (g_mode == int(Mode.OpenGL)) {
        samples = c_frame
        // the GPU does the same full-image, same-spp work per dispatch as a CPU pass, so the last
        // CPU pass's ray count is a faithful per-dispatch estimate (GPU-side ray counting would need an SSBO atomic).
        instant_rps = g_gpu_us > 0 ? double(g_last_rays) / double(g_gpu_us) : 0.0lf
        time_text = "gpu: {fmt(":.2f", double(g_gpu_us) / 1000.0lf)} ms"
    } else {
        samples = g_pass + 1
        instant_rps = g_last_pass_us > 0 ? double(g_last_rays) / double(g_last_pass_us) : 0.0lf
        time_text = "pass: {fmt(":.1f", double(g_last_pass_us) / 1000.0lf)} ms"
    }
    g_smooth_rps = g_smooth_rps * 0.92lf + instant_rps * 0.08lf
    Text("samples: {samples}   rays/s: {fmt(":.1f", g_smooth_rps)}M   {time_text}")
    End()
}

// Read back the framebuffer and write a PNG (flips vertically — GL origin is bottom-left). Used by
// the --max-frames headless smoke path; live sessions use the built-in `screenshot` command instead.
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

struct CmdModeArgs {
    mode : int
}

[live_command(description="Set render mode (0=single, 1=threads, 2=opengl)")]
def cmd_mode(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdModeArgs>)
    g_mode = clamp(args.mode, 0, 2)
    g_epoch++
    g_gpu_prev_tick = 0l
    return JV(args)
}

[export]
def init() {
    harness_init("Path Tracer Lab", WIN_W, WIN_H)
    create_gl_objects()
    backbuffer |> resize(TRACE_W * TRACE_H)
    upload_rgba |> resize(TRACE_W * TRACE_H)
    // Open at the detected core count, floored to 4 so it never looks single-threaded on a browser
    // that clamps navigator.hardwareConcurrency (e.g. Vivaldi -> 2 -> get_total_hw_threads() == 1),
    // and capped at the MAX_BANDS ceiling. Drag the slider up to MAX_BANDS to oversubscribe.
    g_max_threads = clamp(get_total_hw_threads(), 4, MAX_BANDS)
}

[export]
def update() {
    if (!harness_begin_frame()) {
        return
    }
    harness_new_frame()
    build_ui()
    advance_render()
    if (g_mode == int(Mode.OpenGL)) {
        run_gpu_pass()                          // GPU traces one sample into the ping-pong target
        draw_surface(accum_tex[g_accum_idx])    // display the freshest accumulation
    } else {
        c_frame = 0                             // keep the GPU accumulator reset while a CPU mode owns the surface
        upload_surface()                        // CPU accumulation surface -> texture
        draw_surface(texture)
    }
    end_of_frame()
    Render()
    live_imgui_render()
    live_end_frame()
}

[export]
def shutdown() {
    g_epoch++       // make any in-flight band threads bail via the epoch check, then drain them
    if (g_threads_status != null) {
        g_threads_status |> join
        unsafe {
            job_status_remove(g_threads_status)
        }
        g_threads_status = null
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
                save_screenshot("{get_das_root()}/examples/graphics/path_tracer_lab_proof.png")
            }
            if (frame >= max_frames) {
                break
            }
        }
    }
    shutdown()
}
