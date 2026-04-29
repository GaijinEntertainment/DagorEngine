options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require strudel/strudel_midi
require strudel/strudel_midi_player
require strudel/strudel_synth
require dastest/testing_boost
require math

// Helper: build MIDI bytes
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

def push_midi_header(var d : array<uint8>; fmt, nTracks, tpqn : uint) {
    B(d, 0x4Du, 0x54u, 0x68u, 0x64u)  // "MThd"
    B(d, 0u, 0u, 0u, 6u)
    B(d, 0u, fmt)
    B(d, 0u, nTracks)
    B(d, uint(tpqn >> 8u), tpqn & 0xFFu)
}

def push_track_header(var d : array<uint8>; size : uint) {
    B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)  // "MTrk"
    B(d, uint(size >> 24u) & 0xFFu, uint(size >> 16u) & 0xFFu, uint(size >> 8u) & 0xFFu, size & 0xFFu)
}

// ─── Control Change Tests ───

[test]
def test_control_changes(t : T?) {
    t |> run("CC7 sets channel volume") <| @(t : T?) {
        // build MIDI: CC7 on channel 0, value 80
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        B(d, 0u, 0xB0u, 7u, 80u)     // delta=0, CC ch0, controller 7, value 80
        B(d, 0u, 0xFFu, 0x2Fu, 0u)   // end of track
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(abs(state.channels[0].volume - float(80) / 127.0) < 0.01)
    }
    t |> run("CC10 sets channel pan") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        B(d, 0u, 0xB0u, 10u, 0u)     // pan hard left (0)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(state.channels[0].pan < -0.9)
    }
    t |> run("CC11 sets expression") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        B(d, 0u, 0xB0u, 11u, 64u)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(abs(state.channels[0].expression - float(64) / 127.0) < 0.01)
    }
    t |> run("CC64 sets sustain pedal") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        B(d, 0u, 0xB0u, 64u, 127u)    // sustain on
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(state.channels[0].sustain)
    }
}

// ─── Program Change ───

[test]
def test_program_change(t : T?) {
    t |> run("program change updates channel") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 6u)
        B(d, 0u, 0xC0u, 42u)          // program change ch0 to 42
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> equal(42, state.channels[0].program)
    }
}

// ─── Tempo Change ───

[test]
def test_tempo_change(t : T?) {
    t |> run("tempo event updates ticks_per_sec") <| @(t : T?) {
        // Build MIDI with tempo = 250000 usec/qn = 240 BPM
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 10u)
        B(d, 0u, 0xFFu, 0x51u, 3u)    // tempo meta event, length 3
        B(d, 0x03u, 0xD0u, 0x90u)     // 250000 = 0x03D090
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        let old_tps = state.ticks_per_sec
        process_events(state)
        // 240 BPM = ticks_per_sec should be 2x default (120 BPM)
        t |> success(state.ticks_per_sec > old_tps * 1.5lf)
    }
}

// ─── Multi-Voice Polyphony ───

[test]
def test_polyphony(t : T?) {
    t |> run("multiple note-ons create multiple voices") <| @(t : T?) {
        // 3 notes on channel 0
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 13u)
        B(d, 0u, 0x90u, 60u, 100u)    // note on C4
        B(d, 0u, 0x90u, 64u, 100u)    // note on E4
        B(d, 0u, 0x90u, 67u, 100u)    // note on G4
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> equal(3, length(state.voices))
    }
    t |> run("note-offs release independently") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 16u)
        B(d, 0u, 0x90u, 60u, 100u)    // note on C4
        B(d, 0u, 0x90u, 64u, 100u)    // note on E4
        B(d, 0u, 0x80u, 60u, 0u)      // note off C4
        B(d, 0u, 0x80u, 64u, 0u)      // note off E4
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        // both voices should have note_off_t set
        var released = 0
        for (voice in state.voices) {
            if (voice.note_off_t >= 0.0) {
                released ++
            }
        }
        t |> equal(2, released)
    }
}

// ─── Voice Cleanup ───

[test]
def test_voice_cleanup(t : T?) {
    t |> run("finished voices are removed after tick") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 10u)
        B(d, 0u, 0x90u, 60u, 100u)    // note on
        B(d, 0u, 0x80u, 60u, 0u)      // note off immediately
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        // tick enough for the voice to finish its release
        for (_ in range(100)) {
            var chunk <- midi_tick(state, 0.05)
            delete chunk
        }
        // all voices should be cleaned up
        t |> equal(0, length(state.voices))
    }
}

// ─── Running Status ───

[test]
def test_running_status(t : T?) {
    t |> run("consecutive note-ons with running status") <| @(t : T?) {
        // Running status: after 0x90, subsequent messages omit the status byte
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 11u)
        B(d, 0u, 0x90u, 60u, 100u)    // note on C4 (with status)
        B(d, 0u, 64u, 100u)           // note on E4 (running status, no 0x90)
        B(d, 0u, 67u, 100u)           // note on G4 (running status)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        let events = merge_tracks(midi)
        var note_ons = 0
        for (ev in events) {
            if (ev.kind == MidiEventKind.midi_note_on) {
                note_ons ++
            }
        }
        t |> equal(3, note_ons)
    }
}

// ─── Pitch Bend ───

[test]
def test_pitch_bend(t : T?) {
    t |> run("pitch bend parsed from MIDI") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        // pitch bend ch0: LSB=0, MSB=96 → value = 96*128 = 12288 (above center 8192)
        B(d, 0u, 0xE0u, 0u, 96u)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        let events = merge_tracks(midi)
        var found = false
        for (ev in events) {
            if (ev.kind == MidiEventKind.midi_pitch_bend) {
                t |> equal(12288, ev.data1)
                t |> equal(0, ev.channel)
                found = true
            }
        }
        t |> success(found)
    }
    t |> run("pitch bend updates channel state") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        // center (8192) = LSB=0, MSB=64
        B(d, 0u, 0xE0u, 0u, 64u)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(abs(state.channels[0].pitch_bend) < 0.01)  // center = 0 semitones
    }
    t |> run("pitch bend max up = +2 semitones") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        // max bend up: LSB=127, MSB=127 → 16383
        B(d, 0u, 0xE0u, 127u, 127u)
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> success(abs(state.channels[0].pitch_bend - 2.0) < 0.01)
    }
}

// ─── Mod Wheel ───

[test]
def test_mod_wheel(t : T?) {
    t |> run("CC1 sets mod wheel") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 96u)
        push_track_header(d, 7u)
        B(d, 0u, 0xB0u, 1u, 100u)   // CC1 ch0, value 100
        B(d, 0u, 0xFFu, 0x2Fu, 0u)
        let midi = parse_midi(d)
        var state = MidiPlaybackState()
        init_playback(state, unsafe(addr(midi)), "test", 1.0, false)
        process_events(state)
        t |> equal(100, state.channels[0].mod_wheel)
    }
}
