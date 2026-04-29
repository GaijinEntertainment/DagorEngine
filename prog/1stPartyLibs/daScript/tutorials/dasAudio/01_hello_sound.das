// Tutorial AUDIO-01: Hello Sound (Generating a Beep)
//
// This tutorial covers:
//   - Initializing the audio system with with_audio_system
//   - Generating PCM samples (sine wave, square wave)
//   - Playing sounds with play_sound_from_pcm
//   - Understanding SID (sound identifier) and MA_SAMPLE_RATE
//
// Run: daslang.exe tutorials/dasAudio/01_hello_sound.das

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Generate and play a 440 Hz sine wave (concert A)
// ──────────────────────────────────────────────────────────────────────────
//
// MA_SAMPLE_RATE is the audio system's sample rate (typically 48000 Hz).
// We generate exactly 1 second of samples: a pure sine tone at 440 Hz.
// play_sound_from_pcm takes ownership of the array (move semantics).

def example_sine_beep() {
    print("=== Section 1: 440 Hz sine wave (concert A) ===\n")
    var samples <- [for (x in range(MA_SAMPLE_RATE)); sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))]
    print("  Generated {length(samples)} samples at {MA_SAMPLE_RATE} Hz\n")
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    print("  Playing 440 Hz sine wave, SID = {sid}\n")
    sleep(1100u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Different frequencies: middle C (261 Hz) and C5 (523 Hz)
// ──────────────────────────────────────────────────────────────────────────
//
// Musical notes correspond to specific frequencies. Here we play two notes
// sequentially to hear the difference.

def example_frequencies() {
    print("\n=== Section 2: Different frequencies ===\n")

    // Middle C (C4) at 261 Hz
    var samples_c4 <- [for (x in range(MA_SAMPLE_RATE)); sin(2.0 * PI * 261.0 * float(x) / float(MA_SAMPLE_RATE))]
    let sid_c4 = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples_c4)
    print("  Playing 261 Hz (middle C), SID = {sid_c4}\n")
    sleep(1100u)

    // C5 at 523 Hz (one octave higher)
    var samples_c5 <- [for (x in range(MA_SAMPLE_RATE)); sin(2.0 * PI * 523.0 * float(x) / float(MA_SAMPLE_RATE))]
    let sid_c5 = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples_c5)
    print("  Playing 523 Hz (C5, one octave up), SID = {sid_c5}\n")
    sleep(1100u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Square wave using sign(sin(...))
// ──────────────────────────────────────────────────────────────────────────
//
// A square wave has a harsher, buzzier timbre than a sine wave. We create
// one by taking the sign of a sine wave: +1 for the positive half, -1 for
// the negative half. Multiply by 0.3 to reduce volume (square waves are
// perceptually louder than sine waves at the same amplitude).

def example_square_wave() {
    print("\n=== Section 3: 440 Hz square wave ===\n")
    var samples <- [for (x in range(MA_SAMPLE_RATE)); sign(sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))) * 0.3]
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    print("  Playing 440 Hz square wave, SID = {sid}\n")
    print("  Notice the harsher, buzzier timbre compared to the sine wave\n")
    sleep(1100u)
}

[export]
def main() {
    print("AUDIO-01: Hello Sound\n\n")
    with_audio_system() {
        example_sine_beep()
        example_frequencies()
        example_square_wave()
        print("\nDone!\n")
    }
}
