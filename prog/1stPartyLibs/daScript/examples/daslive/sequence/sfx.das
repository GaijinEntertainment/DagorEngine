options gen2

module sfx public

require audio/audio_boost
require live/audio_live
require live/live_vars
require live/live_commands
require live_host
require daslib/json_boost
require math

// --- Constants ---

let AUDIO_RATE = 48000
let AUDIO_CHANNELS = 1

// --- Generators ---

def gen_sine_sweep(freq_start, freq_end : float; duration : float; volume : float = 0.3) : array<float> {
    let num_samples = int(float(AUDIO_RATE) * duration)
    var samples : array<float>
    samples |> resize(num_samples)
    for (i in range(num_samples)) {
        let t = float(i) / float(AUDIO_RATE)
        let frac = t / duration
        let freq = freq_start + (freq_end - freq_start) * frac
        let envelope = 1.0 - frac
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

def gen_arpeggio(notes : array<float>; note_dur : float; volume : float = 0.25) : array<float> {
    let total_dur = note_dur * float(length(notes))
    let total_samples = int(float(AUDIO_RATE) * total_dur)
    var samples : array<float>
    samples |> resize(total_samples)
    for (ni in range(length(notes))) {
        var note <- gen_sine_sweep(notes[ni], notes[ni], note_dur, volume)
        let offset = ni * int(float(AUDIO_RATE) * note_dur)
        for (i in range(length(note))) {
            if (offset + i < total_samples) {
                samples[offset + i] += note[i]
            }
        }
    }
    return <- samples
}

// --- Sample buffers ---

var @live snd_chip_place : array<float>
var @live snd_chip_remove : array<float>
var @live snd_card_play : array<float>
var @live snd_card_draw : array<float>
var @live snd_sequence : array<float>
var @live snd_game_win : array<float>
var @live snd_turn_chime : array<float>
var @live audio_initialized = false
var @live sfx_volume = 0.5

// --- Init / Play ---

def init_audio() {
    if (!audio_initialized) {
        audio_system_create()
        audio_initialized = true
    }
    // Card play: short click/snap
    snd_card_play <- gen_sine_sweep(1200.0, 600.0, 0.03, 0.2)
    // Chip place: soft tap
    snd_chip_place <- gen_noise_burst(0.03, 0.05)
    var thud <- gen_sine_sweep(150.0, 80.0, 0.04, 0.08)
    mix_into(snd_chip_place, thud)
    // Chip remove (black jack): descending tone
    snd_chip_remove <- gen_sine_sweep(600.0, 150.0, 0.15, 0.2)
    // Card draw: swish (noise burst, short)
    snd_card_draw <- gen_noise_burst(0.04, 0.05)
    var swish <- gen_sine_sweep(800.0, 2000.0, 0.04, 0.04)
    mix_into(snd_card_draw, swish)
    // Sequence complete: ascending arpeggio C5-E5-G5-C6
    var seq_notes <- [523.0, 659.0, 784.0, 1047.0]
    snd_sequence <- gen_arpeggio(seq_notes, 0.1, 0.25)
    // Game win: longer fanfare C5-E5-G5-C6-E6
    var win_notes <- [523.0, 659.0, 784.0, 1047.0, 1319.0]
    snd_game_win <- gen_arpeggio(win_notes, 0.15, 0.3)
    // Turn chime: gentle single note
    snd_turn_chime <- gen_sine_sweep(880.0, 880.0, 0.06, 0.1)
}

def play_sfx(samples : array<float>) {
    if (!audio_initialized || length(samples) == 0) {
        return
    }
    var copy <- clone(samples)
    for (i in range(length(copy))) {
        copy[i] *= sfx_volume
    }
    play_sound_from_pcm(AUDIO_RATE, AUDIO_CHANNELS, copy)
}

// Convenience wrappers
def sfx_card_play() {
    play_sfx(snd_card_play)
}

def sfx_chip_place() {
    play_sfx(snd_chip_place)
}

def sfx_chip_remove() {
    play_sfx(snd_chip_remove)
}

def sfx_card_draw() {
    play_sfx(snd_card_draw)
}

def sfx_sequence_complete() {
    play_sfx(snd_sequence)
}

def sfx_game_win() {
    play_sfx(snd_game_win)
}

def sfx_turn_chime() {
    play_sfx(snd_turn_chime)
}

// --- Live command ---

struct PlaySoundArgs {
    name : string
}

[live_command(description="Play a sound effect. Args: name (card_play/chip_place/chip_remove/card_draw/sequence/game_win/turn_chime)")]
def cmd_play_sound(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<PlaySoundArgs>)
    if (args.name == "card_play") {
        sfx_card_play()
    } elif (args.name == "chip_place") {
        sfx_chip_place()
    } elif (args.name == "chip_remove") {
        sfx_chip_remove()
    } elif (args.name == "card_draw") {
        sfx_card_draw()
    } elif (args.name == "sequence") {
        sfx_sequence_complete()
    } elif (args.name == "game_win") {
        sfx_game_win()
    } elif (args.name == "turn_chime") {
        sfx_turn_chime()
    } else {
        return JV("unknown sound: {args.name}")
    }
    return JV("played: {args.name}")
}
