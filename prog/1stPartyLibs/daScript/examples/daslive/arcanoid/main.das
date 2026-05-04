options gen2
// arcanoid live
require glfw/glfw_boost
require live/glfw_live
require opengl/opengl_boost
require opengl/opengl_gen
require opengl/opengl_cache
require daslib/defer
require daslib/safe_addr
require daslib/decs_boost
require live/decs_live
require live/opengl_live
require live/live_commands
require live/live_api
require live/live_watch_boost
require live/live_vars
require live/audio_live
require daslib/json
require daslib/json_boost
require daslib/math_boost
require daslib/archive
require opengl/opengl_ttf
require audio/audio_boost
require strudel/strudel
require strudel/strudel_player
require strudel/strudel_live
require daslib/strings_boost
require live_host

// --- Constants ---

let LIGHT_DIRECTION = normalize(float3(-2.0, -2.0, 1.5))

let FIELD_W = 10.0
let FIELD_H = 14.0
let WALL_THICKNESS = 0.5
let WALL_HEIGHT = 0.6
let GROUND_Y = -0.1

let WALL_COLOR = float3(0.4, 0.4, 0.5)
let GROUND_COLOR = float3(0.3, 0.25, 0.38)
let PADDLE_COLOR = float3(0.2, 0.6, 1.0)
let PADDLE_W = 2.0
let PADDLE_H = 0.3
let PADDLE_D = 0.4
let PADDLE_Z = 0.5
let PADDLE_SPEED = 8.0

let BALL_COLOR = float3(1.0, 0.9, 0.3)
let BALL_RADIUS = 0.2
let BALL_SPEED = 8.0
let BALL_START_Z = 1.5

let BRICK_COLS = 8
let BRICK_ROWS = 5
let BRICK_W = 1.0
let BRICK_D = 0.5
let BRICK_H = 0.4
let BRICK_GAP = 0.1
let BRICK_START_Z = 8.0

let BRICK_COLORS = fixed_array(
    float3(0.9, 0.2, 0.2),
    float3(0.9, 0.5, 0.1),
    float3(0.9, 0.9, 0.2),
    float3(0.2, 0.8, 0.3),
    float3(0.3, 0.5, 0.9)
)

let BRICK_POINTS = fixed_array(50, 40, 30, 20, 10)
let START_LIVES = 3

let BONUS_DROP_CHANCE = 0.25
let BONUS_FALL_SPEED = 3.0
let BONUS_SIZE = 0.4
let POWERUP_DURATION = 10.0  // seconds for timed effects

// --- Audio ---

let AUDIO_RATE = 48000
let AUDIO_CHANNELS = 1

def gen_sine_sweep(freq_start, freq_end : float; duration : float; volume : float = 0.3) : array<float> {
    let num_samples = int(float(AUDIO_RATE) * duration)
    var samples : array<float>
    samples |> resize(num_samples)
    for (i in range(num_samples)) {
        let t = float(i) / float(AUDIO_RATE)
        let frac = t / duration
        let freq = freq_start + (freq_end - freq_start) * frac
        let envelope = 1.0 - frac  // linear fade out
        samples[i] = sin(2.0 * PI * freq * t) * envelope * volume
    }
    return <- samples
}

def gen_noise_burst(duration : float; volume : float = 0.2) : array<float> {
    let num_samples = int(float(AUDIO_RATE) * duration)
    var samples : array<float>
    samples |> resize(num_samples)
    var seed = 12345u
    for (i in range(num_samples)) {
        let frac = float(i) / float(num_samples)
        let envelope = 1.0 - frac
        // Simple LCG random
        seed = seed * 1103515245u + 12345u
        let noise = float(int(seed >> 16u) % 1000) / 500.0 - 1.0
        samples[i] = noise * envelope * volume
    }
    return <- samples
}

def mix_into(var dst : array<float>; src : array<float>) {
    let n = min(length(dst), length(src))
    for (i in range(n)) {
        dst[i] += src[i]
    }
}

var @live snd_paddle_hit : array<float>
var @live snd_brick_break : array<float>
var @live snd_wall_bounce : array<float>
var @live snd_ball_miss : array<float>
var @live snd_win : array<float>
var @live snd_game_over : array<float>
var @live audio_initialized = false

def init_audio_samples() {
    // Paddle hit: chirp 440→880Hz, 50ms
    snd_paddle_hit <- gen_sine_sweep(440.0, 880.0, 0.05, 0.3)
    // Brick break: noise + sine
    snd_brick_break <- gen_noise_burst(0.08, 0.15)
    let brick_tone <- gen_sine_sweep(600.0, 200.0, 0.08, 0.2)
    mix_into(snd_brick_break, brick_tone)
    // Wall bounce: short high tick
    snd_wall_bounce <- gen_sine_sweep(1200.0, 800.0, 0.02, 0.15)
    // Ball miss: descending tone
    snd_ball_miss <- gen_sine_sweep(440.0, 110.0, 0.3, 0.25)
    // Bonus pickup: bright rising chirp
    snd_bonus_pickup <- gen_sine_sweep(600.0, 1200.0, 0.08, 0.25)
    // Bonus bad: descending buzz
    snd_bonus_bad <- gen_sine_sweep(400.0, 200.0, 0.1, 0.2)
    // Win: ascending arpeggio
    let notes = fixed_array(523.0, 659.0, 784.0, 1047.0)  // C5 E5 G5 C6
    let note_dur = 0.12
    let total_dur = note_dur * float(length(notes))
    let total_samples = int(float(AUDIO_RATE) * total_dur)
    snd_win |> resize(total_samples)
    for (ni in range(length(notes))) {
        let note <- gen_sine_sweep(notes[ni], notes[ni], note_dur, 0.25)
        let offset = ni * int(float(AUDIO_RATE) * note_dur)
        for (i in range(length(note))) {
            if (offset + i < total_samples) {
                snd_win[offset + i] += note[i]
            }
        }
    }
    // Game over: descending arpeggio
    let go_notes = fixed_array(523.0, 392.0, 330.0, 196.0)  // C5 G4 E4 G3
    let go_note_dur = 0.15
    let go_total_dur = go_note_dur * float(length(go_notes))
    let go_total_samples = int(float(AUDIO_RATE) * go_total_dur)
    snd_game_over |> resize(go_total_samples)
    for (ni in range(length(go_notes))) {
        let note <- gen_sine_sweep(go_notes[ni], go_notes[ni] * 0.9, go_note_dur, 0.3)
        let offset = ni * int(float(AUDIO_RATE) * go_note_dur)
        for (i in range(length(note))) {
            if (offset + i < go_total_samples) {
                snd_game_over[offset + i] += note[i]
            }
        }
    }
}

def play_sfx(samples : array<float>) {
    if (!audio_initialized) { return ; }
    var copy <- clone(samples)
    play_sound_from_pcm(AUDIO_RATE, AUDIO_CHANNELS, copy)
}

// --- Music (daStrudel) ---

var g_music_tracks : table<string; int>

def setup_menu_music() {
    // Title screen — mysterious, anticipatory, classic arcade title vibe
    g_music_tracks["drums"] = strudel_add_track(s("<bd ~ [~ hh] ~> <bd ~ [~ hh] [hh?0.3 ~]> <bd ~ [~ hh] ~> <bd ~ [~ cp?0.4] [hh?0.3 ~]>"), 0.45)
    g_music_tracks["bass"] = strudel_add_track(note_pattern("<[a2 a2] ~ [e2 e2] ~> <[a2 a2] ~ [f2 f2] ~> <[a2 a2] ~ [g2 g2] ~> <[a2 a2] ~ [e2 e2] [f2 e2]>", "sine"), 0.35)
    g_music_tracks["lead"] = strudel_add_track(note_pattern("<a4 c5 e5 c5> <a4 b4 c5 e5> <f5 e5 c5 a4> <e5?0.7 d5 c5 b4>", "triangle"), 0.2)
    g_music_tracks["arp"] = strudel_add_track(note_pattern("<[a4 c5 e5 a5]> <[a4 c5 e5 c5]> <[f4 a4 c5 f5?0.5]> <[e4 g4 b4 e5?0.5]>", "triangle"), 0.1)
}

def strudel_music_cmd(cmd : string) {
    let parts <- split(cmd, ":")
    if (length(parts) < 2) {
        return
    }
    let action = parts[0]
    let name = parts[1]
    if (action == "play" && length(parts) >= 3) {
        let gain = length(parts) >= 4 ? float(to_double(parts[3])) : 1.0
        if (key_exists(g_music_tracks, name)) {
            strudel_remove_track(g_music_tracks[name])
        }
        g_music_tracks[name] = strudel_add_track(s(parts[2]), gain)
    } elif (action == "note" && length(parts) >= 4) {
        let synth = parts[3]
        let gain = length(parts) >= 5 ? float(to_double(parts[4])) : 0.4
        if (key_exists(g_music_tracks, name)) {
            strudel_remove_track(g_music_tracks[name])
        }
        g_music_tracks[name] = strudel_add_track(note_pattern(parts[2], synth), gain)
    } elif (action == "stop") {
        if (key_exists(g_music_tracks, name)) {
            let fade_time = length(parts) >= 3 ? float(to_double(parts[2])) : 0.5
            strudel_fade_track(g_music_tracks[name], 0.0, fade_time)
        }
    } elif (action == "fade" && length(parts) >= 4) {
        if (key_exists(g_music_tracks, name)) {
            let gain = float(to_double(parts[2]))
            let time = float(to_double(parts[3]))
            strudel_fade_track(g_music_tracks[name], gain, time)
        }
    }
}

def strudel_music_main() {
    strudel_set_bpm(118.0lf)
    setup_menu_music()
    strudel_play(@@strudel_music_cmd)
}

def stop_all_tracks(fade : float) {
    strudel_command("stop:drums:{fade}")
    strudel_command("stop:bass:{fade}")
    strudel_command("stop:lead:{fade}")
    strudel_command("stop:arp:{fade}")
}

def start_menu_music() {
    strudel_set_bpm(118.0lf)
    strudel_command("play:drums:<bd ~ [~ hh] ~> <bd ~ [~ hh] [hh?0.3 ~]> <bd ~ [~ hh] ~> <bd ~ [~ cp?0.4] [hh?0.3 ~]>:0.45")
    strudel_command("note:bass:<[a2 a2] ~ [e2 e2] ~> <[a2 a2] ~ [f2 f2] ~> <[a2 a2] ~ [g2 g2] ~> <[a2 a2] ~ [e2 e2] [f2 e2]>:sine:0.35")
    strudel_command("note:lead:<a4 c5 e5 c5> <a4 b4 c5 e5> <f5 e5 c5 a4> <e5?0.7 d5 c5 b4>:triangle:0.2")
    strudel_command("note:arp:<[a4 c5 e5 a5]> <[a4 c5 e5 c5]> <[f4 a4 c5 f5?0.5]> <[e4 g4 b4 e5?0.5]>:triangle:0.1")
}

def start_play_music() {
    // Stage theme — driving, energetic, classic arcade action in A minor
    strudel_set_bpm(142.0lf)
    strudel_command("play:drums:<bd [hh hh] sd [hh hh]> <bd [hh hh] sd [hh cp]> <bd [hh hh] sd [hh hh]> <bd [hh hh] [sd sd] [hh hh]> <bd [hh hh] sd [hh hh]> <bd [hh cp?0.5] sd [hh hh]> <bd [hh hh] sd [hh?0.6 hh]> <bd [hh hh] [sd?0.4 sd] [cp?0.5 hh]>:0.6")
    strudel_command("note:bass:<[a2 a2] [a2 a2] [f2 f2] [g2 g2]> <[a2 a2] [a2 g2] [f2 f2] [e2 e2]> <[a2 a2] [a2 a2] [f2 f2] [g2 g2]> <[a2 a2] [c3 c3] [d3 d3] [e3 e3]> <[a2 a2] [a2 a2] [f2 f2] [g2 g2]> <[a2 a2] [g2 f2] [e2 e2] [e2 f2]> <[a2 a2] [a2 a2] [d3 d3] [c3 c3]> <[a2 a2] [f2?0.7 f2] [g2 g2] [a2 b2]>:square:0.3")
    strudel_command("note:lead:<a4 c5 e5 a5> <g5 e5 c5 ~> <a4 c5 e5 a5> <f5 e5 d5 c5> <a4 ~ [c5 d5] e5> <g5 e5 c5 a4> <[a4 b4] c5 e5 ~> <b4?0.7 c5 [d5 e5?0.8] a5>:sawtooth:0.25")
    strudel_command("note:arp:<[a4 c5 e5 a5 c6 a5 e5 c5]> <[e5 a5 c6 e6 c6 a5 e5 a4]> <[a4 c5 e5 a5?0.6 c6?0.6 a5 e5 c5]> <[f4 a4 c5 f5 c5 a4 f4 a4]> <[a4 c5 e5 a5 c6 a5 e5 c5]> <[e4 g4 b4 e5 b4 g4?0.7 e4 g4]> <[a4 e5?0.6 c5 a5?0.6 e5 c5?0.6 a4 e5]> <[f4 a4 c5 f5?0.5 c5 a4?0.7 f4 a4]>:triangle:0.12")
}

def start_win_music() {
    // Victory — triumphant C major fanfare, fast arpeggios
    strudel_set_bpm(152.0lf)
    strudel_command("play:drums:<bd sd bd [sd sd]> <bd [sd cp] bd sd> <bd sd [bd bd] [sd cp]> <bd [cp cp] bd [sd?0.7 sd?0.7]>:0.6")
    strudel_command("note:bass:<[c3 c3] [e3 e3] [g3 g3] [c4 c4]> <[c3 c3] [d3 d3] [e3 e3] [g3 g3]> <[e3 e3] [g3 g3] [c4 c4] [e4 e4]> <[c4 c4] [g3 g3] [e3 e3] [c3 c3]>:square:0.3")
    strudel_command("note:lead:<c5 e5 g5 c6> <e5 g5 c6 e6> <g5 e5 c6 g5> <c6 b5 a5 g5>:sawtooth:0.3")
    strudel_command("note:arp:<[c5 e5 g5 c6 e6 c6 g5 e5]> <[e5 g5 c6 e6 g6?0.7 e6 c6 g5]> <[g5 c6 e6 g6 e6 c6 g5 c6]> <[c6 g5 e5 c5 e5 g5 c6?0.8 e6?0.8]>:triangle:0.18")
}

def start_gameover_music() {
    strudel_set_bpm(85.0lf)
    strudel_command("play:drums:<bd ~ ~ ~> <bd ~ [~ hh?0.3] ~> <bd ~ ~ [~ bd?0.3]> <bd ~ ~ ~>:0.25")
    strudel_command("note:bass:<c3 bb2 ab2 g2> <c3 ~ ab2 ~> <bb2 ab2 g2 f2> <g2 ~ ab2 ~>:sine:0.3")
    strudel_command("note:lead:<eb4 ~ d4 ~> <~ d4 ~ c4> <eb4 d4?0.5 c4 ~> <~ ~ d4?0.5 ~>:triangle:0.2")
}

def update_music() {
    if (game_state == prev_music_state) {
        return
    }
    let FADE_OUT = 0.8
    let FADE_IN = 0.5
    // Pause: fade down existing tracks, keep them alive
    if (game_state == GameState.paused) {
        strudel_command("fade:drums:0.15:{FADE_OUT}")
        strudel_command("fade:bass:0.15:{FADE_OUT}")
        strudel_command("fade:lead:0.1:{FADE_OUT}")
        strudel_command("fade:arp:0.0:{FADE_OUT}")
        prev_music_state = game_state
        return
    }
    // Unpause: fade tracks back up
    if (prev_music_state == GameState.paused && game_state == GameState.playing) {
        strudel_command("fade:drums:0.6:{FADE_IN}")
        strudel_command("fade:bass:0.3:{FADE_IN}")
        strudel_command("fade:lead:0.25:{FADE_IN}")
        strudel_command("fade:arp:0.15:{FADE_IN}")
        prev_music_state = game_state
        return
    }
    // Full transition: fade out everything, start new music
    stop_all_tracks(FADE_OUT)
    if (game_state == GameState.menu) {
        start_menu_music()
    } elif (game_state == GameState.playing) {
        start_play_music()
    } elif (game_state == GameState.win_state) {
        start_win_music()
    } elif (game_state == GameState.game_over_state) {
        start_gameover_music()
    }
    prev_music_state = game_state
}

// --- Shaders ---

var @in @location = 0 v_position : float3
var @in @location = 1 v_normal   : float3
var @in @location = 2 v_texcoord : float2
var @uniform v_model : float4x4
var @uniform v_view : float4x4
var @uniform v_projection : float4x4
var @inout f_normal : float3
var @inout f_position : float3
var @uniform f_Color : float4
var @out f_FragColor : float4

[vertex_program]
def vs_arcanoid {
    f_normal = normalize(float3x3(v_model) * v_normal)
    let tpos = v_model * float4(v_position, 1.0)
    f_position = tpos.xyz
    gl_Position = v_projection * v_view * tpos
}

[fragment_program]
def fs_arcanoid {
    let normal = normalize(f_normal)
    let k = saturate(-dot(LIGHT_DIRECTION, normal))
    f_FragColor.xyz = f_Color.xyz * (k * 0.8 + 0.2)
    f_FragColor.w = f_Color.w
}

let SHADOW_COLOR = float3(0.05, 0.03, 0.08)

[fragment_program]
def fs_shadow {
    f_FragColor = float4(SHADOW_COLOR, 0.5)
}

// --- Rounded box geometry ---

def gen_rounded_cube(bevel : float; sectors : int = 16; stacks : int = 8) {
    // Use sphere topology (known-good CCW winding) mapped onto rounded box surface
    // Each sphere vertex direction is projected onto the box, then offset outward by bevel
    var frag : GeometryFragment
    frag.vertices |> reserve((stacks + 1) * (sectors + 1))
    frag.indices |> reserve(stacks * sectors * 6)

    let b = clamp(bevel, 0.01, 0.49)
    let inner = 1.0 - b
    let sectorStep = 2.0 * PI / float(sectors)
    let stackStep = PI / float(stacks)
    for (i in range(stacks + 1)) {
        let stackAngle = PI / 2.0 - float(i) * stackStep
        let xy = cos(stackAngle)
        let z = sin(stackAngle)
        for (j in range(sectors + 1)) {
            let sectorAngle = float(j) * sectorStep
            let x = xy * cos(sectorAngle)
            let y = xy * sin(sectorAngle)
            let n = normalize(float3(x, z, y))
            // Clamp to inner box, then push out by bevel along normal
            let clamped = float3(
                clamp(n.x * 100.0, -inner, inner),
                clamp(n.y * 100.0, -inner, inner),
                clamp(n.z * 100.0, -inner, inner)
            )
            let pos = clamped + n * b
            frag.vertices |> push(GeometryPreviewVertex(
                xyz = pos, normal = n,
                uv = float2(float(j) / float(sectors), float(i) / float(stacks))
            ))
        }
    }
    // Same index generation as gen_sphere
    for (i in range(stacks)) {
        let k1 = i * (sectors + 1)
        let k2 = k1 + sectors + 1
        for (j in range(sectors)) {
            if (i != 0) {
                frag.indices |> push(k1 + j)
                frag.indices |> push(k1 + j + 1)
                frag.indices |> push(k2 + j)
            }
            if (i != stacks - 1) {
                frag.indices |> push(k1 + j + 1)
                frag.indices |> push(k2 + j + 1)
                frag.indices |> push(k2 + j)
            }
        }
    }
    frag.prim = GeometryFragmentType.triangles
    gen_bbox(frag)
    return <- frag
}

// --- GL objects ---

var program : uint
var shadow_program : uint
var cube : OpenGLGeometryFragment
var brick_geo : OpenGLGeometryFragment
var ground_plane : OpenGLGeometryFragment
var sphere : OpenGLGeometryFragment
var hud_font : Font?

def create_gl_objects() {
    program = cache_shader_program(vs_arcanoid`shader_text, fs_arcanoid`shader_text)
    shadow_program = cache_shader_program(vs_arcanoid`shader_text, fs_shadow`shader_text)
    cube <- create_geometry_fragment <| gen_cube()
    brick_geo <- create_geometry_fragment <| gen_rounded_cube(0.2)
    ground_plane <- create_geometry_fragment <| gen_plane(GenDirection.xz)
    sphere <- create_geometry_fragment <| gen_sphere(16, 8, false)
    cache_ttf_objects()
    hud_font = cache_font("{get_das_root()}/modules/dasStbImage/fonts/droidsansmono.ttf")
}

def delete_gl_objects() {
    // GL objects are re-created each init; old ones leak but are cheap
}

// --- Drawing helpers ---

var display_w = 0
var display_h = 0
var active_program : uint

def draw_box(pos : float3; dim : float3; color : float3; alpha : float = 1.0) {
    v_model = compose(pos, float4(0), dim * 0.5)
    f_Color = float4(color, alpha)
    vs_arcanoid_bind_uniform(active_program)
    if (active_program == shadow_program) {
        fs_shadow_bind_uniform(active_program)
    } else {
        fs_arcanoid_bind_uniform(active_program)
    }
    cube |> draw_geometry_fragment()
}

def draw_sphere(pos : float3; radius : float; color : float3; alpha : float = 1.0) {
    v_model = compose(pos, float4(0), float3(radius))
    f_Color = float4(color, alpha)
    vs_arcanoid_bind_uniform(active_program)
    if (active_program == shadow_program) {
        fs_shadow_bind_uniform(active_program)
    } else {
        fs_arcanoid_bind_uniform(active_program)
    }
    sphere |> draw_geometry_fragment()
}

// --- Shadows (planar projection — same geometry re-drawn) ---

def draw_ground() {
    glDisable(GL_CULL_FACE)
    v_model = compose(float3(0.0, GROUND_Y, FIELD_H * 0.5), float4(0), float3(FIELD_W * 0.6, 1.0, FIELD_H * 0.6))
    f_Color = float4(GROUND_COLOR, 1.0)
    vs_arcanoid_bind_uniform(program)
    fs_arcanoid_bind_uniform(program)
    ground_plane |> draw_geometry_fragment()
    glEnable(GL_CULL_FACE)
}

// --- Walls ---

def draw_walls() {
    let hw = FIELD_W * 0.5
    // Left wall
    draw_box(
        float3(-hw - WALL_THICKNESS * 0.5, WALL_HEIGHT * 0.5, FIELD_H * 0.5),
        float3(WALL_THICKNESS, WALL_HEIGHT, FIELD_H + WALL_THICKNESS),
        WALL_COLOR
    )
    // Right wall
    draw_box(
        float3(hw + WALL_THICKNESS * 0.5, WALL_HEIGHT * 0.5, FIELD_H * 0.5),
        float3(WALL_THICKNESS, WALL_HEIGHT, FIELD_H + WALL_THICKNESS),
        WALL_COLOR
    )
    // Top wall
    draw_box(
        float3(0.0, WALL_HEIGHT * 0.5, FIELD_H + WALL_THICKNESS * 0.5),
        float3(FIELD_W + WALL_THICKNESS * 2.0, WALL_HEIGHT, WALL_THICKNESS),
        WALL_COLOR
    )
}

def draw_shield() {
    if (shield_timer > 0.0) {
        let alpha = min(shield_timer / 2.0, 1.0)  // fade out in last 2 seconds
        let pulse = 0.7 + 0.3 * sin(float(get_uptime()) * 6.0)
        let color = BONUS_COLORS[int(BonusType.shield)] * pulse
        draw_box(
            float3(0.0, WALL_HEIGHT * 0.25, SHIELD_Z),
            float3(FIELD_W, WALL_HEIGHT * 0.5, 0.15),
            color, alpha
        )
    }
}

// --- Game state ---

enum GameState {
    menu
    playing
    paused
    game_over_state
    win_state
}

var @live score = 0
var @live lives = START_LIVES
var @live game_state = GameState.menu
var space_was_pressed = false
var esc_was_pressed = false
var @live god_mode = false
var @live game_speed = 1.0
var @live prev_music_state = GameState.menu
var music_initialized = false


// --- Bonuses ---

enum BonusType {
    triple_ball
    wide_paddle
    narrow_paddle
    sticky_paddle
    fireball
    extra_life
    speed_up
    speed_down
    shield
}

let NUM_BONUS_TYPES = 9

let BONUS_COLORS = fixed_array(
    float3(1.0, 1.0, 1.0),   // triple_ball — white
    float3(0.2, 0.8, 1.0),   // wide_paddle — cyan
    float3(1.0, 0.3, 0.3),   // narrow_paddle — red
    float3(0.8, 0.8, 0.2),   // sticky_paddle — yellow
    float3(1.0, 0.5, 0.0),   // fireball — orange
    float3(0.3, 1.0, 0.3),   // extra_life — green
    float3(1.0, 0.2, 0.8),   // speed_up — pink
    float3(0.5, 0.3, 1.0),   // speed_down — purple
    float3(0.4, 0.9, 0.9)    // shield — teal
)

let SHIELD_Z = -0.5  // just below paddle

[decs_template]
struct Bonus {
    pos       : float3
    bonus_type : int  // BonusType as int (enums in DECS templates need int)
    color     : float3
}

var @live snd_bonus_pickup : array<float>
var @live snd_bonus_bad : array<float>

// Active powerup timers
var @live wide_paddle_timer = 0.0
var @live narrow_paddle_timer = 0.0
var @live fireball_timer = 0.0
var @live speed_mod_timer = 0.0
var @live speed_mod_factor = 1.0
var @live sticky_timer = 0.0
var @live shield_timer = 0.0
var @live display_paddle_w = PADDLE_W
var @live display_paddle_color = PADDLE_COLOR
var @live last_powerup_color = PADDLE_COLOR
var @live lives_blink_timer = 0.0
var rng_seed = 42u

def game_random() : float {
    rng_seed = rng_seed * 1103515245u + 12345u
    return float((rng_seed >> 16u) & 0x7FFFu) / 32767.0
}

def target_paddle_w() : float {
    if (wide_paddle_timer > 0.0) { return PADDLE_W * 1.6; }
    if (narrow_paddle_timer > 0.0) { return PADDLE_W * 0.6; }
    return PADDLE_W
}

def current_paddle_w() : float {
    return display_paddle_w
}

def current_ball_speed() : float {
    if (speed_mod_timer > 0.0) { return BALL_SPEED * speed_mod_factor; }
    return BALL_SPEED
}

def spawn_bonus(pos : float3) {
    if (game_random() > BONUS_DROP_CHANCE) { return ; }
    let btype = int(game_random() * float(NUM_BONUS_TYPES)) % NUM_BONUS_TYPES
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Bonus(
            pos = float3(pos.x, pos.y, pos.z),
            bonus_type = btype,
            color = BONUS_COLORS[btype]
        ))
    }
}

def activate_bonus(btype : int) {
    if (btype == int(BonusType.triple_ball)) {
        // Spawn 2 extra balls from existing ball positions
        var positions : array<float3>
        var velocities : array<float3>
        query <| $(b : Ball) {
            positions |> push(b.pos)
            velocities |> push(b.vel)
        }
        for (i in range(length(positions))) {
            let p = positions[i]
            let speed = length(velocities[i])
            let vel1 = normalize(float3(velocities[i].x + sin(0.5) * speed * 0.3, 0.0, velocities[i].z)) * speed
            let vel2 = normalize(float3(velocities[i].x + sin(-0.5) * speed * 0.3, 0.0, velocities[i].z)) * speed
            create_entity <| @(eid, cmp) {
                apply_decs_template(cmp, Ball(pos = p, vel = vel1))
            }
            create_entity <| @(eid, cmp) {
                apply_decs_template(cmp, Ball(pos = p, vel = vel2))
            }
        }
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.wide_paddle)) {
        wide_paddle_timer = POWERUP_DURATION
        narrow_paddle_timer = 0.0
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.narrow_paddle)) {
        narrow_paddle_timer = POWERUP_DURATION
        wide_paddle_timer = 0.0
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_bad)
    } elif (btype == int(BonusType.sticky_paddle)) {
        sticky_timer = POWERUP_DURATION
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.fireball)) {
        fireball_timer = POWERUP_DURATION
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.extra_life)) {
        lives++
        lives_blink_timer = 1.5
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.speed_up)) {
        speed_mod_timer = POWERUP_DURATION
        speed_mod_factor = 1.5
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_bad)
    } elif (btype == int(BonusType.speed_down)) {
        speed_mod_timer = POWERUP_DURATION
        speed_mod_factor = 0.6
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_pickup)
    } elif (btype == int(BonusType.shield)) {
        shield_timer = POWERUP_DURATION
        last_powerup_color = BONUS_COLORS[btype]
        play_sfx(snd_bonus_pickup)
    }
}

def update_powerup_timers(dt : float) {
    if (wide_paddle_timer > 0.0) { wide_paddle_timer -= dt; }
    if (narrow_paddle_timer > 0.0) { narrow_paddle_timer -= dt; }
    if (fireball_timer > 0.0) { fireball_timer -= dt; }
    if (speed_mod_timer > 0.0) { speed_mod_timer -= dt; }
    let was_sticky = sticky_timer > 0.0
    if (sticky_timer > 0.0) { sticky_timer -= dt; }
    // Auto-launch stuck balls when sticky expires
    if (was_sticky && sticky_timer <= 0.0) {
        query <| $(var b : Ball) {
            if (b.stuck != 0) {
                let offset = b.stuck_offset / (current_paddle_w() * 0.5)
                let angle = clamp(offset, -1.0, 1.0) * 0.7
                b.vel = normalize(float3(sin(angle), 0.0, cos(angle))) * current_ball_speed()
                b.stuck = 0
                b.stuck_offset = 0.0
            }
        }
    }
    if (shield_timer > 0.0) { shield_timer -= dt; }
    // Smooth paddle width transition
    let target = target_paddle_w()
    let lerp_speed = 8.0 * dt
    display_paddle_w += (target - display_paddle_w) * saturate(lerp_speed)
    // Smooth paddle color transition — last activated powerup wins
    let any_active = wide_paddle_timer > 0.0 || narrow_paddle_timer > 0.0 || sticky_timer > 0.0 || fireball_timer > 0.0 || speed_mod_timer > 0.0 || shield_timer > 0.0
    let target_color = any_active ? last_powerup_color : PADDLE_COLOR
    display_paddle_color += (target_color - display_paddle_color) * saturate(lerp_speed)
    // Lives blink timer
    if (lives_blink_timer > 0.0) { lives_blink_timer -= dt; }
}

def reset_powerups() {
    wide_paddle_timer = 0.0
    narrow_paddle_timer = 0.0
    fireball_timer = 0.0
    speed_mod_timer = 0.0
    speed_mod_factor = 1.0
    sticky_timer = 0.0
    shield_timer = 0.0
}

// --- Paddle state ---

var @live paddle_x = 0.0

def update_paddle(dt : float) {
    if (glfwGetKey(live_window, int(GLFW_KEY_LEFT)) == int(GLFW_PRESS)) {
        paddle_x += PADDLE_SPEED * dt
    }
    if (glfwGetKey(live_window, int(GLFW_KEY_RIGHT)) == int(GLFW_PRESS)) {
        paddle_x -= PADDLE_SPEED * dt
    }
    let pw = current_paddle_w()
    let limit = FIELD_W * 0.5 - pw * 0.5
    paddle_x = clamp(paddle_x, -limit, limit)
}

def draw_paddle() {
    let pw = current_paddle_w()
    draw_box(
        float3(paddle_x, PADDLE_H * 0.5, PADDLE_Z),
        float3(pw, PADDLE_H, PADDLE_D),
        display_paddle_color
    )
}

// --- Ball (DECS) ---

var tick_dt = 0.0

[decs_template]
struct Ball {
    pos : float3
    vel : float3
    stuck : int = 0         // 1 if stuck to paddle
    stuck_offset : float    // x-offset from paddle center when caught
}

def spawn_ball(px, vx : float) {
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Ball(
            pos = float3(px, BALL_RADIUS, BALL_START_Z),
            vel = normalize(float3(vx, 0.0, 1.0)) * BALL_SPEED
        ))
    }
}

def spawn_ball_on_paddle() {
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Ball(
            pos = float3(paddle_x, BALL_RADIUS, PADDLE_Z + PADDLE_D * 0.5 + BALL_RADIUS),
            vel = float3(0.0),
            stuck = 1,
            stuck_offset = 0.0
        ))
    }
}

def has_stuck_balls() : bool {
    var found = false
    query <| $(b : Ball) { if (b.stuck != 0) { found = true; } }
    return found
}

def has_balls() : bool {
    var found = false
    query <| $(b : Ball) {
        found = true
    }
    return found
}

[decs(stage=update)]
def update_balls(eid : EntityId; var b : Ball) {
    // Sticky: ball sits on paddle at caught offset
    if (b.stuck != 0) {
        b.pos.x = paddle_x + b.stuck_offset
        b.pos.z = PADDLE_Z + PADDLE_D * 0.5 + BALL_RADIUS
        b.vel = float3(0.0)
        return
    }
    // Normalize speed to current ball speed
    let spd = current_ball_speed()
    let cur_spd = length(b.vel)
    if (cur_spd > 0.0 && abs(cur_spd - spd) > 0.1) {
        b.vel = normalize(b.vel) * spd
    }
    b.pos += b.vel * tick_dt
    let hw = FIELD_W * 0.5 - BALL_RADIUS
    // Left wall
    if (b.pos.x > hw) {
        b.pos.x = hw
        b.vel.x = -b.vel.x
        play_sfx(snd_wall_bounce)
    }
    // Right wall
    if (b.pos.x < -hw) {
        b.pos.x = -hw
        b.vel.x = -b.vel.x
        play_sfx(snd_wall_bounce)
    }
    // Top wall
    if (b.pos.z > FIELD_H - BALL_RADIUS) {
        b.pos.z = FIELD_H - BALL_RADIUS
        b.vel.z = -b.vel.z
        play_sfx(snd_wall_bounce)
    }
    // Paddle collision
    let pw = current_paddle_w()
    let pz_min = PADDLE_Z - PADDLE_D * 0.5 - BALL_RADIUS
    let pz_max = PADDLE_Z + PADDLE_D * 0.5 + BALL_RADIUS
    let px_min = paddle_x - pw * 0.5 - BALL_RADIUS
    let px_max = paddle_x + pw * 0.5 + BALL_RADIUS
    if (b.vel.z < 0.0 && b.pos.z >= pz_min && b.pos.z <= pz_max && b.pos.x >= px_min && b.pos.x <= px_max) {
        b.pos.z = pz_max
        let offset = (b.pos.x - paddle_x) / (pw * 0.5)
        let angle = offset * 0.7
        b.vel = normalize(float3(sin(angle), 0.0, cos(angle))) * spd
        play_sfx(snd_paddle_hit)
        if (sticky_timer > 0.0) {
            b.stuck = 1
            b.stuck_offset = b.pos.x - paddle_x
        }
    }
    // Shield wall collision
    if (shield_timer > 0.0 && b.vel.z < 0.0 && b.pos.z < SHIELD_Z + BALL_RADIUS) {
        b.pos.z = SHIELD_Z + BALL_RADIUS
        b.vel.z = abs(b.vel.z)
        play_sfx(snd_wall_bounce)
    }
    // Ball fell below paddle
    if (b.pos.z < -1.0) {
        if (god_mode) {
            b.pos.z = PADDLE_Z + PADDLE_D * 0.5 + BALL_RADIUS
            b.vel.z = abs(b.vel.z)
        } else {
            delete_entity(eid)
            play_sfx(snd_ball_miss)
            // Life lost only tracked after commit — see update loop
        }
    }
}

[decs(stage=draw)]
def draw_balls(b : Ball) {
    var color = BALL_COLOR
    if (fireball_timer > 0.0) { color = float3(1.0, 0.4, 0.1); }
    draw_sphere(b.pos, BALL_RADIUS, color)
}

// --- Bricks (DECS) ---

[decs_template]
struct Brick {
    pos   : float3
    color : float3
    row   : int
}

def spawn_bricks() {
    let total_w = float(BRICK_COLS) * (BRICK_W + BRICK_GAP) - BRICK_GAP
    let x_offset = -total_w * 0.5 + BRICK_W * 0.5
    let total = BRICK_ROWS * BRICK_COLS
    create_entities`Brick(total) <| $(eid : EntityId; i : int; var b : Brick) {
        let row = i / BRICK_COLS
        let col = i % BRICK_COLS
        b.pos = float3(x_offset + float(col) * (BRICK_W + BRICK_GAP), BRICK_H * 0.5, BRICK_START_Z + float(row) * (BRICK_D + BRICK_GAP))
        b.color = BRICK_COLORS[row]
        b.row = row
    }
}

def has_bricks() : bool {
    var found = false
    query <| $(br : Brick) {
        found = true
    }
    return found
}

[decs(stage=draw)]
def draw_bricks(br : Brick) {
    v_model = compose(br.pos, float4(0), float3(BRICK_W, BRICK_H, BRICK_D) * 0.5)
    f_Color = float4(br.color, 1.0)
    vs_arcanoid_bind_uniform(active_program)
    if (active_program == shadow_program) {
        fs_shadow_bind_uniform(active_program)
    } else {
        fs_arcanoid_bind_uniform(active_program)
    }
    brick_geo |> draw_geometry_fragment()
}

// --- Bonus entities ---

[decs(stage=update)]
def update_bonuses(eid : EntityId; var bn : Bonus) {
    bn.pos.z -= BONUS_FALL_SPEED * tick_dt
    // Rotate Y for visual flair
    bn.pos.y = BONUS_SIZE * 0.5 + sin(float(get_uptime()) * 4.0) * 0.1
    // Check paddle collection
    let pw = current_paddle_w()
    let px_min = paddle_x - pw * 0.5 - BONUS_SIZE * 0.5
    let px_max = paddle_x + pw * 0.5 + BONUS_SIZE * 0.5
    let pz_min = PADDLE_Z - PADDLE_D * 0.5 - BONUS_SIZE * 0.5
    let pz_max = PADDLE_Z + PADDLE_D * 0.5 + BONUS_SIZE * 0.5
    if (bn.pos.z >= pz_min && bn.pos.z <= pz_max && bn.pos.x >= px_min && bn.pos.x <= px_max) {
        spawn_pickup_vfx(bn.pos, bn.color)
        activate_bonus(bn.bonus_type)
        delete_entity(eid)
        return
    }
    // Fell off screen
    if (bn.pos.z < -1.0) {
        delete_entity(eid)
    }
}

[decs(stage=draw)]
def draw_bonuses(bn : Bonus) {
    // Draw as small rotating cube
    let angle = float(get_uptime()) * 3.0
    v_model = compose(bn.pos, float4(0.0, sin(angle * 0.5), 0.0, cos(angle * 0.5)), float3(BONUS_SIZE * 0.5))
    f_Color = float4(bn.color, 1.0)
    vs_arcanoid_bind_uniform(program)
    fs_arcanoid_bind_uniform(program)
    cube |> draw_geometry_fragment()
}

// --- Particles ---

[decs_template]
struct Particle {
    pos      : float3
    vel      : float3
    color    : float3
    lifetime : float
    max_life : float
    radius   : float
}

def spawn_particles(origin : float3; color : float3; count : int = 8) {
    for (i in range(count)) {
        let angle = float(i) / float(count) * 2.0 * PI
        let elevation = game_random() * 0.5 + 0.2
        let speed = 2.0 + game_random() * 3.0
        let vel = float3(cos(angle) * speed, elevation * speed, sin(angle) * speed)
        let lt = 0.3 + game_random() * 0.4
        let r = 0.05 + game_random() * 0.08
        create_entity <| @(eid, cmp) {
            apply_decs_template(cmp, Particle(pos = origin, vel = vel, color = color,
                lifetime = lt, max_life = lt, radius = r))
        }
    }
}

// --- Pickup VFX ---

def spawn_pickup_vfx(origin : float3; color : float3) {
    // Scatter of tiny cubes
    let count = 10
    for (i in range(count)) {
        let angle = float(i) / float(count) * 2.0 * PI + game_random() * 0.5
        let speed = 1.5 + game_random() * 2.0
        let elevation = 1.0 + game_random() * 1.5
        let vel = float3(cos(angle) * speed, elevation, sin(angle) * speed)
        let c = lerp(color, float3(1.0), game_random() * 0.3)
        let lt = 0.25 + game_random() * 0.2
        let r = 0.03 + game_random() * 0.03
        create_entity <| @(eid, cmp) {
            apply_decs_template(cmp, Particle(pos = origin, vel = vel, color = c,
                lifetime = lt, max_life = lt, radius = r))
        }
    }
}

[decs(stage=update)]
def update_particles(eid : EntityId; var p : Particle) {
    p.lifetime -= tick_dt
    if (p.lifetime <= 0.0) {
        delete_entity(eid)
        return
    }
    p.pos += p.vel * tick_dt
    p.vel.y -= 6.0 * tick_dt  // gravity
}

[decs(stage=draw)]
def draw_particles(p : Particle) {
    let frac = p.lifetime / p.max_life
    let alpha = frac
    let r = p.radius * (0.5 + frac * 0.5)  // shrink as they fade
    draw_box(p.pos, float3(r), p.color, alpha)
}

// --- Ball trail ---

[decs_template]
struct Trail {
    pos      : float3
    lifetime : float
}

let TRAIL_INTERVAL = 0.02
var trail_timer = 0.0

def spawn_trail_dots() {
    trail_timer -= tick_dt
    if (trail_timer > 0.0) { return ; }
    trail_timer = TRAIL_INTERVAL
    var ball_positions : array<float3>
    query <| $(b : Ball) {
        ball_positions |> push(b.pos)
    }
    if (length(ball_positions) > 0) {
        create_entities`Trail(length(ball_positions)) <| $(eid : EntityId; i : int; var tr : Trail) {
            tr.pos = ball_positions[i]
            tr.lifetime = 0.15
        }
    }
}

[decs(stage=update)]
def update_trail(eid : EntityId; var t : Trail) {
    t.lifetime -= tick_dt
    if (t.lifetime <= 0.0) {
        delete_entity(eid)
    }
}

[decs(stage=draw)]
def draw_trail(t : Trail) {
    let alpha = t.lifetime / 0.15
    var color = BALL_COLOR
    if (fireball_timer > 0.0) { color = float3(1.0, 0.4, 0.1); }
    draw_sphere(t.pos, BALL_RADIUS * 0.5 * alpha, color, alpha * 0.5)
}

// --- Shadow drawing ---

def draw_shadow_scene() {
    // Re-draw all scene geometry projected onto ground plane
    // Paddle
    let pw = current_paddle_w()
    draw_box(float3(paddle_x, PADDLE_H * 0.5, PADDLE_Z), float3(pw, PADDLE_H, PADDLE_D), SHADOW_COLOR)
    // Walls
    let hw = FIELD_W * 0.5
    draw_box(float3(-hw - WALL_THICKNESS * 0.5, WALL_HEIGHT * 0.5, FIELD_H * 0.5),
        float3(WALL_THICKNESS, WALL_HEIGHT, FIELD_H + WALL_THICKNESS), SHADOW_COLOR)
    draw_box(float3(hw + WALL_THICKNESS * 0.5, WALL_HEIGHT * 0.5, FIELD_H * 0.5),
        float3(WALL_THICKNESS, WALL_HEIGHT, FIELD_H + WALL_THICKNESS), SHADOW_COLOR)
    draw_box(float3(0.0, WALL_HEIGHT * 0.5, FIELD_H + WALL_THICKNESS * 0.5),
        float3(FIELD_W + WALL_THICKNESS * 2.0, WALL_HEIGHT, WALL_THICKNESS), SHADOW_COLOR)
    // Balls
    query <| $(b : Ball) {
        draw_sphere(b.pos, BALL_RADIUS, SHADOW_COLOR)
    }
    // Bricks
    query <| $(br : Brick) {
        draw_box(br.pos, float3(BRICK_W, BRICK_H, BRICK_D), SHADOW_COLOR)
    }
    // Bonuses (with rotation matching the main draw)
    let angle = float(get_uptime()) * 3.0
    query <| $(bn : Bonus) {
        v_model = compose(bn.pos, float4(0.0, sin(angle * 0.5), 0.0, cos(angle * 0.5)), float3(BONUS_SIZE * 0.5))
        f_Color = float4(SHADOW_COLOR, 0.5)
        vs_arcanoid_bind_uniform(active_program)
        fs_shadow_bind_uniform(active_program)
        cube |> draw_geometry_fragment()
    }
}

def draw_shadows() {
    let tm_shadow = planar_shadow(
        float4(LIGHT_DIRECTION, 0.0),
        plane_from_point_normal(float3(0.0, GROUND_Y + 0.005, 0.0), float3(0.0, 1.0, 0.0))
    )
    let saved_view = v_view
    v_view = saved_view * tm_shadow
    glUseProgram(shadow_program)
    active_program = shadow_program
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glDisable(GL_DEPTH_TEST)
    draw_shadow_scene()
    glEnable(GL_DEPTH_TEST)
    glDisable(GL_BLEND)
    v_view = saved_view
    glUseProgram(program)
    active_program = program
}

// --- Live commands ---

struct CmdSpawnBallArgs {
    x : float = 0.0
    vx : float = 0.3
}

[live_command(description="Spawn a ball. Args: x, vx")]
def cmd_spawn_ball(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdSpawnBallArgs>)
    if (args.x == 0.0) { args.x = paddle_x; }
    spawn_ball(args.x, args.vx)
    return JV(args)
}

struct GameStatusResult {
    balls : int
    bricks : int
    score : int
    lives : int
    state : string
    god_mode : bool
    speed : float
    paddle_x : float
}

[live_command(description="Show score, lives, ball/brick counts")]
def cmd_game_status(input : JsonValue?) : JsonValue? {
    var result = GameStatusResult()
    query <| $(b : Ball) { result.balls++; }
    query <| $(br : Brick) { result.bricks++; }
    result.score = score
    result.lives = lives
    result.state = "{game_state}"
    result.god_mode = god_mode
    result.speed = game_speed
    result.paddle_x = paddle_x
    return JV(result)
}

[live_command(description="Spawn a new set of bricks")]
def cmd_spawn_bricks(input : JsonValue?) : JsonValue? {
    spawn_bricks()
    return JV(true)
}

struct CmdBonusTypeArgs {
    @rename = "type" _type : BonusType = BonusType.wide_paddle
}

[live_command(description="Spawn a bonus at paddle. Args: type")]
def cmd_spawn_bonus(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdBonusTypeArgs>)
    let btype = int(args._type)
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Bonus(
            pos = float3(paddle_x, 0.3, 5.0),
            bonus_type = btype,
            color = BONUS_COLORS[btype]
        ))
    }
    return JV(args)
}

[live_command(description="Dump DECS archetypes, entities, and queries")]
def cmd_decs_dump(input : JsonValue?) : JsonValue? {
    return JV(debug_dump_string())
}

[live_command(description="Drop a powerup above paddle. Args: type (triple_ball, wide_paddle, narrow_paddle, sticky_paddle, fireball, extra_life, speed_up, speed_down, shield)")]
def cmd_drop_powerup(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdBonusTypeArgs>)
    let btype = int(args._type)
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Bonus(
            pos = float3(paddle_x, 0.3, PADDLE_Z + 3.0),
            bonus_type = btype,
            color = BONUS_COLORS[btype]
        ))
    }
    return JV(args)
}

[live_command(description="Activate a powerup. Args: type (triple_ball, wide_paddle, narrow_paddle, sticky_paddle, fireball, extra_life, speed_up, speed_down, shield)")]
def cmd_powerup(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdBonusTypeArgs>)
    activate_bonus(int(args._type))
    return JV(args)
}

[live_command(description="Reset game to initial state")]
def cmd_reset_game(input : JsonValue?) : JsonValue? {
    decs::restart()
    spawn_bricks()
    commit()
    score = 0
    lives = START_LIVES
    paddle_x = 0.0
    game_state = GameState.playing
    reset_powerups()
    return JV(true)
}

struct CmdSetLivesArgs {
    lives : int = -1
}

[live_command(description="Set lives. Args: lives")]
def cmd_set_lives(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdSetLivesArgs>)
    if (args.lives >= 0) { lives = args.lives; }
    return JV(CmdSetLivesArgs(lives = lives))
}

[live_command(description="Toggle god mode (ball bounces back instead of falling)")]
def cmd_god_mode(input : JsonValue?) : JsonValue? {
    god_mode = !god_mode
    return JV(god_mode)
}

struct CmdSpeedArgs {
    speed : float = 1.0
}

[live_command(description="Set game speed multiplier. Args: speed (0.1=slow, 1=normal, 2=fast)")]
def cmd_slow_motion(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdSpeedArgs>)
    game_speed = args.speed
    return JV(args)
}

struct CmdMusicVolumeArgs {
    volume : float = 0.4
    time : float = 0.3
}

[live_command(description="Set music volume (0.0-1.0) with optional fade time in seconds")]
def cmd_music_volume(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdMusicVolumeArgs>)
    strudel_set_volume(args.volume, args.time)
    return JV(args)
}

// --- HUD ---

def draw_text(text : string; x, y : int; tint : float3 = float3(1.0)) {
    if (hud_font == null) { return ; }
    var quads <- (*hud_font) |> create_quads(text)
    (*hud_font) |> draw_quads_2d(quads, display_w, display_h, x, y, tint)
    delete quads
}

def draw_text_centered(text : string; cx, y : int; tint : float3 = float3(1.0)) {
    if (hud_font == null) { return ; }
    var quads <- (*hud_font) |> create_quads(text)
    let dim = quads_dim(quads)
    let tw = int(dim.vmax.x - dim.vmin.x)
    let x = cx - tw / 2
    (*hud_font) |> draw_quads_2d(quads, display_w, display_h, x, y, tint)
    delete quads
}

def pulse_tint(base : float3) : float3 {
    let pulse = 0.5 + 0.5 * sin(float(get_uptime()) * 4.5)
    return lerp(base * 0.86, base * 1.06, pulse)
}

def draw_hud() {
    let cx = display_w / 2
    let cy = display_h / 2
    if (game_state == GameState.menu) {
        draw_text_centered("ARCANOID", cx, cy - 60, float3(1.0, 0.8, 0.2))
        draw_text_centered("PRESS SPACE TO START", cx, cy + 10, pulse_tint(float3(0.7, 0.7, 0.7)))
        return
    }
    draw_text("SCORE: {score}", 10, 35, float3(1.0, 1.0, 1.0))
    var lives_color = float3(0.3, 0.8, 1.0)
    if (lives_blink_timer > 0.0) {
        let blink = sin(lives_blink_timer * 20.0) > 0.0 ? 1.0 : 0.3
        lives_color = float3(blink, 1.0, blink)
    }
    draw_text("LIVES: {lives}", 10, 70, lives_color)
    if (game_state == GameState.game_over_state) {
        draw_text_centered("GAME OVER", cx, cy - 20, float3(1.0, 0.2, 0.2))
        draw_text_centered("PRESS SPACE TO RESTART", cx, cy + 25, pulse_tint(float3(0.7, 0.7, 0.7)))
    } elif (game_state == GameState.win_state) {
        draw_text_centered("YOU WIN!", cx, cy - 20, float3(0.2, 1.0, 0.3))
        draw_text_centered("SCORE: {score}", cx, cy + 25, float3(1.0, 1.0, 0.5))
        draw_text_centered("PRESS SPACE TO RESTART", cx, cy + 65, pulse_tint(float3(0.7, 0.7, 0.7)))
    } elif (game_state == GameState.paused) {
        draw_text_centered("PAUSED", cx, cy - 20, float3(0.8, 0.8, 0.8))
        draw_text_centered("PRESS ESC TO RESUME", cx, cy + 25, pulse_tint(float3(0.5, 0.5, 0.5)))
    } elif (has_stuck_balls()) {
        draw_text_centered("PRESS SPACE TO LAUNCH", cx, cy + 25, pulse_tint(float3(0.5, 0.5, 0.5)))
    }
    if (god_mode) {
        draw_text("GOD", display_w - 80, 35, float3(1.0, 0.5, 0.0))
    }
}

// --- State preservation across reloads ---
// Game globals tagged with @live are auto-serialized by live_vars macro.
// Audio handles preserved by audio_live module.

// --- Lifecycle ---

[export]
def init() {
    live_create_window("Arcanoid", 1024, 768)
    create_gl_objects()
    if (!audio_initialized) {
        audio_system_create()
        audio_initialized = true
        init_audio_samples()
    }
    if (!music_initialized) {
        strudel_init(@@strudel_music_main)
        strudel_set_volume(0.4, 0.0)
        music_initialized = true
    }
    if (!is_reload()) {
        decs::restart()
        spawn_bricks()
        commit()
    }
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    live_get_framebuffer_size(display_w, display_h)
    glViewport(0, 0, display_w, display_h)
    glClearColor(0.05, 0.02, 0.1, 1.0)
    glClearDepth(1.0lf)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glEnable(GL_DEPTH_TEST)
    glEnable(GL_CULL_FACE)

    let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.0
    v_projection = perspective_rh_opengl(45.0 * PI / 180.0, aspect, 0.1, 100.0)
    v_view = look_at_rh(
        float3(0.0, 14.0, -4.0),
        float3(0.0, 0.0, 5.0),
        float3(0.0, 1.0, 0.0)
    )

    let dt = get_dt()
    tick_dt = min(dt, 1.0 / 30.0) * game_speed
    let space_pressed = glfwGetKey(live_window, int(GLFW_KEY_SPACE)) == int(GLFW_PRESS)
    let space_just = space_pressed && !space_was_pressed
    space_was_pressed = space_pressed
    let esc_pressed = glfwGetKey(live_window, int(GLFW_KEY_ESCAPE)) == int(GLFW_PRESS)
    let esc_just = esc_pressed && !esc_was_pressed
    esc_was_pressed = esc_pressed
    // Menu → start
    if (game_state == GameState.menu && space_just) {
        game_state = GameState.playing
    }
    // Pause toggle
    if (game_state == GameState.playing && esc_just) {
        game_state = GameState.paused
    } elif (game_state == GameState.paused && (esc_just || space_just)) {
        game_state = GameState.playing
    }
    // Restart from game over / win
    if ((game_state == GameState.game_over_state || game_state == GameState.win_state) && space_just) {
        decs::restart()
        spawn_bricks()
        commit()
        score = 0
        lives = START_LIVES
        paddle_x = 0.0
        game_state = GameState.playing
        reset_powerups()
    }
    update_music()
    if (game_state == GameState.playing) {
        update_paddle(dt)
        update_powerup_timers(tick_dt)
        // Sticky release — launch all stuck balls
        if (space_just) {
            query <| $(var b : Ball) {
                if (b.stuck != 0) {
                    var offset = b.stuck_offset / (current_paddle_w() * 0.5)
                    if (abs(offset) < 0.1) { offset = 0.3; }  // avoid straight-up launch
                    let angle = offset * 0.7
                    b.vel = normalize(float3(sin(angle), 0.0, cos(angle))) * current_ball_speed()
                    b.stuck = 0
                    b.stuck_offset = 0.0
                }
            }
        }
        var any_stuck = false
        query <| $(b : Ball) { if (b.stuck != 0) { any_stuck = true; } }
        if (!has_balls() && !any_stuck) {
            spawn_ball_on_paddle()
            commit()
        }
        spawn_trail_dots()
        decs_stage("update")
        commit()
        // Check if all balls are gone after update
        if (!has_balls() && game_state == GameState.playing) {
            lives--
            if (lives <= 0) {
                game_state = GameState.game_over_state
                play_sfx(snd_game_over)
            } else {
                spawn_ball_on_paddle()
                commit()
            }
        }
        // Ball-brick collision (one per frame)
        var hit_eid : EntityId
        var hit_pos : float3
        var hit_ball_idx = -1
        var hit_side = false
        var hit_row = 0
        var ball_idx = 0
        let is_fireball = fireball_timer > 0.0
        query <| $(var b : Ball) {
            if (hit_ball_idx < 0 || is_fireball) {
                find_query <| $(eid : EntityId; br : Brick) {
                    let dx = abs(b.pos.x - br.pos.x) - (BRICK_W * 0.5 + BALL_RADIUS)
                    let dz = abs(b.pos.z - br.pos.z) - (BRICK_D * 0.5 + BALL_RADIUS)
                    if (dx < 0.0 && dz < 0.0) {
                        hit_eid = eid
                        hit_pos = br.pos
                        hit_ball_idx = ball_idx
                        hit_side = dx > dz
                        hit_row = br.row
                        if (!is_fireball) {
                            if (hit_side) {
                                b.vel.x = -b.vel.x
                            } else {
                                b.vel.z = -b.vel.z
                            }
                        }
                        return true
                    }
                    return false
                }
            }
            ball_idx++
        }
        if (hit_ball_idx >= 0) {
            delete_entity(hit_eid)
            commit()
            score += BRICK_POINTS[hit_row]
            play_sfx(snd_brick_break)
            spawn_bonus(hit_pos)
            spawn_particles(hit_pos, BRICK_COLORS[hit_row])
            commit()
            if (!has_bricks()) {
                game_state = GameState.win_state
                play_sfx(snd_win)
            }
        }
    }

    glUseProgram(program)
    active_program = program
    draw_ground()
    draw_shadows()
    draw_walls()
    draw_shield()
    draw_paddle()
    // Particles and trail need blending
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    decs_stage("draw")
    glDisable(GL_BLEND)

    glDisable(GL_CULL_FACE)
    draw_hud()
    live_end_frame()
}

[export]
def shutdown() {
    strudel_shutdown()
    delete_gl_objects()
    live_destroy_window()
}

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
