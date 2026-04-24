options gen2
options indenting = 4
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require strudel/strudel_midi
require strudel/strudel_midi_player
require strudel/strudel_synth
require math

// Helper: build MIDI bytes from uint values
def B(var arr : array<uint8>; b0 : uint) {
    arr |> push(uint8(b0))
}
def B(var arr : array<uint8>; b0, b1 : uint) {
    arr |> push(uint8(b0)); arr |> push(uint8(b1))
}
def B(var arr : array<uint8>; b0, b1, b2 : uint) {
    arr |> push(uint8(b0)); arr |> push(uint8(b1)); arr |> push(uint8(b2))
}
def B(var arr : array<uint8>; b0, b1, b2, b3 : uint) {
    arr |> push(uint8(b0)); arr |> push(uint8(b1)); arr |> push(uint8(b2)); arr |> push(uint8(b3))
}

def make_simple_midi() : array<uint8> {
    var d : array<uint8>
    // header: format 0, 1 track, 480 tpqn
    B(d, 0x4Du, 0x54u, 0x68u, 0x64u); B(d, 0u, 0u, 0u, 6u)
    B(d, 0u, 0u); B(d, 0u, 1u); B(d, 0x01u, 0xE0u)
    // track
    B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
    B(d, 0u, 0u, 0u, 11u)
    B(d, 0x00u, 0x90u, 0x3Cu, 0x64u)  // note on C4 vel=100
    B(d, 0x83u, 0x60u)                // delta=480
    B(d, 0x80u, 0x3Cu, 0x00u)         // note off
    B(d, 0x00u, 0xFFu, 0x2Fu, 0x00u)  // end of track
    return <- d
}

// ─── GM Presets ───

[test]
def test_gm_presets(tt : T?) {
    tt |> run("all programs valid") <| @(tt : T?) {
        for (i in range(128)) {
            let p = gm_preset(i)
            tt |> success(p.osc != OscType.osc_unknown, "program {i} valid osc")
        }
    }
    tt |> run("piano is triangle") <| @(tt : T?) {
        tt |> equal(gm_preset(0).osc, OscType.osc_triangle)
    }
    tt |> run("strings have slow attack") <| @(tt : T?) {
        tt |> success(gm_preset(40).att >= 0.05)
    }
    tt |> run("pipe is sine") <| @(tt : T?) {
        tt |> equal(gm_preset(73).osc, OscType.osc_sine)
    }
}

// ─── MIDI ADSR ───

[test]
def test_midi_adsr_fn(tt : T?) {
    let att = 0.01
    let dec = 0.05
    let sus = 0.8
    let rel = 0.1
    tt |> run("attack ramps up") <| @(tt : T?) {
        let v = midi_adsr(0.005, -1.0, att, dec, sus, rel)
        tt |> success(v > 0.0 && v < 1.0)
    }
    tt |> run("sustain hold with natural decay") <| @(tt : T?) {
        let v = midi_adsr(1.0, -1.0, att, dec, sus, rel)
        tt |> success(v > 0.0 && v <= sus, "decaying but positive")
    }
    tt |> run("release decays") <| @(tt : T?) {
        let v = midi_adsr(1.0, 0.05, att, dec, sus, rel)
        tt |> success(v > 0.0 && v < sus)
    }
    tt |> run("finished") <| @(tt : T?) {
        let v = midi_adsr(1.0, rel + 0.01, att, dec, sus, rel)
        tt |> success(abs(v) < 0.001)
    }
}

// ─── Drum Rendering ───

[test]
def test_drums(tt : T?) {
    tt |> run("kick non-empty") <| @(tt : T?) {
        let s <- render_drum_hit(36, 1.0)
        tt |> success(length(s) > 0)
    }
    tt |> run("snare non-empty") <| @(tt : T?) {
        let s <- render_drum_hit(38, 1.0)
        tt |> success(length(s) > 0)
    }
    tt |> run("hihat non-empty") <| @(tt : T?) {
        let s <- render_drum_hit(42, 1.0)
        tt |> success(length(s) > 0)
    }
    tt |> run("unknown drum fallback") <| @(tt : T?) {
        let s <- render_drum_hit(99, 1.0)
        tt |> success(length(s) > 0)
    }
}

// ─── Playback Tick ───

[test]
def test_playback(tt : T?) {
    tt |> run("non-silent output") <| @(tt : T?) {
        let d <- make_simple_midi()
        var midi <- parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        var has_sound = false
        for (_ in range(20)) {
            var chunk <- midi_tick(state, 0.05)
            for (s in chunk) {
                if (abs(s) > 0.001) {
                    has_sound = true
                }
            }
        }
        tt |> success(has_sound, "produced audio")
    }
    tt |> run("done after all notes") <| @(tt : T?) {
        var d : array<uint8>
        B(d, 0x4Du, 0x54u, 0x68u, 0x64u); B(d, 0u, 0u, 0u, 6u)
        B(d, 0u, 0u); B(d, 0u, 1u); B(d, 0x01u, 0xE0u)
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 14u)
        B(d, 0x00u, 0x90u, 0x3Cu, 0x64u)  // note on
        B(d, 0x81u, 0x70u)                // delta=240 (~0.25s at 480tpqn 120BPM)
        B(d, 0x80u, 0x3Cu, 0x00u)         // note off
        B(d, 0x00u, 0xFFu, 0x2Fu, 0x00u)  // end of track
        var midi <- parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        for (_ in range(100)) {
            midi_tick(state, 0.05)
        }
        // debug: check why done might not be set
        tt |> success(state.done, "playback finished")
    }
}

// ─── Event Processing ───

[test]
def test_events(tt : T?) {
    tt |> run("note on creates voice") <| @(tt : T?) {
        let d <- make_simple_midi()
        var midi <- parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        midi_tick(state, 0.05)
        tt |> success(length(state.voices) > 0, "voice created")
    }
    tt |> run("program change updates channel") <| @(tt : T?) {
        var d : array<uint8>
        B(d, 0x4Du, 0x54u, 0x68u, 0x64u); B(d, 0u, 0u, 0u, 6u)
        B(d, 0u, 0u); B(d, 0u, 1u); B(d, 0x01u, 0xE0u)
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 3u)
        B(d, 0x00u, 0xC0u, 0x28u)  // program change ch=0 prog=40
        var midi <- parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        midi_tick(state, 0.05)
        tt |> equal(state.channels[0].program, 40)
    }
}
