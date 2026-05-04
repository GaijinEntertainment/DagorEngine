options gen2
options indenting = 4
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require strudel/strudel_midi

// Helper: append bytes (uint params since hex literals are uint in daslang)
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
def B6(var arr : array<uint8>; b0, b1, b2, b3, b4, b5 : uint) {
    B(arr, b0, b1, b2, b3); B(arr, b4, b5)
}

// Standard MIDI header
def push_midi_header(var d : array<uint8>; fmt, nTracks, tpqn : uint) {
    B(d, 0x4Du, 0x54u, 0x68u, 0x64u)             // "MThd"
    B(d, 0u, 0u, 0u, 6u)                          // header length
    B(d, 0u, fmt)                                  // format
    B(d, 0u, nTracks)                              // tracks
    B(d, (tpqn >> 8u) & 0xFFu, tpqn & 0xFFu)      // division
}

// ─── VLQ ───

[test]
def test_vlq(t : T?) {
    t |> run("single byte 0") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0u)
        var cursor = 0
        t |> equal(read_vlq(data, cursor), 0)
        t |> equal(cursor, 1)
    }
    t |> run("single byte 127") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0x7Fu)
        var cursor = 0
        t |> equal(read_vlq(data, cursor), 127)
    }
    t |> run("two bytes 128") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0x81u, 0x00u)
        var cursor = 0
        t |> equal(read_vlq(data, cursor), 128)
        t |> equal(cursor, 2)
    }
    t |> run("two bytes 480") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0x83u, 0x60u)
        var cursor = 0
        t |> equal(read_vlq(data, cursor), 480)
    }
}

// ─── Binary Readers ───

[test]
def test_binary_readers(t : T?) {
    t |> run("read_u16 big-endian") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0x01u, 0xE0u)
        var cursor = 0
        t |> equal(read_u16(data, cursor), 480)
    }
    t |> run("read_u32 big-endian") <| @(t : T?) {
        var data : array<uint8>
        B(data, 0u, 0u, 1u, 0u)
        var cursor = 0
        t |> equal(read_u32(data, cursor), 256)
    }
    t |> run("read_byte past end") <| @(t : T?) {
        var data : array<uint8>
        B(data, 42u)
        var cursor = 1
        t |> equal(read_byte(data, cursor), -1)
    }
}

// ─── Minimal MIDI File ───

def make_test_midi() : array<uint8> {
    var d : array<uint8>
    push_midi_header(d, 0u, 1u, 480u)
    B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)  // "MTrk"
    let lenIdx = length(d)
    B(d, 0u, 0u, 0u, 0u)  // track length placeholder
    let trackStart = length(d)
    // Tempo: delta=0, FF 51 03 07A120 (500000 us)
    B(d, 0x00u)
    B(d, 0xFFu, 0x51u, 0x03u)
    B(d, 0x07u, 0xA1u, 0x20u)
    // Note On C4 vel=100: delta=0
    B(d, 0x00u)
    B(d, 0x90u, 0x3Cu, 0x64u)
    // Note Off C4: delta=240 (VLQ: 0x81 0x70)
    B(d, 0x81u, 0x70u)
    B(d, 0x80u, 0x3Cu, 0x00u)
    // Note On E4: delta=240
    B(d, 0x81u, 0x70u)
    B(d, 0x90u, 0x40u, 0x64u)
    // Note Off E4: delta=240
    B(d, 0x81u, 0x70u)
    B(d, 0x80u, 0x40u, 0x00u)
    // Note On G4: delta=240
    B(d, 0x81u, 0x70u)
    B(d, 0x90u, 0x43u, 0x64u)
    // Note Off G4: delta=240
    B(d, 0x81u, 0x70u)
    B(d, 0x80u, 0x43u, 0x00u)
    // End of Track: delta=0
    B(d, 0x00u)
    B(d, 0xFFu, 0x2Fu, 0x00u)
    // fill in track length
    let trackLen = uint(length(d) - trackStart)
    d[lenIdx]     = uint8((trackLen >> 24u) & 0xFFu)
    d[lenIdx + 1] = uint8((trackLen >> 16u) & 0xFFu)
    d[lenIdx + 2] = uint8((trackLen >> 8u) & 0xFFu)
    d[lenIdx + 3] = uint8(trackLen & 0xFFu)
    return <- d
}

[test]
def test_parse_format0(t : T?) {
    t |> run("header fields") <| @(t : T?) {
        let data <- make_test_midi()
        let midi <- parse_midi(data)
        t |> equal(midi.format, 0)
        t |> equal(midi.ticks_per_qn, 480)
        t |> equal(length(midi.tracks), 1)
    }
    t |> run("event count and types") <| @(t : T?) {
        let data <- make_test_midi()
        let midi <- parse_midi(data)
        t |> equal(length(midi.tracks[0].events), 8)
        t |> equal(midi.tracks[0].events[0].kind, MidiEventKind.midi_tempo)
        t |> equal(midi.tracks[0].events[0].tempo, 500000)
        t |> equal(midi.tracks[0].events[1].kind, MidiEventKind.midi_note_on)
        t |> equal(midi.tracks[0].events[1].data1, 60, "C4=60")
        t |> equal(midi.tracks[0].events[1].data2, 100, "velocity=100")
        t |> equal(midi.tracks[0].events[7].kind, MidiEventKind.midi_end_of_track)
    }
    t |> run("absolute tick positions") <| @(t : T?) {
        let data <- make_test_midi()
        let midi <- parse_midi(data)
        t |> equal(midi.tracks[0].events[0].tick, 0, "tempo at 0")
        t |> equal(midi.tracks[0].events[1].tick, 0, "C4 on at 0")
        t |> equal(midi.tracks[0].events[2].tick, 240, "C4 off at 240")
        t |> equal(midi.tracks[0].events[3].tick, 480, "E4 on at 480")
        t |> equal(midi.tracks[0].events[4].tick, 720, "E4 off at 720")
        t |> equal(midi.tracks[0].events[5].tick, 960, "G4 on at 960")
        t |> equal(midi.tracks[0].events[6].tick, 1200, "G4 off at 1200")
    }
}

// ─── Velocity 0 = Note Off ───

[test]
def test_velocity_zero(t : T?) {
    t |> run("note on with velocity 0 becomes note off") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 480u)
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)  // MTrk
        B(d, 0u, 0u, 0u, 9u)              // track length
        B(d, 0x00u, 0x90u, 0x3Cu, 0x64u)  // note on C4 vel=100
        B(d, 0x81u, 0x70u)                // delta=240
        B(d, 0x90u, 0x3Cu, 0x00u)         // note on vel=0 = note off
        let midi <- parse_midi(d)
        t |> equal(length(midi.tracks[0].events), 2)
        t |> equal(midi.tracks[0].events[0].kind, MidiEventKind.midi_note_on)
        t |> equal(midi.tracks[0].events[1].kind, MidiEventKind.midi_note_off, "vel 0 = note off")
    }
}

// ─── Program Change ───

[test]
def test_program_change(t : T?) {
    t |> run("program change parsed") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 480u)
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 3u)
        B(d, 0x00u, 0xC2u, 0x28u)         // program change ch=2 prog=40
        let midi <- parse_midi(d)
        t |> equal(length(midi.tracks[0].events), 1)
        t |> equal(midi.tracks[0].events[0].kind, MidiEventKind.midi_program_change)
        t |> equal(midi.tracks[0].events[0].channel, 2)
        t |> equal(midi.tracks[0].events[0].data1, 40, "program=40")
    }
}

// ─── Format 1 + merge_tracks ───

[test]
def test_format1_merge(t : T?) {
    t |> run("two tracks merged and sorted") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 1u, 2u, 480u)
        // Track 1: tempo
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 11u)
        B(d, 0x00u, 0xFFu, 0x51u, 0x03u)
        B(d, 0x07u, 0xA1u, 0x20u)
        B(d, 0x00u, 0xFFu, 0x2Fu, 0x00u)
        // Track 2: note
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 11u)
        B(d, 0x00u, 0x90u, 0x3Cu, 0x64u)
        B(d, 0x83u, 0x60u)                // delta=480
        B(d, 0x80u, 0x3Cu, 0x00u)
        B(d, 0x00u, 0xFFu, 0x2Fu, 0x00u)
        let midi <- parse_midi(d)
        t |> equal(midi.format, 1)
        t |> equal(length(midi.tracks), 2)
        var merged <- merge_tracks(midi)
        t |> success(length(merged) > 0, "merged has events")
        var sorted = true
        for (i in range(1, length(merged))) {
            if (merged[i].tick < merged[i - 1].tick) {
                sorted = false
            }
        }
        t |> success(sorted, "events sorted by tick")
        var has_tempo = false
        var has_note = false
        for (ev in merged) {
            if (ev.kind == MidiEventKind.midi_tempo) { has_tempo = true; }
            if (ev.kind == MidiEventKind.midi_note_on) { has_note = true; }
        }
        t |> success(has_tempo, "tempo from track 1")
        t |> success(has_note, "note from track 2")
    }
}

// ─── Edge Cases ───

[test]
def test_edge_cases(t : T?) {
    t |> run("empty data") <| @(t : T?) {
        let empty : array<uint8>
        let midi <- parse_midi(empty)
        t |> equal(length(midi.tracks), 0)
    }
    t |> run("wrong magic") <| @(t : T?) {
        var d : array<uint8>
        B(d, 0x4Du, 0x54u, 0x68u, 0x58u)  // "MThX"
        B(d, 0u, 0u, 0u, 6u)
        B6(d, 0u, 0u, 0u, 1u, 0x01u, 0xE0u)
        let midi <- parse_midi(d)
        t |> equal(length(midi.tracks), 0)
    }
    t |> run("truncated data no crash") <| @(t : T?) {
        var d : array<uint8>
        push_midi_header(d, 0u, 1u, 480u)
        B(d, 0x4Du, 0x54u, 0x72u, 0x6Bu)
        B(d, 0u, 0u, 0u, 0xFFu)  // claims 255 bytes but data ends here
        let _midi <- parse_midi(d)
        t |> success(true, "no crash on truncated data")
    }
}
