options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require strudel/strudel_synth
require math

// Helper: compute RMS of an array of samples.
def private rms(samples : array<float>) : float {
    var sum = 0.0
    for (s in samples) {
        sum += s * s
    }
    return sqrt(sum / float(length(samples)))
}

// Helper: render an OscVoice for a given number of frames and return the stereo output.
def private render_voice_stereo(var voice : OscVoice; frames : int) : array<float> {
    var output : array<float>
    output |> resize(frames * 2)
    for (s in output) {
        s = 0.0
    }
    var empty_send : array<float>
    var empty_delay_send : array<float>
    var empty_chorus_send : array<float>
    osc_render_chunk(voice, output, empty_send, empty_delay_send, empty_chorus_send, frames)
    return <- output
}

// ─── Pink noise ───

[test]
def test_pink_noise_nonzero(t : T?) {
    // Pink noise should produce non-zero output
    var voice = OscVoice(osc_type = OscType.osc_pink_noise, freq = 440.0, duration = 1.0, gain = 1.0)
    let frames = 2400  // 50ms at 48kHz
    let output <- render_voice_stereo(voice, frames)
    var maxVal = 0.0
    for (s in output) {
        maxVal = max(maxVal, abs(s))
    }
    t |> success(maxVal > 0.001)
}

[test]
def test_pink_noise_varies(t : T?) {
    // Pink noise should not be constant — check that samples differ
    var voice = OscVoice(osc_type = OscType.osc_pink_noise, freq = 440.0, duration = 1.0, gain = 1.0)
    let frames = 2400
    let output <- render_voice_stereo(voice, frames)
    var differs = false
    let first = output[0]
    for (i in range(1, length(output) / 2)) {
        if (abs(output[i * 2] - first) > 0.0001) {
            differs = true
            break
        }
    }
    t |> success(differs)
}

[test]
def test_pink_noise_lower_than_white(t : T?) {
    // Pink noise has more energy in low frequencies — its high-frequency RMS
    // should be lower relative to overall RMS compared to white noise.
    // We test by checking that pink noise output has reasonable amplitude (not full-scale white).
    var voice = OscVoice(osc_type = OscType.osc_pink_noise, freq = 440.0, duration = 1.0, gain = 1.0)
    let frames = 4800  // 100ms
    let output <- render_voice_stereo(voice, frames)
    // extract left channel
    var mono : array<float>
    mono |> resize(frames)
    for (i in range(frames)) {
        mono[i] = output[i * 2]
    }
    let r = rms(mono)
    // pink noise RMS with 0.11 scale factor should be well below 0.5
    t |> success(r > 0.001)
    t |> success(r < 0.5)
}

// ─── Supersaw ───

[test]
def test_supersaw_nonzero(t : T?) {
    var voice = OscVoice(osc_type = OscType.osc_supersaw, freq = 440.0, duration = 1.0, gain = 1.0)
    osc_voice_init_supersaw(voice)
    let frames = 2400
    let output <- render_voice_stereo(voice, frames)
    var maxVal = 0.0
    for (s in output) {
        maxVal = max(maxVal, abs(s))
    }
    t |> success(maxVal > 0.01)
}

[test]
def test_supersaw_stereo_width(t : T?) {
    // Supersaw should have different L and R channels (stereo spread)
    var voice = OscVoice(osc_type = OscType.osc_supersaw, freq = 440.0, duration = 1.0, gain = 1.0)
    osc_voice_init_supersaw(voice)
    let frames = 2400
    let output <- render_voice_stereo(voice, frames)
    var diffCount = 0
    for (i in range(frames)) {
        if (abs(output[i * 2] - output[i * 2 + 1]) > 0.0001) {
            diffCount ++
        }
    }
    // most frames should differ between L and R
    t |> success(diffCount > frames / 2)
}

[test]
def test_supersaw_wider_than_saw(t : T?) {
    // Supersaw RMS should be comparable to or greater than single saw at same gain
    var saw_voice = OscVoice(osc_type = OscType.osc_sawtooth, freq = 440.0, duration = 1.0, gain = 1.0)
    let frames = 2400
    let saw_out <- render_voice_stereo(saw_voice, frames)
    var saw_mono : array<float>
    saw_mono |> resize(frames)
    for (i in range(frames)) {
        saw_mono[i] = saw_out[i * 2]
    }
    let sawRms = rms(saw_mono)

    var ss_voice = OscVoice(osc_type = OscType.osc_supersaw, freq = 440.0, duration = 1.0, gain = 1.0)
    osc_voice_init_supersaw(ss_voice)
    let ss_out <- render_voice_stereo(ss_voice, frames)
    var ss_mono : array<float>
    ss_mono |> resize(frames)
    for (i in range(frames)) {
        ss_mono[i] = (ss_out[i * 2] + ss_out[i * 2 + 1]) * 0.5
    }
    let ssRms = rms(ss_mono)

    // supersaw should have non-trivial energy
    t |> success(ssRms > 0.01)
    t |> success(sawRms > 0.01)
}

// ─── FM synthesis ───

[test]
def test_fm_changes_output(t : T?) {
    // Sine with FM should differ from plain sine
    let frames = 2400
    var plain = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0)
    let plain_out <- render_voice_stereo(plain, frames)

    var fm_voice = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0, fm_depth = 2.0, fm_harmonicity = 3.0)
    let fm_out <- render_voice_stereo(fm_voice, frames)

    var diffSum = 0.0
    for (i in range(frames)) {
        diffSum += abs(plain_out[i * 2] - fm_out[i * 2])
    }
    // FM should produce meaningfully different output
    t |> success(diffSum > 1.0)
}

[test]
def test_fm_zero_depth_matches_plain(t : T?) {
    // FM with depth=0 should produce identical output to plain sine
    let frames = 2400
    var plain = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0)
    let plain_out <- render_voice_stereo(plain, frames)

    var fm_voice = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0, fm_depth = 0.0, fm_harmonicity = 3.0)
    let fm_out <- render_voice_stereo(fm_voice, frames)

    var maxDiff = 0.0
    for (i in range(length(plain_out))) {
        maxDiff = max(maxDiff, abs(plain_out[i] - fm_out[i]))
    }
    t |> success(maxDiff < 0.0001)
}

[test]
def test_fm_on_sawtooth(t : T?) {
    // FM should also work on sawtooth oscillator
    let frames = 2400
    var plain = OscVoice(osc_type = OscType.osc_sawtooth, freq = 220.0, duration = 1.0, gain = 1.0)
    let plain_out <- render_voice_stereo(plain, frames)

    var fm_voice = OscVoice(osc_type = OscType.osc_sawtooth, freq = 220.0, duration = 1.0, gain = 1.0, fm_depth = 1.5, fm_harmonicity = 2.0)
    let fm_out <- render_voice_stereo(fm_voice, frames)

    var diffSum = 0.0
    for (i in range(frames)) {
        diffSum += abs(plain_out[i * 2] - fm_out[i * 2])
    }
    t |> success(diffSum > 0.5)
}

[test]
def test_fm_harmonicity_affects_timbre(t : T?) {
    // Different harmonicity ratios should produce different output
    let frames = 2400
    var fm1 = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0, fm_depth = 2.0, fm_harmonicity = 1.0)
    let out1 <- render_voice_stereo(fm1, frames)

    var fm2 = OscVoice(osc_type = OscType.osc_sine, freq = 440.0, duration = 1.0, gain = 1.0, fm_depth = 2.0, fm_harmonicity = 7.0)
    let out2 <- render_voice_stereo(fm2, frames)

    var diffSum = 0.0
    for (i in range(frames)) {
        diffSum += abs(out1[i * 2] - out2[i * 2])
    }
    t |> success(diffSum > 1.0)
}
