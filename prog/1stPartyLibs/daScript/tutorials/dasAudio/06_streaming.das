// Tutorial AUDIO-06: Streaming Audio
//
// This tutorial covers:
//   - Creating a PCM stream with play_sound_from_pcm_stream
//   - Feeding audio chunks with append_to_pcm
//   - Building a real-time frequency sweep
//   - Using with_collected_audio_system and audio_system_collect
//
// Run: daslang.exe tutorials/dasAudio/06_streaming.das

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Basic streaming with frequency sweep
// ──────────────────────────────────────────────────────────────────────────
//
// Unlike play_sound_from_pcm (which takes all samples at once),
// play_sound_from_pcm_stream creates a channel that you feed incrementally
// with append_to_pcm. This is essential for:
//   - Real-time synthesis (generating audio on the fly)
//   - Network audio (receiving chunks over time)
//   - Procedural music (composing while playing)
//
// Here we generate a sine wave that sweeps from 220 Hz to 880 Hz over
// 3 seconds, appending small chunks (100ms each) in a loop.

def example_streaming_sweep() {
    print("=== Section 1: Streaming frequency sweep (220 Hz -> 880 Hz) ===\n")

    let sid = play_sound_from_pcm_stream(MA_SAMPLE_RATE, 1)
    print("  Created stream, SID = {sid}\n")

    let chunk_duration = 0.1                                // 100ms per chunk
    let chunk_samples = int(float(MA_SAMPLE_RATE) * chunk_duration)
    let total_duration = 3.0                                // 3 seconds total
    let num_chunks = int(total_duration / chunk_duration)    // 30 chunks
    let start_freq = 220.0
    let end_freq = 880.0

    var phase = 0.0

    for (i in range(num_chunks)) {
        // Interpolate frequency for this chunk
        let t = float(i) / float(num_chunks)
        let freq = start_freq + (end_freq - start_freq) * t

        // Generate one chunk of sine wave, maintaining phase continuity
        var samples <- [for (x in range(chunk_samples));
            sin(phase + 2.0 * PI * freq * float(x) / float(MA_SAMPLE_RATE)) * 0.5
        ]

        // Advance phase for next chunk (avoid discontinuities)
        phase += 2.0 * PI * freq * float(chunk_samples) / float(MA_SAMPLE_RATE)

        append_to_pcm(sid, samples)

        if (i % 10 == 0) {
            print("  Chunk {i}/{num_chunks} — freq = {freq:.0f} Hz\n")
        }

        sleep(uint(chunk_duration * 1000.0))
    }

    print("  Sweep complete\n")
    sleep(500u)
    stop(sid, 0.1)
    sleep(200u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Collected audio system
// ──────────────────────────────────────────────────────────────────────────
//
// with_collected_audio_system is an alternative to with_audio_system that
// enables garbage collection for audio resources. When using it, you must
// call audio_system_collect periodically to process deferred cleanup.
//
// This is useful for long-running applications (games, tools) where audio
// channels are created and destroyed over time.

def example_collected_system() {
    print("\n=== Section 2: Collected audio system ===\n")
    print("  with_collected_audio_system enables GC for audio resources.\n")
    print("  Call audio_system_collect() periodically for cleanup.\n\n")

    with_collected_audio_system() {
        // Play a short tone
        let sid = play_sound_from_pcm_stream(MA_SAMPLE_RATE, 1)
        print("  Created stream in collected system, SID = {sid}\n")

        let chunk_samples = MA_SAMPLE_RATE / 10  // 100ms
        for (i in range(10)) {
            let freq = 440.0 + float(i) * 44.0
            var samples <- [for (x in range(chunk_samples));
                sin(2.0 * PI * freq * float(x) / float(MA_SAMPLE_RATE)) * 0.4
            ]
            append_to_pcm(sid, samples)
            // Call collect periodically to process cleanup
            audio_system_collect()
            sleep(100u)
        }

        print("  Played 1 second of ascending tones with periodic GC\n")
        stop(sid, 0.1)
        audio_system_collect()
        sleep(200u)
    }
}

[export]
def main {
    print("AUDIO-06: Streaming Audio\n\n")
    with_audio_system() {
        example_streaming_sweep()
    }
    example_collected_system()
    print("\nDone!\n")
}
