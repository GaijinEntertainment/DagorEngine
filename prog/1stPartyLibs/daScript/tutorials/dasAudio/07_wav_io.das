// Tutorial AUDIO-07: WAV File I/O
//
// This tutorial covers:
//   - Generating PCM data and saving to WAV with write_wav
//   - Reading WAV files back with read_wav
//   - Simple audio processing: reversing and amplifying
//   - Playing both original and processed audio
//
// Run: daslang.exe tutorials/dasAudio/07_wav_io.das

options gen2
options indenting = 4

require audio/audio_boost
require audio/audio_wav
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Generate and save a WAV file
// ──────────────────────────────────────────────────────────────────────────
//
// write_wav saves an array<float> of PCM samples as a 16-bit WAV file.
// Here we generate a 1-second 440 Hz sine wave and write it to disk.

def example_generate_and_save(out_path : string) {
    print("=== Section 1: Generate and save WAV ===\n")

    var samples <- [for (x in range(MA_SAMPLE_RATE));
        sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE)) * 0.8
    ]
    print("  Generated {length(samples)} samples (1 second at 440 Hz)\n")

    write_wav(out_path, samples, uint(MA_SAMPLE_RATE), 1)
    print("  Saved to {out_path}\n")

    delete samples
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Read WAV file back and verify
// ──────────────────────────────────────────────────────────────────────────
//
// read_wav reads a WAV file into an array<float>, returning the sample
// rate and channel count via out-parameters. Returns true on success.

def example_read_back(path : string) {
    print("\n=== Section 2: Read WAV back ===\n")

    var samples : array<float>
    var sample_rate : int
    var channels : int

    if (!read_wav(path, samples, sample_rate, channels)) {
        print("  ERROR: Failed to read {path}\n")
        return
    }

    print("  Read {path}\n")
    print("  Sample rate: {sample_rate} Hz\n")
    print("  Channels:    {channels}\n")
    print("  Samples:     {length(samples)}\n")
    print("  Duration:    {float(length(samples)) / float(sample_rate * channels):.2f} seconds\n")

    // Verify first few samples are close to expected sine values
    print("  First 5 samples:\n")
    for (i in range(min(5, length(samples)))) {
        let expected = sin(2.0 * PI * 440.0 * float(i) / float(sample_rate)) * 0.8
        print("    [{i}] = {samples[i]:.4f}  (expected ~{expected:.4f})\n")
    }
    print("  Note: small differences are from 16-bit quantization\n")

    delete samples
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Reverse audio
// ──────────────────────────────────────────────────────────────────────────
//
// Read gong.wav, reverse the samples, write the result, then play both
// the original and reversed versions.

def example_reverse(media_path, out_path : string) {
    print("\n=== Section 3: Reverse audio ===\n")

    let gong_path = "{media_path}/audio/gong.wav"
    var samples : array<float>
    var sample_rate : int
    var channels : int

    if (!read_wav(gong_path, samples, sample_rate, channels)) {
        print("  ERROR: Could not read {gong_path}\n")
        return
    }

    print("  Read gong.wav: {length(samples)} samples, {sample_rate} Hz, {channels} ch\n")

    // Reverse the samples (swap from both ends toward the middle)
    let n = length(samples)
    for (i in range(n / 2)) {
        let tmp = samples[i]
        samples[i] = samples[n - 1 - i]
        samples[n - 1 - i] = tmp
    }

    write_wav(out_path, samples, uint(sample_rate), channels)
    print("  Saved reversed gong to {out_path}\n")

    // Play original
    print("  Playing original gong...\n")
    var original : array<float>
    var sr2 : int
    var ch2 : int
    if (read_wav(gong_path, original, sr2, ch2)) {
        play_sound_from_pcm(sr2, ch2, original)
        sleep(2500u)
    }

    // Play reversed
    print("  Playing reversed gong...\n")
    play_sound_from_pcm(sample_rate, channels, samples)
    sleep(2500u)

    remove(out_path)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Amplify (halve volume)
// ──────────────────────────────────────────────────────────────────────────
//
// Read gong.wav, multiply all samples by 0.5 to halve the volume, save
// the result, and play both for comparison.

def example_amplify(media_path, out_path : string) {
    print("\n=== Section 4: Amplify (halve volume) ===\n")

    let gong_path = "{media_path}/audio/gong.wav"
    var samples : array<float>
    var sample_rate : int
    var channels : int

    if (!read_wav(gong_path, samples, sample_rate, channels)) {
        print("  ERROR: Could not read {gong_path}\n")
        return
    }

    // Halve all sample values
    for (i in range(length(samples))) {
        samples[i] *= 0.5
    }

    write_wav(out_path, samples, uint(sample_rate), channels)
    print("  Saved half-volume gong to {out_path}\n")

    // Play original at full volume
    print("  Playing original gong (full volume)...\n")
    var original : array<float>
    var sr2 : int
    var ch2 : int
    if (read_wav(gong_path, original, sr2, ch2)) {
        play_sound_from_pcm(sr2, ch2, original)
        sleep(2500u)
    }

    // Play quieter version
    print("  Playing half-volume gong...\n")
    play_sound_from_pcm(sample_rate, channels, samples)
    sleep(2500u)

    remove(out_path)
}

[export]
def main {
    print("AUDIO-07: WAV File I/O\n\n")

    let media_path = "{get_das_root()}/examples/media"
    let tmp_dir = "{get_das_root()}/tutorials/dasAudio"
    let tone_path = "{tmp_dir}/_tutorial_tone.wav"
    let reversed_path = "{tmp_dir}/_tutorial_gong_reversed.wav"
    let quiet_path = "{tmp_dir}/_tutorial_gong_quiet.wav"

    // Sections 1 and 2 don't need audio playback
    example_generate_and_save(tone_path)
    example_read_back(tone_path)
    remove(tone_path)

    // Sections 3 and 4 play audio
    with_audio_system() {
        example_reverse(media_path, reversed_path)
        example_amplify(media_path, quiet_path)
        print("\nDone!\n")
    }
}
