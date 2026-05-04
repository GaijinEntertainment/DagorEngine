// Tutorial AUDIO-04: 3D Spatial Audio
//
// This tutorial covers:
//   - Setting the listener position with set_head_position
//   - Playing 3D positional sounds with play_3d_sound_loop_from_pcm
//   - Moving sounds in space with set_position
//   - Attenuation models: inverse, linear, quadratic, inverse_square
//   - Doppler effect from moving sources
//
// Run: daslang.exe tutorials/dasAudio/04_spatial_audio.das
//
// Best experienced with headphones for spatial positioning.

options gen2
options indenting = 4

require audio/audio_boost
require daslib/fio
require math

// ──────────────────────────────────────────────────────────────────────────
// Helper — generate a short repeating click/noise burst
// ──────────────────────────────────────────────────────────────────────────
//
// For spatial audio, short impulsive sounds (clicks, noise bursts) are
// easier to localize than pure tones. We generate 0.25 seconds of a
// filtered noise burst: a sine wave amplitude-modulated by a fast envelope.

def generate_click_burst() : array<float> {
    let num_samples = MA_SAMPLE_RATE / 4  // 0.25 seconds
    var samples : array<float>
    samples |> resize(num_samples)
    for (x in range(num_samples)) {
        // Create repeating clicks: 8 clicks per 0.25 seconds
        let t = float(x) / float(MA_SAMPLE_RATE)
        let click_freq = 32.0       // 32 Hz envelope = 8 clicks in 0.25s
        let carrier_freq = 880.0    // 880 Hz carrier tone
        let envelope = max(sin(2.0 * PI * click_freq * t), 0.0)
        samples[x] = sin(2.0 * PI * carrier_freq * t) * envelope * envelope
    }
    return <- samples
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Listener and a static 3D source
// ──────────────────────────────────────────────────────────────────────────
//
// set_head_position(pos, dir, vel) sets the listener's position, facing
// direction, and velocity (for Doppler). Here we place the listener at
// the origin facing forward (+Y), and a sound source to the right (+X).
//
// play_3d_sound_loop_from_pcm places a looping sound at a 3D position
// with a specified attenuation model that controls how volume falls off
// with distance.

def example_static_source() : uint64 {
    print("=== Section 1: Listener and static 3D source ===\n")

    // Listener at origin, facing forward (+Y), stationary
    set_head_position(float3(0, 0, 0), float3(0, 1, 0), float3(0, 0, 0))
    print("  Listener at origin, facing +Y\n")

    // Sound source to the right at distance 5
    var samples <- generate_click_burst()
    let sid = play_3d_sound_loop_from_pcm(
        float3(5, 0, 0),
        linear_attenuation(20.0),
        MA_SAMPLE_RATE, 1,
        samples
    )
    print("  Sound at (5,0,0) with linear attenuation (range 20)\n")
    print("  You should hear it on the RIGHT, SID = {sid}\n")
    sleep(2000u)
    return sid
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Moving a sound in a circle around the listener
// ──────────────────────────────────────────────────────────────────────────
//
// set_position(sid, pos, vel) updates the position and velocity of a
// 3D sound. By calling it in a loop we can orbit the sound around the
// listener. The velocity vector enables the Doppler effect.

def example_moving_source(sid : uint64) {
    print("\n=== Section 2: Sound orbiting the listener ===\n")
    print("  Moving in a circle (radius 5) over 5 seconds...\n")

    let radius = 5.0
    let num_steps = 100
    let step_time_ms = 50u  // 50ms per step = 5 seconds total

    for (i in range(num_steps)) {
        let angle = 2.0 * PI * float(i) / float(num_steps)
        let x = radius * cos(angle)
        let y = radius * sin(angle)
        let pos = float3(x, y, 0)

        // Velocity: tangential direction for Doppler effect
        let vx = -radius * sin(angle) * 2.0 * PI / 5.0
        let vy =  radius * cos(angle) * 2.0 * PI / 5.0
        let vel = float3(vx, vy, 0)

        set_position(sid, pos, vel)
        sleep(step_time_ms)
    }

    // Return to a fixed position
    set_position(sid, float3(5, 0, 0), float3(0, 0, 0))
    print("  Orbit complete\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Comparing attenuation models
// ──────────────────────────────────────────────────────────────────────────
//
// Different attenuation models control how volume decreases with distance:
//   - inverse_distance: 1/d falloff (natural, like real sound)
//   - linear:           straight line from full to zero at max distance
//   - quadratic:        d^2 falloff (faster than inverse)
//   - inverse_square:   1/d^2 falloff (physically accurate)
//
// We place four sounds at the same position, then move them to increasing
// distances to hear how each model differs.

def example_attenuation_models() {
    print("\n=== Section 3: Attenuation model comparison ===\n")

    let max_dist = 30.0

    // Generate one burst, clone for each source
    var samples_a <- generate_click_burst()
    var samples_b <- clone(samples_a)
    var samples_c <- clone(samples_a)
    var samples_d <- clone(samples_a)

    // Start all 4 sounds at the same nearby position
    let start_pos = float3(2, 0, 0)

    let sid_inv = play_3d_sound_loop_from_pcm(start_pos,
        inverse_distance_attenuation(max_dist), MA_SAMPLE_RATE, 1, samples_a)
    print("  [1] Inverse distance attenuation, SID = {sid_inv}\n")

    let sid_lin = play_3d_sound_loop_from_pcm(start_pos,
        linear_attenuation(max_dist), MA_SAMPLE_RATE, 1, samples_b)
    print("  [2] Linear attenuation, SID = {sid_lin}\n")

    let sid_quad = play_3d_sound_loop_from_pcm(start_pos,
        quadratic_attenuation(max_dist), MA_SAMPLE_RATE, 1, samples_c)
    print("  [3] Quadratic attenuation, SID = {sid_quad}\n")

    let sid_isq = play_3d_sound_loop_from_pcm(start_pos,
        inverse_square_attenuation(max_dist), MA_SAMPLE_RATE, 1, samples_d)
    print("  [4] Inverse square attenuation, SID = {sid_isq}\n")

    print("\n  Moving each to a different distance...\n")

    // Spread them out at different distances along X axis
    let distances = fixed_array(5.0, 10.0, 15.0, 20.0)
    let sids = fixed_array(sid_inv, sid_lin, sid_quad, sid_isq)
    let names = fixed_array("Inverse", "Linear", "Quadratic", "Inv.Square")

    for (i in range(4)) {
        let d = distances[i]
        set_position(sids[i], float3(d, 0, 0), float3(0, 0, 0))
        print("  {names[i]} now at distance {d}m\n")
    }

    print("  Listen for volume differences at varying distances\n")
    sleep(3000u)

    // Stop all
    for (i in range(4)) {
        stop(sids[i], 0.0)
    }
    sleep(200u)
}

[export]
def main() {
    print("AUDIO-04: 3D Spatial Audio\n")
    print("(Best with headphones)\n\n")
    with_audio_system() {
        let sid = example_static_source()
        example_moving_source(sid)
        stop(sid, 0.5)
        sleep(600u)
        example_attenuation_models()
        print("\nDone!\n")
    }
}
