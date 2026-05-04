// Tutorial AUDIO-02: Playing Audio Files
//
// This tutorial covers:
//   - Playing audio directly from a file with play_sound_from_file
//   - Decoding MP3 to PCM with decode_audio
//   - Looping sounds with play_sound_loop_from_pcm
//   - Stopping sounds with stop
//
// Run: daslang.exe tutorials/dasAudio/02_playing_files.das

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Play directly from a WAV file
// ──────────────────────────────────────────────────────────────────────────
//
// play_sound_from_file loads and decodes a file (WAV, MP3, FLAC) on the
// audio thread. You specify the expected sample rate and channel count.
// Returns a SID that can be used to control the sound.

def example_play_from_file() {
    print("=== Section 1: Play from WAV file ===\n")
    let path = "{get_das_root()}/examples/media/audio/gong.wav"
    print("  Loading: {path}\n")
    let sid = play_sound_from_file(path, MA_SAMPLE_RATE, 2)
    print("  Playing gong, SID = {sid}\n")
    sleep(2000u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Decode MP3 to PCM, inspect, and play
// ──────────────────────────────────────────────────────────────────────────
//
// decode_audio takes raw file bytes and returns an array<float> of
// interleaved PCM samples. It also outputs the channel count and sample
// rate via out-parameters. This gives you full control over the audio
// data before playback.

def example_decode_and_play() {
    print("\n=== Section 2: Decode MP3 to PCM ===\n")
    let path = "{get_das_root()}/examples/media/audio/music_cc0.mp3"
    print("  Decoding: {path}\n")

    var channels : int
    var rate : int
    var inscope samples : array<float>
    fopen(path, "rb") <| $(f) {
        if (f == null) {
            print("  ERROR: Could not open file\n")
            return
        }
        fmap(f) <| $(data) {
            samples <- decode_audio(data, channels, rate)
        }
    }

    if (length(samples) == 0) {
        print("  ERROR: Failed to decode audio\n")
        return
    }

    let num_frames = length(samples) / channels
    let duration = float(num_frames) / float(rate)
    print("  Channels:  {channels}\n")
    print("  Rate:      {rate} Hz\n")
    print("  Frames:    {num_frames}\n")
    print("  Duration:  {duration:.1f} seconds\n")

    // Play the first 3 seconds, then stop
    let sid = play_sound_from_pcm(rate, channels, samples)
    print("  Playing decoded MP3, SID = {sid}\n")
    sleep(3000u)
    stop(sid, 0.0)
    print("  Stopped\n")
    sleep(200u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Looping a short tone
// ──────────────────────────────────────────────────────────────────────────
//
// play_sound_loop_from_pcm plays the samples in an infinite loop until
// you stop it. Great for background music, ambient sounds, or drones.
// Here we generate a short 0.5-second tone and loop it.

def example_looping() {
    print("\n=== Section 3: Looping a short tone ===\n")
    let half_second = MA_SAMPLE_RATE / 2
    var samples <- [for (x in range(half_second)); sin(2.0 * PI * 330.0 * float(x) / float(MA_SAMPLE_RATE))]
    print("  Generated {length(samples)} samples (0.5 seconds at 330 Hz)\n")
    let sid = play_sound_loop_from_pcm(MA_SAMPLE_RATE, 1, samples)
    print("  Looping, SID = {sid}\n")
    sleep(2000u)
    stop(sid, 0.0)
    print("  Stopped loop\n")
    sleep(200u)
}

[export]
def main() {
    print("AUDIO-02: Playing Audio Files\n\n")
    with_audio_system() {
        example_play_from_file()
        example_decode_and_play()
        example_looping()
        print("\nDone!\n")
    }
}
