options gen2
options indenting = 4

require live/glfw_live
require opengl/opengl_boost
require opengl/opengl_ttf
require live/opengl_live
require live/live_commands
require live/live_api
require live/live_watch_boost
require live_host
require daslib/math_boost
require daslib/safe_addr
require audio/audio_boost
require live/audio_live
require strudel/strudel
require strudel/strudel_player
require daslib/fio
require daslib/json
require daslib/json_boost
require audio_analysis
require vis_renderer

// USE_SF2 — toggle between SF2 soundfont and built-in synth
var USE_SF2 = true

// --- Shaders ---

var @in @location v_position : float2
var @in @location v_color : float4
var @inout f_color : float4
var @uniform v_projection : float4x4
var @out f_FragColor : float4

[vertex_program]
def vs_pianoroll {
    f_color = v_color
    gl_Position = v_projection * float4(v_position, 0.0, 1.0)
}

[fragment_program]
def fs_pianoroll {
    f_FragColor = f_color
}

// --- GL objects ---

var program : uint
var vao : uint
var vbo : uint

// --- Panel system ---

enum PanelKind {
    pianoroll_panel
    scope_panel
    spectrum_panel
    drums_panel
    vectorscope_panel
}

struct Panel {
    kind           : PanelKind
    name           : string
    color          : uint = 0xFFCA28u
    track_index    : int = -1   // index into track data (pianoroll captures, audio analyses, etc.)
}

// --- Globals ---

var render_states : array<PianorollRenderState>
var audio_analyses : array<AudioAnalysis>
var drum_panel_states : array<DrumPanelState>
var panels : array<Panel>
var font : Font
var g_window_cycles : float = 4.0

// --- GL setup ---

def create_gl_objects() {
    program = create_shader_program(@@vs_pianoroll, @@fs_pianoroll)
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao)
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    bind_vertex_buffer(null, type<VisVertex>)
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
}

// --- SF2 finder ---

def find_sf2() : string {
    let dir = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"
    var st : FStat
    for (name in fixed_array("Musyng Kite.sf2", "GeneralUser GS v1.471.sf2", "FluidR3_GM.sf2")) {
        if (stat("{dir}/{name}", st)) {
            return "{dir}/{name}"
        }
    }
    return ""
}

// --- Build panels from vis registrations ---

def build_panels() {
    for (i in range(length(g_vis_registrations))) {
        let reg & = unsafe(g_vis_registrations[i])
        let name = reg.name
        let color = reg.color
        if (reg.flags.has_pianoroll) {
            while (length(render_states) <= i) {
                render_states |> emplace(PianorollRenderState())
            }
            render_states[i].color = color
            render_states[i].name = name
            panels |> emplace(Panel(kind = PanelKind.pianoroll_panel, name = name, color = color, track_index = i))
        }
        if (reg.flags.has_drums) {
            while (length(render_states) <= i) {
                render_states |> emplace(PianorollRenderState())
            }
            render_states[i].color = color
            render_states[i].name = name
            drum_panel_states |> emplace(DrumPanelState())
            panels |> emplace(Panel(kind = PanelKind.drums_panel, name = name, color = color, track_index = i))
        }
        let needs_audio = reg.flags.has_spectrum || reg.flags.has_scope || reg.flags.has_vectorscope
        if (needs_audio) {
            while (length(audio_analyses) <= i) {
                var aa = AudioAnalysis()
                audio_analysis_init(aa)
                audio_analyses |> emplace(aa)
            }
        }
        if (reg.flags.has_scope) {
            panels |> emplace(Panel(kind = PanelKind.scope_panel, name = name, color = color, track_index = i))
        }
        if (reg.flags.has_spectrum) {
            panels |> emplace(Panel(kind = PanelKind.spectrum_panel, name = name, color = color, track_index = i))
        }
        if (reg.flags.has_vectorscope) {
            panels |> emplace(Panel(kind = PanelKind.vectorscope_panel, name = name, color = color, track_index = i))
        }
    }
}

// --- Live commands ---

[live_command(description="Dump debug info")]
def cmd_debug(input : JsonValue?) : JsonValue? {
    var info = "tracks: {length(g_vis_registrations)}, panels: {length(panels)}\n"
    for (i in range(length(render_states))) {
        var rs & = unsafe(render_states[i])
        info += "  pianoroll[{i}] \"{rs.name}\": {length(rs.haps)} haps, drum={rs.is_drum_mode}\n"
    }
    for (i in range(length(audio_analyses))) {
        var aa & = unsafe(audio_analyses[i])
        info += "  audio[{i}]: ring_write={aa.ring_write}\n"
    }
    let dbg_pt = strudel_get_playback_time()
    info += "playback: wall_time={dbg_pt._0}, cps={dbg_pt._1}\n"
    return JV(info)
}

// --- Lifecycle ---

[export]
def init() {
    live_create_window("daStrudel Visualizer", 1280, 720)
    create_gl_objects()
    create_ttf_objects()
    font <- load_ttf("{get_das_root()}/examples/opengl/droidsansmono.ttf", [pixel_height = 16.0, oversampling = 2])
    if (!!empty(font.bitmap.bytes)) {
        font <- load_ttf("{get_das_root()}/examples/opengl/media/droidsansmono.ttf", [oversampling = 2])
    }
    if (!!empty(font.bitmap.bytes)) {
        panic("can't load font")
    }
    upload_font_texture(font)
    // audio
    if (get_audio_command_channel() == null) {
        audio_system_create()
    }
    // load SF2 soundfont
    if (USE_SF2) {
        let sf2_path = find_sf2()
        if (!empty(sf2_path) && strudel_load_sf2(sf2_path)) {
            print("Loaded SF2: {sf2_path}\n")
        } else {
            print("No SF2 found, using regular strudel patterns\n")
            USE_SF2 = false
        }
    }
    // Main-thread mode: create own PCM stream
    strudel_create_channel()
    strudel_set_look_ahead(4.0)
    strudel_set_bpm(120.0lf)
    // Create patterns with visualization combinators
    if (USE_SF2) {
        /*
        strudel_add_track(
            s("hh hh") |> pianoroll("drums") |> vis_color(0xFF5722u) |> drums()
        )
        */
        strudel_add_track(
            note_pattern("c2 ~ [~ c2] ~, ~ eb2 ~ eb2, [~ fs2] [~ fs2] [~ fs2] [~ fs2]")
                |> set_sf2(0) |> set_sf2_bank(128) |> set_gain(0.7)
                |> pianoroll("drums") |> vis_color(0xFF5722u) |> drums()
        )
        strudel_add_track(
            note_pattern("<[a3 c4 e4 a4 e4 c4 a3 c4] [f3 a3 c4 f4 c4 a3 f3 a3] [c3 e3 g3 c4 g3 e3 c3 e3] [g3 b3 d4 g4 d4 b3 g3 b3]>")
                |> set_sf2("piano") |> set_gain(0.45)
                |> pianoroll("piano") |> vis_color(0x42A5F5u)
        )
        strudel_add_track(
            note_pattern("<[a2 ~ ~ a2 ~ a2 ~ ~] [f2 ~ ~ f2 ~ f2 ~ ~] [c2 ~ ~ c2 ~ c2 ~ ~] [g2 ~ ~ g2 ~ g2 ~ ~]>")
                |> set_sf2("acoustic_bass") |> set_gain(0.6)
                |> pianoroll("bass") |> vis_color(0x8BC34Au) |> scope()
        )
        strudel_add_track(
            note_pattern("<[a2,c3,e3] [f2,a2,c3] [c2,e2,g2] [g2,b2,d3]>")
                |> set_sf2("string_ensemble") |> set_gain(0.2)
                |> pianoroll("strings") |> vis_color(0xE040FFu) |> vectorscope()
        )
        strudel_add_track(
            note_pattern("<e4 ~ ~ a4> <~ d5 ~ ~> <g4 ~ f4 ~> <~ ~ b4 ~>")
                |> set_sf2("violin") |> set_gain(0.9)
                |> pianoroll("violin") |> vis_color(0xF06292u) |> spectrum()
        )
    } else {
        strudel_add_track(
            s("bd [hh hh] sd [hh cp]")
                |> pianoroll("drums") |> vis_color(0xFF5722u) |> drums()
        )
        strudel_add_track(
            note_pattern("<[a3 c4 e4 a4 e4 c4 a3 c4] [f3 a3 c4 f4 c4 a3 f3 a3] [c3 e3 g3 c4 g3 e3 c3 e3] [g3 b3 d4 g4 d4 b3 g3 b3]>", "sawtooth")
                |> set_gain(0.3) |> pianoroll("piano") |> vis_color(0x42A5F5u)
        )
        strudel_add_track(
            note_pattern("<[a2 ~ ~ a2 ~ a2 ~ ~] [f2 ~ ~ f2 ~ f2 ~ ~] [c2 ~ ~ c2 ~ c2 ~ ~] [g2 ~ ~ g2 ~ g2 ~ ~]>", "sine")
                |> set_gain(0.4) |> pianoroll("bass") |> vis_color(0x8BC34Au) |> scope()
        )
        strudel_add_track(
            note_pattern("<[a2,c3,e3] [f2,a2,c3] [c2,e2,g2] [g2,b2,d3]>", "sawtooth")
                |> set_gain(0.15) |> pianoroll("strings") |> vis_color(0xE040FFu) |> vectorscope()
        )
        strudel_add_track(
            note_pattern("<e4 ~ ~ a4> <~ d5 ~ ~> <g4 ~ f4 ~> <~ ~ b4 ~>", "sawtooth")
                |> set_gain(0.1) |> pianoroll("violin") |> vis_color(0xF06292u) |> spectrum()
        )
    }
    // Build panels from visualization registrations
    build_panels()
    // Warm up GL pipeline — real draw to force driver initialization before audio starts
    {
        var dw, dh : int
        live_get_framebuffer_size(dw, dh)
        glViewport(0, 0, dw, dh)
        glClearColor(0.0, 0.0, 0.0, 1.0)
        glClear(GL_COLOR_BUFFER_BIT)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        let projection = ortho_rh(0.0, float(dw), 0.0, float(dh), -1.0, 1.0)
        glUseProgram(program)
        v_projection = projection
        vs_pianoroll_bind_uniform(program)
        fs_pianoroll_bind_uniform(program)
        glBindVertexArray(vao)
        glBindBuffer(GL_ARRAY_BUFFER, vbo)
        var warmup_verts : array<VisVertex>
        push_quad(warmup_verts, 0.0, 0.0, 1.0, 1.0, float4(0.0, 0.0, 0.0, 0.0))
        glBufferData(GL_ARRAY_BUFFER, warmup_verts, GL_DYNAMIC_DRAW)
        glDrawArrays(GL_TRIANGLES, 0, length(warmup_verts))
        glFinish()
        live_end_frame()
        // second warmup frame (back buffer)
        glClear(GL_COLOR_BUFFER_BIT)
        glDrawArrays(GL_TRIANGLES, 0, length(warmup_verts))
        glFinish()
        live_end_frame()
    }
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    // tick strudel (main-thread mode)
    strudel_tick()
    var display_w, display_h : int
    live_get_framebuffer_size(display_w, display_h)
    let pt = strudel_get_playback_time()
    let current_cycle = float(pt._0 * pt._1)
    let dt = get_dt()
    // update pianoroll render states from captured haps
    for (i in range(length(g_pianoroll_captures))) {
        if (i < length(render_states)) {
            pianoroll_update(render_states[i], g_pianoroll_captures[i].haps, current_cycle, g_window_cycles)
        }
    }
    // feed per-track PCM to audio analyses
    for (i in range(length(audio_analyses))) {
        if (i < length(g_track_pcm) && length(g_track_pcm[i]) > 0) {
            audio_analysis_feed(audio_analyses[i], g_track_pcm[i])
        }
        audio_analysis_update_spectrum(audio_analyses[i])
    }
    // update drum panels
    for (pi in range(length(panels))) {
        let p & = unsafe(panels[pi])
        if (p.kind == PanelKind.drums_panel && p.track_index >= 0 && p.track_index < length(render_states)) {
            var dps_idx = 0
            for (qi in range(pi)) {
                if (panels[qi].kind == PanelKind.drums_panel) {
                    dps_idx ++
                }
            }
            if (dps_idx < length(drum_panel_states)) {
                drum_panel_update(drum_panel_states[dps_idx], render_states[p.track_index].haps, current_cycle, dt)
            }
        }
    }
    // render
    glViewport(0, 0, display_w, display_h)
    glClearColor(0.12, 0.12, 0.14, 1.0)
    glClear(GL_COLOR_BUFFER_BIT)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    let num_panels = length(panels)
    if (num_panels > 0) {
        let fw = float(display_w)
        let fh = float(display_h)
        let panel_h = fh / float(num_panels)
        let m = 4.0
        var verts : array<VisVertex>
        for (pi in range(num_panels)) {
            let p & = unsafe(panels[pi])
            let py1 = fh - panel_h * float(pi)
            let py0 = fh - panel_h * float(pi + 1)
            let iy0 = py0 + m
            let iy1 = py1 - m
            let col = uint_to_rgba(p.color)
            if (p.kind == PanelKind.pianoroll_panel && p.track_index >= 0 && p.track_index < length(render_states)) {
                pianoroll_render_panel(verts, render_states[p.track_index], current_cycle, g_window_cycles, display_w, iy0, iy1)
            } elif (p.kind == PanelKind.scope_panel && p.track_index >= 0 && p.track_index < length(audio_analyses)) {
                scope_render(verts, audio_analyses[p.track_index], m, iy0, fw - m, iy1, col)
            } elif (p.kind == PanelKind.spectrum_panel && p.track_index >= 0 && p.track_index < length(audio_analyses)) {
                spectrum_render(verts, audio_analyses[p.track_index], m, iy0, fw - m, iy1, col)
            } elif (p.kind == PanelKind.drums_panel) {
                var dps_idx = 0
                for (qi in range(pi)) {
                    if (panels[qi].kind == PanelKind.drums_panel) {
                        dps_idx ++
                    }
                }
                if (dps_idx < length(drum_panel_states)) {
                    drum_panel_render(verts, drum_panel_states[dps_idx], m, iy0, fw - m, iy1, col)
                }
            } elif (p.kind == PanelKind.vectorscope_panel && p.track_index >= 0 && p.track_index < length(audio_analyses)) {
                vectorscope_render(verts, audio_analyses[p.track_index], m, iy0, fw - m, iy1, col)
            }
            // separator
            if (pi > 0) {
                push_quad(verts, 0.0, py1 - 1.0, fw, py1 + 1.0, float4(0.5, 0.5, 0.5, 0.5))
            }
        }
        if (length(verts) > 0) {
            let projection = ortho_rh(0.0, fw, 0.0, fh, -1.0, 1.0)
            glUseProgram(program)
            v_projection = projection
            vs_pianoroll_bind_uniform(program)
            fs_pianoroll_bind_uniform(program)
            glBindVertexArray(vao)
            glBindBuffer(GL_ARRAY_BUFFER, vbo)
            glBufferData(GL_ARRAY_BUFFER, verts, GL_DYNAMIC_DRAW)
            glDrawArrays(GL_TRIANGLES, 0, length(verts))
        }
        // panel name labels
        for (pi in range(num_panels)) {
            let p & = unsafe(panels[pi])
            let py1 = fh - panel_h * float(pi)
            let py0 = fh - panel_h * float(pi + 1)
            render_panel_name(font, p.name, display_w, display_h, py0 + m, py1 - m)
        }
    }
    live_end_frame()
}

[export]
def shutdown() {
    delete render_states
    delete audio_analyses
    delete drum_panel_states
    delete panels
    delete_gl_objects()
    live_destroy_window()
}

// --- Standalone mode ---

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
