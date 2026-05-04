// Tutorial AUDIO-08: MIDI Files
//
// This tutorial covers:
//   - Parsing MIDI files with load_midi
//   - Inspecting tracks, events, and tempo
//   - Computing MIDI duration from tick/tempo data
//   - Playing MIDI with midi_play and midi_load_samples
//   - Cross-fading between two MIDI files with midi_set_volume
//
// Run: daslang.exe tutorials/dasAudio/08_midi.das

options gen2
options indenting = 4

require strudel/strudel_midi
require strudel/strudel_midi_player
require audio/audio_boost
require daslib/fio
require daslib/strings_boost
require math

// ──────────────────────────────────────────────────────────────────────────
// Part A — MIDI Parsing
// ──────────────────────────────────────────────────────────────────────────
//
// load_midi reads a .mid file and returns a MidiFile struct containing:
//   - format:      MIDI format (0 = single track, 1 = multi-track)
//   - ticks_per_qn: ticks per quarter note (timing resolution)
//   - tracks:      array of MidiTrack, each with an array of MidiEvent
//
// merge_tracks combines all tracks into a single sorted event list,
// which is easier to iterate for analysis or playback.

def example_parse_midi(path : string) {
    print("=== Part A: MIDI Parsing ===\n")
    print("  Loading: {path}\n\n")

    let midi = load_midi(path)

    if (length(midi.tracks) == 0) {
        print("  ERROR: Failed to load MIDI file\n")
        return
    }

    print("  Format:       {midi.format}\n")
    print("  Ticks per QN: {midi.ticks_per_qn}\n")
    print("  Tracks:       {length(midi.tracks)}\n")

    // Count total events per track
    for (i in range(length(midi.tracks))) {
        print("    Track {i}: {length(midi.tracks[i].events)} events\n")
    }

    // Merge all tracks and show the first 20 events
    var merged <- merge_tracks(midi)
    print("\n  First 20 merged events:\n")
    let show_count = min(20, length(merged))
    for (idx in range(show_count)) {
        let evt = merged[idx]
        print("    [{idx:2}] tick={evt.tick:6}  kind={evt.kind}  ch={evt.channel}  d1={evt.data1:3}  d2={evt.data2:3}")
        if (evt.kind == MidiEventKind.midi_tempo) {
            let bpm = 60000000.0 / float(evt.tempo)
            print("  tempo={evt.tempo} ({bpm:.1f} BPM)")
        }
        print("\n")
    }

    // Compute total duration by walking through all events and tracking tempo
    var tempo_us = 500000      // default: 120 BPM (500000 microseconds per QN)
    var total_time_s = 0.0
    var prev_tick = 0
    for (evt in merged) {
        if (evt.tick > prev_tick) {
            let delta_ticks = evt.tick - prev_tick
            let seconds_per_tick = float(tempo_us) / 1000000.0 / float(midi.ticks_per_qn)
            total_time_s += float(delta_ticks) * seconds_per_tick
        }
        prev_tick = evt.tick
        if (evt.kind == MidiEventKind.midi_tempo && evt.tempo > 0) {
            tempo_us = evt.tempo
        }
    }

    print("\n  Computed duration: {total_time_s:.1f} seconds\n")

    delete merged
}

// ──────────────────────────────────────────────────────────────────────────
// Part B — MIDI Playback
// ──────────────────────────────────────────────────────────────────────────
//
// The MIDI player uses the audio system's streaming infrastructure.
// midi_load_samples loads the built-in piano and drum sample banks.
// midi_init starts the MIDI playback thread.
// midi_play starts a named MIDI track (supports multiple simultaneous).

def example_play_midi(media_path, midi_path : string) {
    print("\n=== Part B: MIDI Playback ===\n")

    midi_load_samples(media_path)
    midi_init()

    if (midi_play("music", midi_path, [gain = 0.8, looping = true])) {
        print("  Playing: {midi_path}\n")
        print("  Listening for 5 seconds...\n")
        sleep(5000u)
    } else {
        print("  ERROR: Failed to play MIDI file\n")
    }

    midi_stop("music")
    sleep(200u)
    midi_shutdown()
    print("  Stopped playback\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Part C — Cross-fading two MIDI files
// ──────────────────────────────────────────────────────────────────────────
//
// midi_set_volume(name, volume, fade_time) smoothly transitions the volume
// over fade_time seconds. By running two MIDI tracks simultaneously and
// fading one down while fading the other up, we get a cross-fade effect.

def example_crossfade(media_path : string; path_a, path_b : string) {
    print("\n=== Part C: Cross-fading two MIDI files ===\n")

    midi_load_samples(media_path)
    midi_init()

    // Start both tracks — track_a at full volume, track_b silent
    let ok_a = midi_play("track_a", path_a, [gain = 1.0, looping = true])
    let ok_b = midi_play("track_b", path_b, [gain = 0.0, looping = true])

    if (!ok_a || !ok_b) {
        print("  ERROR: Failed to load one or both MIDI files\n")
        midi_shutdown()
        return
    }

    print("  Track A: Fur Elise (volume 1.0)\n")
    print("  Track B: Bach Air  (volume 0.0)\n")
    print("  Listening to Fur Elise for 3 seconds...\n")
    sleep(3000u)

    // Cross-fade: A -> 0, B -> 1 over 3 seconds
    print("  Cross-fading A->B over 3 seconds...\n")
    midi_set_volume("track_a", 0.0, 3.0)
    midi_set_volume("track_b", 1.0, 3.0)
    sleep(5000u)

    // Cross-fade back: A -> 1, B -> 0 over 3 seconds
    print("  Cross-fading B->A over 3 seconds...\n")
    midi_set_volume("track_a", 1.0, 3.0)
    midi_set_volume("track_b", 0.0, 3.0)
    sleep(5000u)

    // Stop all and shut down
    midi_stop()
    sleep(200u)
    midi_shutdown()
    print("  Stopped all tracks\n")
}

[export]
def main {
    print("AUDIO-08: MIDI Files\n\n")

    let media_path = "{get_das_root()}/examples/media"
    let fur_elise = "{media_path}/midi/fur_elise.mid"
    let bach_air = "{media_path}/midi/Bach_Air_on_G_string_BWV1068.mid"

    // Part A — parsing (no audio needed)
    example_parse_midi(fur_elise)

    // Parts B and C — playback (needs audio system)
    with_audio_system() {
        example_play_midi(media_path, fur_elise)
        example_crossfade(media_path, fur_elise, bach_air)
    }

    print("\nDone!\n")
}
