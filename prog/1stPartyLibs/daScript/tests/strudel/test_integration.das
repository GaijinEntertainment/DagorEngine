options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// Helper: query a pattern at a given cycle and return all events.
def private query_cycle(var pat : Pattern; cycle : int = 0) : array<Event> {
    let start = double(cycle)
    var haps <- pat(TimeSpan(start = start, stop = start + 1.0lf))
    var events : array<Event>
    for (h in haps) {
        events |> push(h.value) // nolint:PERF006
    }
    delete haps
    return <- events
}

// Helper: query multiple cycles and return all events.
def private query_cycles(var pat : Pattern; num_cycles : int) : array<Event> {
    var haps <- pat(TimeSpan(start = 0.0lf, stop = double(num_cycles)))
    var events : array<Event>
    for (h in haps) {
        events |> push(h.value) // nolint:PERF006
    }
    delete haps
    return <- events
}

// ─── set_s (patternified set_sound) ───

[test]
def test_set_s_static(t : T?) {
    let pat <- note_pattern("c3", "sine") |> set_s("supersaw")
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 1)
    t |> equal(events[0].s, "supersaw")
}

[test]
def test_set_s_mini_notation(t : T?) {
    let pat <- note_pattern("c3", "sine") |> set_s("<supersaw sawtooth>")
    // Cycle 0 → supersaw
    var inscope ev0 <- query_cycle(pat, 0)
    t |> equal(ev0[0].s, "supersaw")
    // Cycle 1 → sawtooth
    var inscope ev1 <- query_cycle(pat, 1)
    t |> equal(ev1[0].s, "sawtooth")
}

[test]
def test_set_sound_pattern(t : T?) {
    let sound_pat <- s("<triangle square>")
    let pat <- note_pattern("c3 e3", "sine") |> set_sound(sound_pat)
    // Cycle 0 → both notes get "triangle"
    var inscope ev0 <- query_cycle(pat, 0)
    t |> equal(length(ev0), 2)
    t |> equal(ev0[0].s, "triangle")
    t |> equal(ev0[1].s, "triangle")
    // Cycle 1 → both notes get "square"
    var inscope ev1 <- query_cycle(pat, 1)
    t |> equal(ev1[0].s, "square")
    t |> equal(ev1[1].s, "square")
}

// ─── Signal + filter integration ───

[test]
def test_signal_lpf_on_chord(t : T?) {
    let lpfSig <- signal_sine() |> signal_range(200.0, 2000.0) |> slow(4.0lf)
    let pat <- note_pattern("c3 e3 g3", "sawtooth") |> set_lpf(lpfSig) |> set_room(0.7)
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 3)
    for (ev in events) {
        t |> equal(ev.s, "sawtooth")
        t |> success(ev.lpf > 0.0)
        t |> success(ev.room > 0.0)
    }
}

// ─── FM + vowel + reverb integration ───

[test]
def test_fm_vowel_reverb(t : T?) {
    let vowelPat <- s("<a e i>") |> slow(12.0lf)
    let pat <- note_pattern("<g3 c4>", "sine") |> set_fm(2.0) |> set_fmh(3.0) |> set_vowel(vowelPat) |> set_room(1.0) |> set_roomsize(10.0)
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 1)
    t |> success(abs(events[0].fm - 2.0) < 0.01)
    t |> success(abs(events[0].fmh - 3.0) < 0.01)
    t |> equal(events[0].vowel, "a")
    t |> success(abs(events[0].room - 1.0) < 0.01)
    t |> success(abs(events[0].roomsize - 10.0) < 0.01)
}

// ─── Scale + delay + pan integration ───

[test]
def test_scale_delay_pan(t : T?) {
    let gainSig <- signal_perlin() |> signal_range(0.03, 0.12) |> slow(10.0lf)
    let panSig <- signal_sine() |> signal_range(0.15, 0.85) |> slow(7.0lf)
    let pat <- scale_pattern("0 2 4", "C4:pentatonic", "triangle") |> set_gain(gainSig) |> set_pan(panSig) |> set_delay(0.55) |> set_delaytime(0.5) |> set_delayfeedback(0.65)
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 3)
    // Verify scale conversion happened (minor pentatonic: [0,3,5,7,10])
    // degree 0 → C4=60, degree 2 → F4=65, degree 4 → Bb4=70
    t |> success(abs(events[0].note - 60.0) < 0.01)
    t |> success(abs(events[1].note - 65.0) < 0.01)
    t |> success(abs(events[2].note - 70.0) < 0.01)
    // Verify delay params
    for (ev in events) {
        t |> equal(ev.s, "triangle")
        t |> success(abs(ev.delay_amount - 0.55) < 0.01)
        t |> success(abs(ev.delaytime - 0.5) < 0.01)
        t |> success(abs(ev.delayfeedback - 0.65) < 0.01)
        t |> success(ev.gain >= 0.03 && ev.gain <= 0.12)
        t |> success(ev.pan >= 0.15 && ev.pan <= 0.85)
    }
}

// ─── Supersaw pad integration ───

[test]
def test_supersaw_pad(t : T?) {
    let lpfSig <- signal_sine() |> signal_range(250.0, 800.0) |> slow(32.0lf)
    let panSig <- signal_sine() |> signal_range(0.3, 0.7) |> slow(20.0lf)
    let pat <- note_pattern("<c2 eb2>", "supersaw") |> slow(24.0lf) |> set_gain(0.1) |> set_lpf(lpfSig) |> set_hpf(80.0) |> set_room(0.9) |> set_pan(panSig) |> set_orbit(2)
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 1)
    t |> equal(events[0].s, "supersaw")
    t |> success(events[0].lpf >= 250.0 && events[0].lpf <= 800.0)
    t |> success(abs(events[0].hpf - 80.0) < 0.01)
    t |> success(abs(events[0].room - 0.9) < 0.01)
    t |> equal(events[0].orbit, 2)
}

// ─── Noise bed integration ───

[test]
def test_noise_bed(t : T?) {
    let lpfSig <- signal_sine() |> signal_range(120.0, 350.0) |> slow(40.0lf)
    let pat <- atom("pink") |> set_gain(0.03) |> set_lpf(lpfSig) |> set_hpf(40.0) |> set_room(0.5) |> set_roomsize(4.0) |> set_orbit(5)
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 1)
    t |> equal(events[0].s, "pink")
    t |> success(abs(events[0].gain - 0.03) < 0.001)
    t |> success(events[0].lpf >= 120.0 && events[0].lpf <= 350.0)
    t |> success(abs(events[0].hpf - 40.0) < 0.01)
    t |> equal(events[0].orbit, 5)
}

// ─── Full ambient stack ───

[test]
def test_full_ambient_layer_count(t : T?) {
    // Build all 7 layers exactly as in integration_full_ambient.das
    var layer1 <- note_pattern("<c1 g1 f1 eb1 bb0 f1>", "sine") |> slow(32.0lf) |> set_gain(0.3) |> set_attack(6.0) |> set_release(8.0) |> set_lpf(180.0) |> set_room(1.0) |> set_roomsize(10.0) |> set_orbit(1)
    let lpf2 <- signal_sine() |> signal_range(250.0, 800.0) |> slow(32.0lf)
    let pan2 <- signal_sine() |> signal_range(0.3, 0.7) |> slow(20.0lf)
    var layer2 <- note_pattern("<c2 eb2 g2 bb2 f2 ab2 g2 eb2>", "supersaw") |> slow(24.0lf) |> set_gain(0.1) |> set_lpf(lpf2) |> set_hpf(80.0) |> set_room(0.9) |> set_roomsize(8.0) |> set_pan(pan2) |> set_orbit(2)
    let vowel3 <- s("<a e i o u e>") |> slow(12.0lf)
    var layer3 <- note_pattern("<g3 c4 eb4 f4 bb3 g3 ab3 c4>", "sine") |> set_fm(2.0) |> set_fmh(3.0) |> slow(28.0lf) |> set_gain(0.08) |> set_vowel(vowel3) |> set_lpf(1200.0) |> set_room(1.0) |> set_delay(0.3) |> set_orbit(3)
    let gain4 <- signal_perlin() |> signal_range(0.03, 0.12) |> slow(10.0lf)
    var layer4 <- scale_pattern("0 ~ ~ 4 ~ 2 ~ ~ 6 ~ 3 ~", "C4:pentatonic", "triangle") |> slow(6.0lf) |> set_gain(gain4) |> set_delay(0.55) |> set_orbit(4)
    let gain5 <- signal_perlin() |> signal_range(0.02, 0.08) |> slow(14.0lf)
    var layer5 <- scale_pattern("~ 5 ~ ~ 1 ~ ~ ~ 3 ~ ~ 7", "C5:pentatonic", "sine") |> slow(8.0lf) |> set_gain(gain5) |> set_delay(0.4) |> set_orbit(4)
    let lpf6 <- signal_sine() |> signal_range(120.0, 350.0) |> slow(40.0lf)
    var layer6 <- atom("pink") |> set_gain(0.03) |> set_lpf(lpf6) |> set_hpf(40.0) |> set_orbit(5)
    let gain7 <- signal_sine() |> signal_range(0.0, 0.15) |> slow(16.0lf)
    var layer7 <- note_pattern("<c1 c1 f0 f0>", "sine") |> slow(48.0lf) |> set_gain(gain7) |> set_lpf(100.0) |> set_orbit(1)
    let pat <- stack([layer1, layer2, layer3, layer4, layer5, layer6, layer7])
    // Query cycle 0 — should get events from all 7 layers
    var inscope events <- query_cycle(pat)
    t |> success(length(events) >= 7)
}

[test]
def test_full_ambient_layer_sounds(t : T?) {
    // Quick build — just verify each layer produces the right sound name
    var layer1 <- note_pattern("<c1>", "sine") |> set_orbit(1)
    var layer2 <- note_pattern("<c2>", "supersaw") |> set_orbit(2)
    var layer3 <- note_pattern("<g3>", "sine") |> set_fm(2.0) |> set_orbit(3)
    var layer4 <- scale_pattern("0", "C4:pentatonic", "triangle") |> set_orbit(4)
    var layer5 <- scale_pattern("5", "C5:pentatonic", "sine") |> set_orbit(4)
    var layer6 <- atom("pink") |> set_orbit(5)
    var layer7 <- note_pattern("<c1>", "sine") |> set_orbit(1)
    let pat <- stack([layer1, layer2, layer3, layer4, layer5, layer6, layer7])
    var inscope events <- query_cycle(pat)
    t |> equal(length(events), 7)
    // Verify sounds
    t |> equal(events[0].s, "sine")       // layer 1
    t |> equal(events[1].s, "supersaw")   // layer 2
    t |> equal(events[2].s, "sine")       // layer 3 (FM)
    t |> equal(events[3].s, "triangle")   // layer 4
    t |> equal(events[4].s, "sine")       // layer 5
    t |> equal(events[5].s, "pink")       // layer 6
    t |> equal(events[6].s, "sine")       // layer 7
}

[test]
def test_full_ambient_orbits(t : T?) {
    var layer1 <- note_pattern("<c1>", "sine") |> set_orbit(1)
    var layer2 <- note_pattern("<c2>", "supersaw") |> set_orbit(2)
    var layer3 <- note_pattern("<g3>", "sine") |> set_orbit(3)
    var layer4 <- scale_pattern("0", "C4:pentatonic", "triangle") |> set_orbit(4)
    var layer5 <- scale_pattern("5", "C5:pentatonic", "sine") |> set_orbit(4)
    var layer6 <- atom("pink") |> set_orbit(5)
    var layer7 <- note_pattern("<c1>", "sine") |> set_orbit(1)
    let pat <- stack([layer1, layer2, layer3, layer4, layer5, layer6, layer7])
    var inscope events <- query_cycle(pat)
    t |> equal(events[0].orbit, 1)  // layer 1
    t |> equal(events[1].orbit, 2)  // layer 2
    t |> equal(events[2].orbit, 3)  // layer 3
    t |> equal(events[3].orbit, 4)  // layer 4
    t |> equal(events[4].orbit, 4)  // layer 5 (same orbit as 4)
    t |> equal(events[5].orbit, 5)  // layer 6
    t |> equal(events[6].orbit, 1)  // layer 7 (same orbit as 1)
}

// Memory regression for integration patterns is tested via feature examples
// with --track-memory --duration 10 (real audio playback in main-thread mode).
// Pattern-only queries grow the allocator page pool which is expected behavior.
