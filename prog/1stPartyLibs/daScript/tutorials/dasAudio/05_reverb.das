// Tutorial AUDIO-05: Reverb and Effects
//
// This tutorial covers:
//   - Applying reverb to sounds with set_reverb_preset
//   - Comparing different ReverbPreset values
//   - Applying chorus with set_chorus_default and set_chorus
//   - Understanding how reverb and chorus affect sound character
//
// Run: daslang.exe tutorials/dasAudio/05_reverb.das

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Helper — generate a short impulse burst (decaying sine)
// ──────────────────────────────────────────────────────────────────────────
//
// A short impulse is ideal for hearing reverb tails. We use a 0.1-second
// sine burst at 880 Hz with exponential decay. The rapid decay leaves
// silence for the reverb tail to ring out.

def make_impulse() : array<float> {
    let num_samples = MA_SAMPLE_RATE / 10  // 0.1 seconds
    return <- [for (x in range(num_samples));
        sin(2.0 * PI * 880.0 * float(x) / float(MA_SAMPLE_RATE)) * exp(-float(x) / float(num_samples) * 6.0)
    ]
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Dry (no reverb)
// ──────────────────────────────────────────────────────────────────────────
//
// First, hear the impulse completely dry — no reverb applied. The sound
// is a short "tick" that ends abruptly after 0.1 seconds.

def example_dry() {
    print("=== Section 1: Dry impulse (no reverb) ===\n")
    var samples <- make_impulse()
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    print("  Playing dry impulse, SID = {sid}\n")
    print("  Notice how the sound ends abruptly\n")
    sleep(1500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Reverb presets
// ──────────────────────────────────────────────────────────────────────────
//
// set_reverb_preset applies an I3DL2 reverb effect to a playing sound.
// Different presets simulate different acoustic spaces. We play the same
// impulse with several presets so you can hear how each space colors the
// sound with its own decay time and character.

def example_reverb_presets() {
    print("\n=== Section 2: Reverb presets ===\n")

    // Bathroom — small, bright, short tail
    print("  [Bathroom] — small room, bright reflections\n")
    var s1 <- make_impulse()
    var sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s1)
    set_reverb_preset(sid, ReverbPreset.Bathroom)
    sleep(2000u)

    // ConcertHall — large, warm, long tail
    print("  [ConcertHall] — large space, warm, long decay\n")
    var s2 <- make_impulse()
    sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s2)
    set_reverb_preset(sid, ReverbPreset.ConcertHall)
    sleep(2500u)

    // Cave — very long, dark reverb
    print("  [Cave] — cavernous, very long decay\n")
    var s3 <- make_impulse()
    sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s3)
    set_reverb_preset(sid, ReverbPreset.Cave)
    sleep(2500u)

    // Underwater — muffled, filtered
    print("  [Underwater] — muffled and filtered\n")
    var s4 <- make_impulse()
    sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s4)
    set_reverb_preset(sid, ReverbPreset.Underwater)
    sleep(2500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Summary of preset characteristics
// ──────────────────────────────────────────────────────────────────────────
//
// Each I3DL2 preset models a different acoustic environment. Here's what
// you should have heard.

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Chorus effect
// ──────────────────────────────────────────────────────────────────────────
//
// Chorus adds a modulated delay to simulate multiple detuned copies of the
// sound, giving it width and shimmer. set_chorus_default applies the default
// settings; set_chorus lets you customize rate, depth, feedback, delay, and
// wet/dry mix via a ma_chorus_config.

def example_chorus() {
    print("\n=== Section 3: Chorus effect ===\n")

    // Generate a longer tone so the chorus modulation is audible
    let duration = MA_SAMPLE_RATE * 2  // 2 seconds
    var tone <- [for (x in range(duration));
        sin(2.0 * PI * 330.0 * float(x) / float(MA_SAMPLE_RATE)) * 0.5
    ]

    // Default chorus
    print("  [Default chorus] — subtle widening\n")
    var s1 <- clone(tone)
    var sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s1)
    set_chorus_default(sid)
    sleep(2500u)

    // Custom chorus — deeper, slower modulation
    print("  [Deep chorus] — slower rate, more depth\n")
    var s2 <- clone(tone)
    sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s2)
    var cfg = chorus_config_default()
    cfg.rate = 0.5
    cfg.depth = 0.8
    cfg.delay_ms = 15.0
    cfg.wet = 0.7
    set_chorus(sid, cfg)
    sleep(2500u)

    // Custom chorus — fast vibrato-like
    print("  [Fast chorus] — vibrato-like shimmer\n")
    var s3 <- tone
    sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, s3)
    cfg.rate = 3.0
    cfg.depth = 0.3
    cfg.delay_ms = 5.0
    cfg.wet = 0.5
    cfg.feedback = 0.3
    set_chorus(sid, cfg)
    sleep(2500u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Reverb + Chorus combined
// ──────────────────────────────────────────────────────────────────────────

def example_combined() {
    print("\n=== Section 4: Reverb + Chorus combined ===\n")
    print("  Impulse with ConcertHall reverb and default chorus\n")
    var samples <- make_impulse()
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    set_reverb_preset(sid, ReverbPreset.ConcertHall)
    set_chorus_default(sid)
    sleep(3000u)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Summary
// ──────────────────────────────────────────────────────────────────────────

def example_summary() {
    print("\n=== Section 5: Summary ===\n")
    print("  Reverb presets (23 total in ReverbPreset enum):\n")
    print("    Generic, PaddedCell, Room, Bathroom, LivingRoom, StoneRoom,\n")
    print("    Auditorium, ConcertHall, Cave, Arena, Hangar, CarpetedHallway,\n")
    print("    Hallway, StoneCorridor, Alley, Forest, City, Mountains,\n")
    print("    Quarry, Plain, ParkingLot, SewerPipe, Underwater\n")
    print("\n  Chorus functions:\n")
    print("    set_chorus_default(sid) — apply default chorus\n")
    print("    set_chorus(sid, config) — apply custom chorus via ma_chorus_config\n")
    print("    chorus_config_default() — get default config to customize\n")
}

[export]
def main {
    print("AUDIO-05: Reverb and Effects\n\n")
    with_audio_system() {
        example_dry()
        example_reverb_presets()
        example_chorus()
        example_combined()
        example_summary()
        print("\nDone!\n")
    }
}
