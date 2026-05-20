options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// Helper: query a pattern at cycle 0 and return the first event.
def private query_first(var pat : Pattern) : Event {
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let ev = haps[0].value
    delete haps
    return ev
}

// Helper: query all events in cycle 0.
def private query_all(var pat : Pattern) : array<Event> {
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    var events : array<Event>
    for (h in haps) {
        events |> push(h.value) // nolint:PERF006
    }
    delete haps
    return <- events
}

// ─── Formant data lookup ───

[test]
def test_vowel_a_formant_data(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("a", data))
    t |> equal(data.freqs[0], 660.0)
    t |> equal(data.freqs[1], 1120.0)
    t |> equal(data.gains[0], 1.0)
    t |> equal(data.qs[0], 80.0)
}

[test]
def test_vowel_e_formant_data(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("e", data))
    t |> equal(data.freqs[0], 440.0)
    t |> equal(data.freqs[1], 1800.0)
}

[test]
def test_vowel_i_formant_data(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("i", data))
    t |> equal(data.freqs[0], 270.0)
    t |> equal(data.freqs[1], 1850.0)
}

[test]
def test_vowel_o_formant_data(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("o", data))
    t |> equal(data.freqs[0], 430.0)
    t |> equal(data.freqs[1], 820.0)
}

[test]
def test_vowel_u_formant_data(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("u", data))
    t |> equal(data.freqs[0], 370.0)
    t |> equal(data.freqs[1], 630.0)
}

[test]
def test_vowel_unknown_returns_false(t : T?) {
    var data : FormantData
    t |> success(!vowel_get_formant_data("xyz", data))
}

[test]
def test_vowel_extended_ae(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("ae", data))
    t |> equal(data.freqs[0], 650.0)
}

[test]
def test_vowel_extended_aa(t : T?) {
    var data : FormantData
    t |> success(vowel_get_formant_data("aa", data))
    t |> equal(data.freqs[0], 560.0)
}

// ─── VowelFilter init ───

[test]
def test_vowel_filter_init_a(t : T?) {
    var vf = VowelFilter()
    vowel_filter_init(vf, "a")
    t |> success(vf.active)
    for (i in range(VOWEL_NUM_FORMANTS)) {
        t |> success(vf.formants[i].active != 0)
    }
}

[test]
def test_vowel_filter_init_unknown(t : T?) {
    var vf = VowelFilter()
    vowel_filter_init(vf, "xyz")
    t |> success(!vf.active)
}

[test]
def test_vowel_filter_init_empty(t : T?) {
    var vf = VowelFilter()
    vowel_filter_init(vf, "")
    t |> success(!vf.active)
}

// ─── FormantBiquad bandpass basic behavior ───

[test]
def test_bpf_passes_center_frequency(t : T?) {
    var bq = FormantBiquad()
    formant_biquad_setup_bpf(bq, 1000.0, 10.0, 48000.0)
    t |> success(bq.active != 0)
    let phaseInc = 1000.0 / 48000.0
    var phase = 0.0
    var maxAbs = 0.0
    for (_i in range(480)) {
        let inp = sin(phase * 3.14159265358979323846 * 2.0)
        let out = formant_biquad_tick(bq, float(inp))
        maxAbs = max(maxAbs, float(abs(out)))
        phase += phaseInc
    }
    t |> success(maxAbs > 0.1)
}

[test]
def test_bpf_rejects_distant_frequency(t : T?) {
    var bq = FormantBiquad()
    formant_biquad_setup_bpf(bq, 1000.0, 10.0, 48000.0)
    let phaseInc = 100.0 / 48000.0
    var phase = 0.0
    var maxAbs = 0.0
    for (i in range(4800)) {
        let inp = sin(phase * 3.14159265358979323846 * 2.0)
        let out = formant_biquad_tick(bq, float(inp))
        if (i > 2400) {
            maxAbs = max(maxAbs, float(abs(out)))
        }
        phase += phaseInc
    }
    t |> success(maxAbs < 0.05)
}

// ─── VowelFilter tick produces output ───

[test]
def test_vowel_filter_tick_produces_output(t : T?) {
    var vf = VowelFilter()
    vowel_filter_init(vf, "a")
    var seed = 12345u
    var maxAbs = 0.0
    for (_i in range(4800)) {
        let inp = noise(seed)
        let out = vowel_filter_tick(vf, inp)
        maxAbs = max(maxAbs, float(abs(out)))
    }
    t |> success(maxAbs > 0.1)
}

[test]
def test_different_vowels_different_output(t : T?) {
    var vf_a = VowelFilter()
    var vf_i = VowelFilter()
    vowel_filter_init(vf_a, "a")
    vowel_filter_init(vf_i, "i")
    var seed_a = 12345u
    var seed_i = 12345u
    var energy_a = 0.0
    var energy_i = 0.0
    for (_i in range(4800)) {
        let inp_a = noise(seed_a)
        let inp_i = noise(seed_i)
        let out_a = vowel_filter_tick(vf_a, inp_a)
        let out_i = vowel_filter_tick(vf_i, inp_i)
        energy_a += float(out_a * out_a)
        energy_i += float(out_i * out_i)
    }
    t |> success(energy_a > 0.0)
    t |> success(energy_i > 0.0)
    t |> success(abs(energy_a - energy_i) > 0.01)
}

// ─── Pattern-level set_vowel ───

[test]
def test_set_vowel_static(t : T?) {
    let pat <- atom("sawtooth") |> set_vowel("a")
    let ev = query_first(pat)
    t |> equal(ev.vowel, "a")
    t |> equal(ev.s, "sawtooth")
}

[test]
def test_set_vowel_static_u(t : T?) {
    let pat <- atom("sine") |> set_vowel("u")
    let ev = query_first(pat)
    t |> equal(ev.vowel, "u")
}

[test]
def test_set_vowel_pattern(t : T?) {
    let vowelPat <- fastcat([atom("a"), atom("e")])
    let pat <- atom("sawtooth") |> set_vowel(vowelPat)
    let ev = query_first(pat)
    t |> equal(ev.vowel, "a")
}

[test]
def test_set_vowel_with_s_mini(t : T?) {
    // s() mini-notation creates events with .s field
    let vowelPat <- s("a e i")
    let pat <- atom("sawtooth") |> set_vowel(vowelPat)
    let ev = query_first(pat)
    t |> equal(ev.vowel, "a")
}

[test]
def test_set_vowel_mini_notation(t : T?) {
    // set_vowel("<a e i o u>") detects mini-notation and parses it
    let pat <- atom("sawtooth") |> set_vowel("<a e i o u>")
    let ev = query_first(pat)
    t |> equal(ev.vowel, "a")
}

[test]
def test_vowel_default_empty(t : T?) {
    let pat <- atom("sine")
    let ev = query_first(pat)
    t |> equal(ev.vowel, "")
}

// ─── Extended vowels ───

[test]
def test_all_basic_vowels_init(t : T?) {
    var vowels = fixed_array("a", "e", "i", "o", "u")
    for (v in vowels) {
        var vf = VowelFilter()
        vowel_filter_init(vf, v)
        t |> success(vf.active)
    }
}

[test]
def test_extended_vowels_init(t : T?) {
    var vowels = fixed_array("ae", "aa", "oe", "ue", "y")
    for (v in vowels) {
        var vf = VowelFilter()
        vowel_filter_init(vf, v)
        t |> success(vf.active)
    }
}

[test]
def test_nasal_vowels_init(t : T?) {
    var vowels = fixed_array("uh", "un", "en", "an", "on")
    for (v in vowels) {
        var vf = VowelFilter()
        vowel_filter_init(vf, v)
        t |> success(vf.active)
    }
}

// ─── OscVoice vowel integration ───

[test]
def test_osc_voice_vowel_renders(t : T?) {
    var voice = OscVoice(
        osc_type = OscType.osc_sawtooth,
        freq = 220.0,
        duration = 0.5,
        attack = 0.001,
        decay = 0.05,
        sustain = 0.8,
        release_sec = 0.01,
        gain = 0.8,
        pan = 0.0
    )
    vowel_filter_init(voice.vowel_filter, "a")
    let chunkFrames = 480
    var output : array<float>
    output |> resize(chunkFrames * 2)
    var reverb_buf : array<float>
    var delay_buf : array<float>
    var chorus_buf : array<float>
    osc_render_chunk(voice, output, reverb_buf, delay_buf, chorus_buf, chunkFrames)
    var maxAbs = 0.0
    for (sv in output) {
        maxAbs = max(maxAbs, abs(sv))
    }
    t |> success(maxAbs > 0.01)
}

[test]
def test_osc_voice_vowel_vs_no_vowel(t : T?) {
    let chunkFrames = 2400
    var output_plain : array<float>
    var output_vowel : array<float>
    output_plain |> resize(chunkFrames * 2)
    output_vowel |> resize(chunkFrames * 2)
    var reverb_buf : array<float>
    var delay_buf : array<float>
    var chorus_buf : array<float>
    var voice_plain = OscVoice(
        osc_type = OscType.osc_sawtooth,
        freq = 220.0,
        duration = 0.5,
        gain = 0.8,
        pan = 0.0
    )
    osc_render_chunk(voice_plain, output_plain, reverb_buf, delay_buf, chorus_buf, chunkFrames)
    var voice_vowel = OscVoice(
        osc_type = OscType.osc_sawtooth,
        freq = 220.0,
        duration = 0.5,
        gain = 0.8,
        pan = 0.0
    )
    vowel_filter_init(voice_vowel.vowel_filter, "a")
    osc_render_chunk(voice_vowel, output_vowel, reverb_buf, delay_buf, chorus_buf, chunkFrames)
    var diff_energy = 0.0
    for (i in range(chunkFrames * 2)) {
        let d = output_vowel[i] - output_plain[i]
        diff_energy += d * d
    }
    t |> success(diff_energy > 0.01)
}
