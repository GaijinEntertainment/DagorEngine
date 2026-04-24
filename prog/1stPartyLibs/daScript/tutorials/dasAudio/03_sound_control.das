// Tutorial AUDIO-03: Sound Control
//
// This tutorial covers:
//   - Volume control with set_volume (instant and fade)
//   - Panning with set_pan (left/right)
//   - Pitch shifting with set_pitch
//   - Pause and resume with set_pause
//   - Batching multiple changes with batch
//   - Stopping with fade using stop
//
// Run: daslang.exe tutorials/dasAudio/03_sound_control.das

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Helper — generate a 1-second 440 Hz sine tone for reuse
// ──────────────────────────────────────────────────────────────────────────

def generate_tone() : array<float> {
    return <- [for (x in range(MA_SAMPLE_RATE)); sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))]
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Instant volume changes
// ──────────────────────────────────────────────────────────────────────────
//
// set_volume(sid, volume, time) sets the volume. The default time is
// 0.002s (2ms ramp to avoid clicks). Volume 1.0 is full, 0.0 is silent.

def example_volume(sid : uint64) {
    print("=== Section 1: Volume ===\n")
    set_volume(sid, 0.3)
    print("  Volume 0.3 (quiet)\n")
    sleep(1000u)
    set_volume(sid, 1.0)
    print("  Volume 1.0 (full)\n")
    sleep(1000u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Volume fade
// ──────────────────────────────────────────────────────────────────────────
//
// When the time parameter is > 0, the volume change happens as a smooth
// fade over that many seconds. Useful for crossfades and fade-outs.

def example_volume_fade(sid : uint64) {
    print("\n=== Section 2: Volume fade ===\n")
    set_volume(sid, 0.1, 2.0)
    print("  Fading to 0.1 over 2 seconds...\n")
    sleep(2000u)
    set_volume(sid, 1.0, 0.5)
    print("  Fading back to 1.0 over 0.5 seconds...\n")
    sleep(1000u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Pan (left/right stereo position)
// ──────────────────────────────────────────────────────────────────────────
//
// set_pan(sid, pan) positions the sound in the stereo field.
// -1.0 = full left, 0.0 = center, 1.0 = full right.

def example_pan(sid : uint64) {
    print("\n=== Section 3: Pan ===\n")
    set_pan(sid, -1.0)
    print("  Pan -1.0 (left)\n")
    sleep(1000u)
    set_pan(sid, 1.0)
    print("  Pan  1.0 (right)\n")
    sleep(1000u)
    set_pan(sid, 0.0)
    print("  Pan  0.0 (center)\n")
    sleep(500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Pitch shifting
// ──────────────────────────────────────────────────────────────────────────
//
// set_pitch(sid, pitch) changes the playback speed/pitch.
// 1.0 = normal, 2.0 = one octave up, 0.5 = one octave down.

def example_pitch(sid : uint64) {
    print("\n=== Section 4: Pitch ===\n")
    set_pitch(sid, 2.0)
    print("  Pitch 2.0 (one octave up)\n")
    sleep(1000u)
    set_pitch(sid, 0.5)
    print("  Pitch 0.5 (one octave down)\n")
    sleep(1000u)
    set_pitch(sid, 1.0)
    print("  Pitch 1.0 (normal)\n")
    sleep(500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Pause and resume
// ──────────────────────────────────────────────────────────────────────────
//
// set_pause(sid, true) pauses the sound, set_pause(sid, false) resumes it.
// The sound picks up exactly where it left off.

def example_pause(sid : uint64) {
    print("\n=== Section 5: Pause and resume ===\n")
    set_pause(sid, true)
    print("  Paused\n")
    sleep(1500u)
    set_pause(sid, false)
    print("  Resumed\n")
    sleep(1000u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Batching multiple changes
// ──────────────────────────────────────────────────────────────────────────
//
// batch() groups multiple audio commands into a single atomic update.
// All changes inside the block take effect together on the audio thread,
// avoiding audible glitches from applying them one by one.

def example_batch(sid : uint64) {
    print("\n=== Section 6: Batching ===\n")
    batch() {
        set_volume(sid, 0.5, 0.0)
        set_pan(sid, -0.5)
        set_pitch(sid, 1.5)
    }
    print("  Applied volume=0.5, pan=-0.5, pitch=1.5 in one batch\n")
    sleep(1000u)
    // Reset
    batch() {
        set_volume(sid, 1.0, 0.0)
        set_pan(sid, 0.0)
        set_pitch(sid, 1.0)
    }
    print("  Reset to defaults\n")
    sleep(500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Stop with fade
// ──────────────────────────────────────────────────────────────────────────
//
// stop(sid, time) stops a sound. When time > 0 it fades out over that
// many seconds before stopping. This avoids audible clicks.

def example_stop_with_fade(sid : uint64) {
    print("\n=== Section 7: Stop with fade ===\n")
    stop(sid, 1.0)
    print("  Fading out over 1 second...\n")
    sleep(1500u)
    print("  Sound stopped\n")
}

[export]
def main() {
    print("AUDIO-03: Sound Control\n\n")
    with_audio_system() {
        // Create a looping tone so it keeps playing across all sections
        var tone <- generate_tone()
        let sid = play_sound_loop_from_pcm(MA_SAMPLE_RATE, 1, tone)
        print("Looping 440 Hz tone, SID = {sid}\n\n")

        example_volume(sid)
        example_volume_fade(sid)
        example_pan(sid)
        example_pitch(sid)
        example_pause(sid)
        example_batch(sid)
        example_stop_with_fade(sid)

        print("\nDone!\n")
    }
}
