options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// ─── signal_sine ───

[test]
def test_signal_sine_midpoint(t : T?) {
    // At t=0.0, sin(0) = 0, mapped to 0.5
    let pat <- signal_sine()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(!haps[0].has_whole)
    t |> success(abs(haps[0].value.note - 0.5) < 0.01)
}

[test]
def test_signal_sine_peak(t : T?) {
    // At t=0.25, sin(PI/2) = 1, mapped to 1.0
    let pat <- signal_sine()
    let haps <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

[test]
def test_signal_sine_trough(t : T?) {
    // At t=0.75, sin(3PI/2) = -1, mapped to 0.0
    let pat <- signal_sine()
    let haps <- pat(TimeSpan(start = 0.75lf, stop = 0.751lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 0.0) < 0.01)
}

// ─── signal_cosine ───

[test]
def test_signal_cosine(t : T?) {
    // At t=0.0, cos(0) = 1, mapped to 1.0
    let pat <- signal_cosine()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

// ─── signal_saw ───

[test]
def test_signal_saw(t : T?) {
    let pat <- signal_saw()
    // At t=0.0 → 0.0
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps0[0].value.note - 0.0) < 0.01)
    // At t=0.5 → 0.5
    let haps1 <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps1[0].value.note - 0.5) < 0.01)
    // At t=1.25 → 0.25
    let haps2 <- pat(TimeSpan(start = 1.25lf, stop = 1.251lf))
    t |> success(abs(haps2[0].value.note - 0.25) < 0.01)
}

// ─── signal_tri ───

[test]
def test_signal_tri(t : T?) {
    let pat <- signal_tri()
    // At t=0.0 → 0.0
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps0[0].value.note - 0.0) < 0.01)
    // At t=0.25 → 0.5 (rising)
    let haps1 <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> success(abs(haps1[0].value.note - 0.5) < 0.01)
    // At t=0.5 → 1.0 (peak)
    let haps2 <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps2[0].value.note - 1.0) < 0.01)
    // At t=0.75 → 0.5 (falling)
    let haps3 <- pat(TimeSpan(start = 0.75lf, stop = 0.751lf))
    t |> success(abs(haps3[0].value.note - 0.5) < 0.01)
}

// ─── signal_perlin ───

[test]
def test_signal_perlin_deterministic(t : T?) {
    // Same query twice → same value
    let pat <- signal_perlin()
    let haps1 <- pat(TimeSpan(start = 0.37lf, stop = 0.371lf))
    let haps2 <- pat(TimeSpan(start = 0.37lf, stop = 0.371lf))
    t |> equal(length(haps1), 1)
    t |> equal(length(haps2), 1)
    t |> success(abs(haps1[0].value.note - haps2[0].value.note) < 0.001)
}

[test]
def test_signal_perlin_range(t : T?) {
    // All values should be in [0, 1]
    let pat <- signal_perlin()
    for (i in range(100)) {
        let tt = double(i) * 0.037lf
        let haps <- pat(TimeSpan(start = tt, stop = tt + 0.001lf))
        t |> success(haps[0].value.note >= 0.0)
        t |> success(haps[0].value.note <= 1.0)
    }
}

// ─── signal_range ───

[test]
def test_signal_range(t : T?) {
    // sine at t=0.25 is 1.0 → range(200, 4000) should give 4000
    let pat <- signal_sine() |> signal_range(200.0, 4000.0)
    let haps <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 4000.0) < 10.0)
    // sine at t=0.75 is 0.0 → range(200, 4000) should give 200
    let haps2 <- pat(TimeSpan(start = 0.75lf, stop = 0.751lf))
    t |> success(abs(haps2[0].value.note - 200.0) < 10.0)
}

// ─── signal_slow ───

[test]
def test_signal_slow(t : T?) {
    // slow(4) sine: at t=1.0 (which is 0.25 in signal space) → 1.0
    let pat <- signal_sine() |> slow(4.0lf)
    let haps <- pat(TimeSpan(start = 1.0lf, stop = 1.001lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

// ─── n() numeric pattern ───

[test]
def test_n_alternating(t : T?) {
    // n("<2 16>") alternates note=2.0 on cycle 0, note=16.0 on cycle 1
    let pat <- n("<2 16>")
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> success(abs(haps0[0].value.note - 2.0) < 0.01)
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> success(abs(haps1[0].value.note - 16.0) < 0.01)
    // cycle 2 wraps back to 2.0
    let haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 1)
    t |> success(abs(haps2[0].value.note - 2.0) < 0.01)
}

// ─── innerJoin ───

[test]
def test_innerJoin_basic(t : T?) {
    // innerJoin with a parameter pattern that alternates 1.0 and 2.0,
    // and a fn that creates fast(atom("bd"), val).
    // Cycle 0: val=1.0 → fast(1) = 1 hap. Cycle 1: val=2.0 → fast(2) = 2 haps.
    let param_pat <- n("<1 2>")
    var base <- atom("bd")
    let pat <- innerJoin(param_pat, @capture(<- base) (val : float) : Pattern {
        return fast(base, double(val))
    })
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 2)
}

// ─── patternified slow/fast ───

[test]
def test_slow_patternified(t : T?) {
    // slow(pat, n("<1 2>")): cycle 0 → slow(1)=normal, cycle 1 → slow(2)=half speed
    // Base: s("a b") = 2 events per cycle at normal speed
    // Cycle 0, slow(1): 2 events
    // Cycle 1, slow(2): events stretched, only 1 fits in the cycle
    let pat <- slow(s("a b"), n("<1 2>"))
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 2)
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
}

[test]
def test_fast_patternified(t : T?) {
    // fast(pat, n("<1 2>")): cycle 0 → fast(1)=normal, cycle 1 → fast(2)=double
    // Base: atom("bd") = 1 event per cycle
    // Cycle 0, fast(1): 1 event
    // Cycle 1, fast(2): 2 events
    let pat <- fast(atom("bd"), n("<1 2>"))
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 2)
}

[test]
def test_slow_patternified_signal(t : T?) {
    // The actual use case: slow a signal with alternating factors
    // slow(signal_saw(), n("<1 2>"))
    // Cycle 0, slow(1): saw at t=0.0 → 0.0
    // Cycle 1, slow(2): saw at t=1.0, but slow(2) means time/2 → saw(0.5) → 0.5
    let pat <- slow(signal_saw(), n("<1 2>"))
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps0), 1)
    t |> success(abs(haps0[0].value.note - 0.0) < 0.01)  // saw(0) = 0
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 1.001lf))
    t |> equal(length(haps1), 1)
    // slow(2) at t=1.0: inner time = 1.0 * (1/2) = 0.5, saw(0.5) = 0.5
    t |> success(abs(haps1[0].value.note - 0.5) < 0.01)
}

// ─── combineWith ───

[test]
def test_combine_with_gain(t : T?) {
    // Base pattern atom("bd"), combine with sine signal to set gain.
    // At cycle 0.0, sine=0.5 → gain=0.5
    let base <- atom("bd")
    let sig <- signal_sine()
    let pat <- combineWith(base, sig, @(e : Event; val : float) : Event {
        var r = e; r.gain = val; return r
    })
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.gain - 0.5) < 0.01)
    t |> equal(haps[0].value.s, "bd")
}

[test]
def test_combine_with_varies_by_cycle(t : T?) {
    // Two events per cycle (fastcat), signal_saw modulates gain.
    // Event at 0.0 → gain=0.0, event at 0.5 → gain=0.5
    let base <- s("bd sd")
    let sig <- signal_saw()
    let pat <- combineWith(base, sig, @(e : Event; val : float) : Event {
        var r = e; r.gain = val; return r
    })
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> success(abs(haps[0].value.gain - 0.0) < 0.01)
    t |> success(abs(haps[1].value.gain - 0.5) < 0.01)
}

// ─── Pattern-valued setters ───

[test]
def test_set_gain_pattern(t : T?) {
    // set_gain with a signal pattern
    let pat <- atom("bd") |> set_gain(signal_sine())
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    // At onset 0.0, sine=0.5
    t |> success(abs(haps[0].value.gain - 0.5) < 0.01)
}

[test]
def test_set_lpf_pattern(t : T?) {
    // set_lpf with a ranged signal
    let pat <- atom("bd") |> set_lpf(signal_sine() |> signal_range(200.0, 4000.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    // At onset 0.0, sine=0.5 → range maps to 2100
    t |> success(abs(haps[0].value.lpf - 2100.0) < 50.0)
}

[test]
def test_set_pan_pattern(t : T?) {
    let pat <- atom("bd") |> set_pan(signal_saw())
    // Query full cycles. pure() whole starts at cycle boundary.
    // Cycle 0: onset=0.0, saw=0.0. Cycle 1: onset=1.0, saw=0.0.
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.pan - 0.0) < 0.01)
    // Cycle with offset: use fast(2) so second event onsets at 0.5
    let pat2 <- s("bd sd") |> set_pan(signal_saw())
    let haps2 <- pat2(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps2), 2)
    t |> success(abs(haps2[0].value.pan - 0.0) < 0.01)
    t |> success(abs(haps2[1].value.pan - 0.5) < 0.01)
}
