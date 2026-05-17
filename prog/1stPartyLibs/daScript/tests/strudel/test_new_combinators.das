options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// ─── palindrome ───

[test]
def test_palindrome_even_cycle(t : T?) {
    // On even cycle (0), palindrome plays forward
    let pat <- palindrome(s("bd sd hh cp"))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
    // Forward: bd at 0.0-0.25
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.25lf) < 0.001lf)
}

[test]
def test_palindrome_odd_cycle(t : T?) {
    // On odd cycle (1), palindrome plays reversed
    let pat <- palindrome(s("bd sd hh cp"))
    let haps <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps), 4)
    // Reversed: bd(was 0.0-0.25) now at 0.75-1.0 (relative to cycle)
    t |> equal(haps[0].value.s, "bd")
    t |> success(abs(haps[0].whole.start - 1.75lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 2.0lf) < 0.001lf)
    // cp(was 0.75-1.0) now at 0.0-0.25 (relative to cycle)
    t |> equal(haps[3].value.s, "cp")
    t |> success(abs(haps[3].whole.start - 1.0lf) < 0.001lf)
    t |> success(abs(haps[3].whole.stop - 1.25lf) < 0.001lf)
}

// ─── superimpose ───

[test]
def test_superimpose(t : T?) {
    // Stack original + reversed copy
    let pat <- superimpose(atom("bd"), @(p) => rev(p))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // atom produces 1 event, rev(atom) produces 1 event, stacked = 2
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "bd")
}

[test]
def test_superimpose_transpose(t : T?) {
    // superimpose with transpose: original + transposed copy
    let pat <- superimpose(atom("sine") |> set_note(60.0), @(p) => transpose(p, 7.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    var has_60 = false
    var has_67 = false
    for (h in haps) {
        if (abs(h.value.note - 60.0) < 0.01) {
            has_60 = true
        } elif (abs(h.value.note - 67.0) < 0.01) {
            has_67 = true
        }
    }
    t |> success(has_60)
    t |> success(has_67)
}

// ─── echo ───

[test]
def test_echo_single(t : T?) {
    // echo with times=1 produces just the original (no delayed copies)
    let pat <- echo(atom("bd"), 1, 0.25lf, 0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.gain - 0.8) < 0.001)
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
}

[test]
def test_echo_forward_decay(t : T?) {
    // echoes must play LATER than the original, with decreasing gain
    let pat <- echo(atom("bd"), 4, 0.125lf, 0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // Keep onsets within this cycle (earlier cycles' echoes may trail in but
    // their whole.start is negative — scheduler ignores those)
    var starts : array<double>
    var gains : array<float>
    for (h in haps) {
        if (h.whole.start >= 0.0lf && h.whole.start < 1.0lf) {
            starts |> push(h.whole.start)
            gains |> push(h.value.gain)
        }
    }
    t |> equal(length(starts), 4)
    // Pair-sort by start using simple bubble (4 elements)
    for (i in range(length(starts))) {
        for (j in range(i + 1, length(starts))) {
            if (starts[j] < starts[i]) {
                let ts = starts[i]; starts[i] = starts[j]; starts[j] = ts
                let tg = gains[i];  gains[i]  = gains[j];  gains[j]  = tg
            }
        }
    }
    // Expected: start times 0, 0.125, 0.25, 0.375 with gains 0.8, 0.4, 0.2, 0.1
    t |> success(abs(starts[0] - 0.0lf)   < 0.001lf)
    t |> success(abs(starts[1] - 0.125lf) < 0.001lf)
    t |> success(abs(starts[2] - 0.25lf)  < 0.001lf)
    t |> success(abs(starts[3] - 0.375lf) < 0.001lf)
    t |> success(abs(gains[0] - 0.8f) < 0.001f)
    t |> success(abs(gains[1] - 0.4f) < 0.001f)
    t |> success(abs(gains[2] - 0.2f) < 0.001f)
    t |> success(abs(gains[3] - 0.1f) < 0.001f)
}

// ─── stut (alias for echo with different param order) ───

[test]
def test_stut_single(t : T?) {
    // stut(pat, times, feedback, time) == echo(pat, times, time, feedback)
    let pat <- stut(atom("bd"), 1, 0.5, 0.25lf)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.gain - 0.8) < 0.001)
}

// ─── ply ───

[test]
def test_ply(t : T?) {
    // ply repeats each event n times within its span
    let pat <- ply(atom("bd"), 3)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 3)
    // Each sub-event is 1/3 of the original span
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - (1.0lf / 3.0lf)) < 0.001lf)
    t |> success(abs(haps[1].whole.start - (1.0lf / 3.0lf)) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - (2.0lf / 3.0lf)) < 0.001lf)
    t |> success(abs(haps[2].whole.start - (2.0lf / 3.0lf)) < 0.001lf)
    t |> success(abs(haps[2].whole.stop - 1.0lf) < 0.001lf)
}

[test]
def test_ply_multi_event(t : T?) {
    // ply on 2-event pattern with ply=2
    let pat <- ply(s("bd sd"), 2)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // 2 events, each plied into 2 = 4 total
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "bd")
    t |> equal(haps[2].value.s, "sd")
    t |> equal(haps[3].value.s, "sd")
}

// ─── iter ───

[test]
def test_iter_cycle_0(t : T?) {
    // Cycle 0: no shift (0/4 = 0)
    let pat <- iter(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
}

[test]
def test_iter_cycle_1(t : T?) {
    // Cycle 1: shift left by 1/4
    let pat <- iter(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps), 4)
    // After shifting left by 1/4: sd hh cp bd
    t |> equal(haps[0].value.s, "sd")
    t |> equal(haps[1].value.s, "hh")
    t |> equal(haps[2].value.s, "cp")
    t |> equal(haps[3].value.s, "bd")
}

// ─── iterBack ───

[test]
def test_iterBack_cycle_0(t : T?) {
    // Cycle 0: no shift
    let pat <- iterBack(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
}

[test]
def test_iterBack_cycle_1(t : T?) {
    // Cycle 1: shift right by 1/4
    let pat <- iterBack(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps), 4)
    // After shifting right by 1/4: cp bd sd hh
    t |> equal(haps[0].value.s, "cp")
    t |> equal(haps[1].value.s, "bd")
    t |> equal(haps[2].value.s, "sd")
    t |> equal(haps[3].value.s, "hh")
}

// ─── when_cycle ───

[test]
def test_when_cycle_applies_on_match(t : T?) {
    // Transform on even cycles, original on odd
    let pat <- when_cycle(atom("sine") |> set_note(60.0),
        @(c : int) => c % 2 == 0,
        @(p) => transpose(p, 12.0)
    )
    // Cycle 0 (even): transformed (note=72)
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> success(abs(haps0[0].value.note - 72.0) < 0.001)
    // Cycle 1 (odd): original (note=60)
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> success(abs(haps1[0].value.note - 60.0) < 0.001)
}

// ─── linger ───

[test]
def test_linger(t : T?) {
    // linger(pat, 0.25) loops the first quarter — expect bd repeated 4x per cycle
    let pat <- linger(s("bd sd hh cp"), 0.25lf)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    // sort onsets
    var starts : array<double>
    starts |> reserve(length(haps))
    for (h in haps) {
        starts |> push(h.whole.start)
        t |> equal(h.value.s, "bd")
    }
    for (i in range(length(starts))) {
        for (j in range(i + 1, length(starts))) {
            if (starts[j] < starts[i]) {
                let tmp = starts[i]; starts[i] = starts[j]; starts[j] = tmp
            }
        }
    }
    // Expected onsets: 0.0, 0.25, 0.5, 0.75
    t |> success(abs(starts[0] - 0.0lf)  < 0.001lf)
    t |> success(abs(starts[1] - 0.25lf) < 0.001lf)
    t |> success(abs(starts[2] - 0.5lf)  < 0.001lf)
    t |> success(abs(starts[3] - 0.75lf) < 0.001lf)
}

// ─── hurry ───

[test]
def test_hurry(t : T?) {
    // hurry = fast + speed. Factor 2: twice as fast, speed=2.0
    let pat <- hurry(atom("bd"), 2.0lf)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // fast(2) on atom gives 2 events per cycle
    t |> equal(length(haps), 2)
    // speed should be set to 2.0
    t |> success(abs(haps[0].value.speed - 2.0) < 0.001)
    t |> success(abs(haps[1].value.speed - 2.0) < 0.001)
}

// ─── compress ───

[test]
def test_compress(t : T?) {
    // Squeeze full pattern into [0.25, 0.75]
    let pat <- compress(s("bd sd"), 0.25lf, 0.75lf)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    // bd should be in [0.25, 0.5], sd in [0.5, 0.75]
    t |> success(abs(haps[0].whole.start - 0.25lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.5lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.5lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 0.75lf) < 0.001lf)
}

[test]
def test_compress_outside_window(t : T?) {
    // Query outside the window should return nothing
    let pat <- compress(atom("bd"), 0.5lf, 0.75lf)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.4lf))
    t |> equal(length(haps), 0)
}

// ─── chunk ───

[test]
def test_chunk(t : T?) {
    // chunk(pat, 4, transform) applies transform to rotating chunk.
    // Cycle 0: chunk 0 is transformed
    let pat <- chunk(s("bd sd hh cp"), 4, @(p) => fast(p, 2.0lf))
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // Chunk 0 (0.0-0.25) is transformed (fast doubles events),
    // other chunks are original = 3 normal + transformed chunk
    t |> success(length(haps0) >= 4)
    // Cycle 1: chunk 1 is transformed
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> success(length(haps1) >= 4)
}

// ─── striate ───

[test]
def test_striate(t : T?) {
    // striate(pat, 4) cuts the sample into 4 slices via begin/end
    let pat <- striate(atom("bd"), 4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // 4 events — each slice i maps to begin=i/4, end=(i+1)/4
    t |> equal(length(haps), 4)
    for (i in range(4)) {
        t |> equal(haps[i].value.s, "bd")
        let expectedBegin = float(i) * 0.25
        let expectedEnd = float(i + 1) * 0.25
        t |> success(abs(haps[i].value.begin - expectedBegin) < 0.0001)
        t |> success(abs(haps[i].value.end_pos - expectedEnd) < 0.0001)
    }
}

// ─── struct_ ───

[test]
def test_struct_(t : T?) {
    // struct_ restructures pattern using a mask pattern's timing
    // mask s("x x") has 2 windows: [0,0.5) and [0.5,1.0)
    // inner s("bd sd hh cp") queried at each mask window
    let pat <- struct_(s("bd sd hh cp"), s("x x"))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // Each mask window queries the inner pattern at its whole span.
    // Window [0,0.5): inner has bd[0,0.25) and sd[0.25,0.5) -> 2 events
    // Window [0.5,1.0): inner has hh[0.5,0.75) and cp[0.75,1.0) -> 2 events
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
}

// ─── mask ───

[test]
def test_mask(t : T?) {
    // mask keeps events where mask pattern has events
    // atom("bd") produces 1 event [0,1). mask with s("x ~") has event at [0,0.5) only
    // But "~" is rest/silence — let's use a simpler approach
    let pat <- mask(s("bd sd hh cp"), s("x x"))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // s("x x") has 2 events at [0,0.5) and [0.5,1.0), covering all of [0,1)
    // All 4 events of the inner pattern overlap with mask events
    t |> equal(length(haps), 4)
}

[test]
def test_mask_partial(t : T?) {
    // mask with atom("x") covers full [0,1), so all events pass through
    let pat <- mask(s("bd sd hh cp"), atom("x"))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
}

// ─── choose ───

[test]
def test_choose(t : T?) {
    // choose picks one pattern per cycle deterministically (hash-based)
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    var p2 <- atom("hh")
    pats |> emplace <| p0
    pats |> emplace <| p1
    pats |> emplace <| p2
    let pat <- choose(pats)
    // Each cycle should produce exactly 1 event
    for (cyc in range(10)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps <- pat(span)
        t |> equal(length(haps), 1)
    }
}

[test]
def test_choose_deterministic(t : T?) {
    // Same cycle should always pick same pattern
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    let pat <- choose(pats)
    let haps1 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let haps2 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps1), 1)
    t |> equal(length(haps2), 1)
    t |> equal(haps1[0].value.s, haps2[0].value.s)
}

// ─── wchoose ───

[test]
def test_wchoose(t : T?) {
    // Weighted random: heavier weight should appear more often
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    var weights : array<float>
    weights |> push(100.0)
    weights |> push(0.0)
    let pat <- wchoose(pats, weights)
    // With weight 100 vs 0, should always pick "bd"
    for (cyc in range(10)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps <- pat(span)
        t |> equal(length(haps), 1)
        t |> equal(haps[0].value.s, "bd")
    }
}

// ─── randcat ───

[test]
def test_randcat(t : T?) {
    // randcat is alias for choose
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    let pat <- randcat(pats)
    for (cyc in range(10)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps <- pat(span)
        t |> equal(length(haps), 1)
    }
}

// ─── chooseCycles ───

[test]
def test_choose_cycles(t : T?) {
    // chooseCycles is an alias for choose (per-cycle pick)
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    let pat <- chooseCycles(pats)
    for (cyc in range(10)) {
        let span = TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf)
        let haps <- pat(span)
        t |> equal(length(haps), 1)
    }
}

// ─── shuffle ───

[test]
def test_shuffle(t : T?) {
    // shuffle(pat, 4) produces a permutation of 4 subdivisions
    let pat <- shuffle(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // Should still have 4 events (permutation preserves count)
    t |> equal(length(haps), 4)
    // Each event should be one of the original sounds
    for (h in haps) {
        t |> success(h.value.s == "bd" || h.value.s == "sd" || h.value.s == "hh" || h.value.s == "cp")
    }
}

[test]
def test_shuffle_deterministic(t : T?) {
    // Same cycle gives same permutation
    let pat <- shuffle(s("bd sd hh cp"), 4)
    let haps1 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let haps2 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps1), length(haps2))
    for (i in range(length(haps1))) {
        t |> equal(haps1[i].value.s, haps2[i].value.s)
    }
}

// ─── scramble ───

[test]
def test_scramble(t : T?) {
    // scramble(pat, 4) picks random subdivision for each slot (with replacement)
    let pat <- scramble(s("bd sd hh cp"), 4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // Should have 4 events (one per slot)
    t |> equal(length(haps), 4)
    for (h in haps) {
        t |> success(h.value.s == "bd" || h.value.s == "sd" || h.value.s == "hh" || h.value.s == "cp")
    }
}

[test]
def test_scramble_deterministic(t : T?) {
    let pat <- scramble(s("bd sd hh cp"), 4)
    let haps1 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let haps2 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps1), length(haps2))
    for (i in range(length(haps1))) {
        t |> equal(haps1[i].value.s, haps2[i].value.s)
    }
}

[test]
def test_scramble_slot_positions(t : T?) {
    // scramble(8) produces 8 slots; each slot start at i/8
    let pat <- scramble(s("bd sd hh cp ho oh lt ht"), 8)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 8)
    var starts : array<double>
    starts |> reserve(length(haps))
    for (h in haps) {
        starts |> push(h.whole.start)
    }
    for (i in range(length(starts))) {
        for (j in range(i + 1, length(starts))) {
            if (starts[j] < starts[i]) {
                let tmp = starts[i]; starts[i] = starts[j]; starts[j] = tmp
            }
        }
    }
    for (i in range(8)) {
        t |> success(abs(starts[i] - double(i) / 8.0lf) < 0.001lf)
    }
}

[test]
def test_scramble_no_slot_correlation(t : T?) {
    // Regression: the LCG-based RNG made slots 2 and 3 produce the same index
    // in every cycle. Check that across many cycles, slot i and slot i+1 are
    // NOT always equal for any pair (i, i+1).
    let pat <- scramble(s("a b c d e f g h"), 8)
    let M = 100
    var always_equal = fixed_array(true, true, true, true, true, true, true)
    for (cyc in range(M)) {
        var sorted <- pat(TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf))
        for (i in range(1, length(sorted))) {
            var j = i
            while (j > 0 && sorted[j].whole.start < sorted[j - 1].whole.start) {
                let tmp = sorted[j]; sorted[j] = sorted[j - 1]; sorted[j - 1] = tmp
                j --
            }
        }
        for (i in range(length(sorted) - 1)) {
            if (sorted[i].value.s != sorted[i + 1].value.s) {
                always_equal[i] = false
            }
        }
    }
    for (i in range(7)) {
        t |> equal(always_equal[i], false)
    }
}

[test]
def test_scramble_allows_repeats_across_cycles(t : T?) {
    // scramble picks with replacement — over many cycles, expect some sample to
    // appear with non-uniform frequency (unlike shuffle which is always a permutation).
    // Loose check: across 16 cycles × 4 slots = 64 events, at least one sample appears >16 times.
    let pat <- scramble(s("bd sd hh cp"), 4)
    var counts : array<int>
    counts |> resize(4)
    for (cyc in range(16)) {
        let haps <- pat(TimeSpan(start = double(cyc), stop = double(cyc) + 1.0lf))
        for (h in haps) {
            if (h.value.s == "bd") {
                counts[0] ++
            } elif (h.value.s == "sd") {
                counts[1] ++
            } elif (h.value.s == "hh") {
                counts[2] ++
            } elif (h.value.s == "cp") {
                counts[3] ++
            }
        }
    }
    // Total should be 64 (16 cycles × 4 slots)
    var total = 0
    for (c in counts) {
        total += c
    }
    t |> equal(total, 64)
    // At least one bucket non-uniform (with replacement → some variation)
    var min_c = counts[0]
    var max_c = counts[0]
    for (c in counts) {
        if (c < min_c) {
            min_c = c
        }
        if (c > max_c) {
            max_c = c
        }
    }
    t |> success(max_c > min_c)
}

// ─── New Signals ───

[test]
def test_square_signal(t : T?) {
    let pat <- square()
    // First half = 1.0
    let haps_first <- pat(TimeSpan(start = 0.1lf, stop = 0.101lf))
    t |> equal(length(haps_first), 1)
    t |> success(abs(haps_first[0].value.note - 1.0) < 0.01)
    // Second half = 0.0
    let haps_second <- pat(TimeSpan(start = 0.7lf, stop = 0.701lf))
    t |> equal(length(haps_second), 1)
    t |> success(abs(haps_second[0].value.note - 0.0) < 0.01)
}

[test]
def test_isaw_signal(t : T?) {
    // Inverted saw: falls from 1.0 to 0.0
    let pat <- isaw()
    // At t=0.0, isaw=1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - 1.0) < 0.02)
    // At t=0.5, isaw=0.5
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - 0.5) < 0.02)
}

[test]
def test_itri_signal(t : T?) {
    // Inverted triangle: 1.0 at start, 0.0 at mid, 1.0 at end
    let pat <- itri()
    // At t=0.0, itri=1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - 1.0) < 0.02)
    // At t=0.5, itri=0.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - 0.0) < 0.02)
}

[test]
def test_sine2_signal(t : T?) {
    // Bipolar sine -1..1
    let pat <- sine2()
    // At t=0.25, sin(2*pi*0.25)=1.0
    let haps_peak <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> success(abs(haps_peak[0].value.note - 1.0) < 0.02)
    // At t=0.75, sin(2*pi*0.75)=-1.0
    let haps_trough <- pat(TimeSpan(start = 0.75lf, stop = 0.751lf))
    t |> success(abs(haps_trough[0].value.note - (-1.0)) < 0.02)
}

[test]
def test_cosine2_signal(t : T?) {
    // Bipolar cosine -1..1
    let pat <- cosine2()
    // At t=0, cos(0)=1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - 1.0) < 0.02)
    // At t=0.5, cos(pi)=-1.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - (-1.0)) < 0.02)
}

[test]
def test_saw2_signal(t : T?) {
    // Bipolar sawtooth -1..1
    let pat <- saw2()
    // At t=0, saw2 = -1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - (-1.0)) < 0.02)
    // At t=0.5, saw2 = 0.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - 0.0) < 0.02)
}

[test]
def test_tri2_signal(t : T?) {
    // Bipolar triangle -1..1
    let pat <- tri2()
    // At t=0, tri2 = -1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - (-1.0)) < 0.02)
    // At t=0.25, tri2 = 0.0
    let haps_quarter <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> success(abs(haps_quarter[0].value.note - 0.0) < 0.02)
    // At t=0.5, tri2 = 1.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - 1.0) < 0.02)
}

[test]
def test_square2_signal(t : T?) {
    // Bipolar square -1..1
    let pat <- square2()
    // First half = 1.0
    let haps_first <- pat(TimeSpan(start = 0.1lf, stop = 0.101lf))
    t |> success(abs(haps_first[0].value.note - 1.0) < 0.01)
    // Second half = -1.0
    let haps_second <- pat(TimeSpan(start = 0.7lf, stop = 0.701lf))
    t |> success(abs(haps_second[0].value.note - (-1.0)) < 0.01)
}

[test]
def test_rand_signal(t : T?) {
    // rand() returns values in 0..1
    let pat <- rand()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(haps[0].value.note >= 0.0 && haps[0].value.note <= 1.0)
}

[test]
def test_irand_signal(t : T?) {
    // irand(4) returns integers 0..3
    let pat <- irand(4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    let v = haps[0].value.note
    t |> success(v >= 0.0 && v < 4.0)
    // Should be an integer value
    t |> success(abs(v - float(int(v))) < 0.001)
}

[test]
def test_run_signal(t : T?) {
    // run(4) produces discrete events 0, 1, 2, 3
    let pat <- run(4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    t |> success(abs(haps[0].value.note - 0.0) < 0.001)
    t |> success(abs(haps[1].value.note - 1.0) < 0.001)
    t |> success(abs(haps[2].value.note - 2.0) < 0.001)
    t |> success(abs(haps[3].value.note - 3.0) < 0.001)
    // Each event spans 1/4 of the cycle
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.25lf) < 0.001lf)
    t |> success(abs(haps[3].whole.start - 0.75lf) < 0.001lf)
    t |> success(abs(haps[3].whole.stop - 1.0lf) < 0.001lf)
}

// ─── isaw2, itri2, rand2 (additional bipolar signals) ───

[test]
def test_isaw2_signal(t : T?) {
    // Bipolar inverted sawtooth -1..1
    let pat <- isaw2()
    // At t=0, isaw2 = 1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - 1.0) < 0.02)
    // At t=0.5, isaw2 = 0.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - 0.0) < 0.02)
}

[test]
def test_itri2_signal(t : T?) {
    // Bipolar inverted triangle -1..1
    let pat <- itri2()
    // At t=0, itri2 = 1.0
    let haps_start <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps_start[0].value.note - 1.0) < 0.02)
    // At t=0.5, itri2 = -1.0
    let haps_mid <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps_mid[0].value.note - (-1.0)) < 0.02)
}

[test]
def test_rand2_signal(t : T?) {
    // rand2() returns values in -1..1
    let pat <- rand2()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(haps[0].value.note >= -1.0 && haps[0].value.note <= 1.0)
}
