options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// ─── rev ───

[test]
def test_rev(t : T?) {
    let pat <- s("bd sd hh cp") |> rev()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    // rev flips positions: bd(0.0-0.25) → bd(0.75-1.0), etc.
    // array order matches input order, positions are reversed
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
    // bd now at 0.75-1.0, cp now at 0.0-0.25
    t |> success(abs(haps[0].whole.start - 0.75lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 1.0lf) < 0.001lf)
    t |> success(abs(haps[3].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[3].whole.stop - 0.25lf) < 0.001lf)
}

[test]
def test_rev_double(t : T?) {
    // rev(rev(pat)) should give back the original
    let pat <- s("bd sd") |> rev() |> rev()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.5lf) < 0.001lf)
}

// ─── jux ───

[test]
def test_jux(t : T?) {
    let pat <- jux(s("bd sd"), @(x) => rev(x))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    var left_count = 0
    var right_count = 0
    for (h in haps) {
        if (abs(h.value.pan - 0.0) < 0.01) {
            left_count ++
        } elif (abs(h.value.pan - 1.0) < 0.01) {
            right_count ++
        }
    }
    t |> equal(left_count, 2)
    t |> equal(right_count, 2)
}

// ─── off ───

[test]
def test_off(t : T?) {
    let pat <- off(s("bd"), 0.25lf, @(x) => transpose(x, 12.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // 1 original (note=60) + 2 shifted parts (note=72, split at cycle boundary)
    t |> equal(length(haps), 3)
    var orig_count = 0
    var shifted_count = 0
    for (h in haps) {
        if (abs(h.value.note - 60.0) < 0.01) {
            orig_count ++
        } elif (abs(h.value.note - 72.0) < 0.01) {
            shifted_count ++
        }
    }
    t |> equal(orig_count, 1)
    t |> equal(shifted_count, 2)
}

// ─── sometimesby ───

[test]
def test_sometimesby(t : T?) {
    let pat_never <- sometimesby(s("bd"), 0.0, @(p) => fmap(p, @(e : Event) : Event { var r = e; r.s = "sd"; return r; }))
    let pat_always <- sometimesby(s("bd"), 1.0, @(p) => fmap(p, @(e : Event) : Event { var r = e; r.s = "sd"; return r; }))
    for (cyc in range(5)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps_never <- pat_never(span)
        t |> equal(length(haps_never), 1)
        t |> equal(haps_never[0].value.s, "bd")
        let haps_always <- pat_always(span)
        t |> equal(length(haps_always), 1)
        t |> equal(haps_always[0].value.s, "sd")
    }
}

[test]
def test_sometimes_distribution(t : T?) {
    let pat <- sometimes(s("bd sd hh cp"), @(p) => fast(p, 2.0lf))
    var total_haps = 0
    let cycles = 100
    for (cyc in range(cycles)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps <- pat(span)
        total_haps += length(haps)
    }
    // 4 events/cycle normal, 8 events/cycle fast. With 50% per-event,
    // expect mix: some cycles have 4-8 events. Average ~6.
    // Total should be between 400 (all normal) and 800 (all fast)
    t |> success(total_haps > 400)
    t |> success(total_haps < 800)
}

// ─── transpose ───

[test]
def test_transpose(t : T?) {
    let pat <- atom("sine") |> set_note(60.0) |> transpose(7.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 67.0) < 0.001)
}

[test]
def test_transpose_negative(t : T?) {
    let pat <- atom("sine") |> set_note(60.0) |> transpose(-12.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.note - 48.0) < 0.001)
}

// ─── weighted_fastcat ───

[test]
def test_weighted_fastcat(t : T?) {
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    var weights : array<float>
    weights |> push(3.0)
    weights |> push(1.0)
    let pat <- weighted_fastcat(pats, weights)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 1.0lf) < 0.001lf)
}

// ─── mini-notation: elongation (_) ───

[test]
def test_elongation(t : T?) {
    // "bd _ _ sd" → bd gets weight 3, sd gets weight 1
    let pat <- s("bd _ _ sd")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 1.0lf) < 0.001lf)
}

[test]
def test_elongation_single(t : T?) {
    // "bd _" → bd with weight 2, only one event spanning full cycle
    let pat <- s("bd _")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
}

// ─── mini-notation: weight (@) ───

[test]
def test_weight(t : T?) {
    // "bd@3 sd" → bd gets weight 3, sd gets weight 1
    let pat <- s("bd@3 sd")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.75lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 1.0lf) < 0.001lf)
}

// ─── lpq / hpq fields ───

[test]
def test_lpq_hpq_defaults(t : T?) {
    let pat <- atom("bd")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.lpq - 1.0) < 0.001)
    t |> success(abs(haps[0].value.hpq - 1.0) < 0.001)
}

[test]
def test_set_lpq(t : T?) {
    let pat <- atom("bd") |> set_lpq(5.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.lpq - 5.0) < 0.001)
}

[test]
def test_set_hpq(t : T?) {
    let pat <- atom("bd") |> set_hpq(3.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.hpq - 3.0) < 0.001)
}

// ─── new setters ───

[test]
def test_set_lpf(t : T?) {
    let pat <- atom("bd") |> set_lpf(1000.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.lpf - 1000.0) < 0.001)
}

[test]
def test_set_hpf(t : T?) {
    let pat <- atom("bd") |> set_hpf(200.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.hpf - 200.0) < 0.001)
}

[test]
def test_set_attack(t : T?) {
    let pat <- atom("bd") |> set_attack(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.attack - 0.5) < 0.001)
}

[test]
def test_set_release(t : T?) {
    let pat <- atom("bd") |> set_release(2.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.release - 2.0) < 0.001)
}
