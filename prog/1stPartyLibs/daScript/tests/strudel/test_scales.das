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

// ─── Scale interval tables ───

[test]
def test_major_intervals(t : T?) {
    var inscope intervals <- get_scale_intervals_by_name("major")
    t |> equal(length(intervals), 7)
    t |> equal(intervals[0], 0)
    t |> equal(intervals[1], 2)
    t |> equal(intervals[2], 4)
    t |> equal(intervals[3], 5)
    t |> equal(intervals[4], 7)
    t |> equal(intervals[5], 9)
    t |> equal(intervals[6], 11)
}

[test]
def test_minor_intervals(t : T?) {
    var inscope intervals <- get_scale_intervals_by_name("minor")
    t |> equal(length(intervals), 7)
    t |> equal(intervals[0], 0)
    t |> equal(intervals[2], 3)  // minor third
    t |> equal(intervals[4], 7)  // perfect fifth
}

[test]
def test_pentatonic_intervals(t : T?) {
    var inscope intervals <- get_scale_intervals_by_name("pentatonic")
    t |> equal(length(intervals), 5)
    t |> equal(intervals[0], 0)
    t |> equal(intervals[1], 3)
    t |> equal(intervals[2], 5)
    t |> equal(intervals[3], 7)
    t |> equal(intervals[4], 10)
}

[test]
def test_blues_intervals(t : T?) {
    var inscope intervals <- get_scale_intervals_by_name("blues")
    t |> equal(length(intervals), 6)
    t |> equal(intervals[3], 6)  // blue note (tritone)
}

// ─── Root note parsing ───

[test]
def test_parse_root_c4(t : T?) {
    t |> equal(parse_root("C4"), 60)
}

[test]
def test_parse_root_c3(t : T?) {
    t |> equal(parse_root("C3"), 48)
}

[test]
def test_parse_root_g(t : T?) {
    // G without octave → G3 = 55 (default octave is 3)
    t |> equal(parse_root("G"), 55)
}

[test]
def test_parse_root_eb4(t : T?) {
    // Eb4 = E4 - 1 = 64 - 1 = 63
    t |> equal(parse_root("Eb4"), 63)
}

[test]
def test_parse_root_fs3(t : T?) {
    // F#3 = F3 + 1 = 53 + 1 = 54
    t |> equal(parse_root("Fs3"), 54)
}

// ─── degree_to_note ───

[test]
def test_degree_to_note_c_major(t : T?) {
    // C major: C D E F G A B = 60 62 64 65 67 69 71
    var inscope intervals <- get_scale_intervals_by_name("major")
    t |> success(abs(degree_to_note(0, 60, intervals) - 60.0) < 0.01)
    t |> success(abs(degree_to_note(1, 60, intervals) - 62.0) < 0.01)
    t |> success(abs(degree_to_note(2, 60, intervals) - 64.0) < 0.01)
    t |> success(abs(degree_to_note(3, 60, intervals) - 65.0) < 0.01)
    t |> success(abs(degree_to_note(4, 60, intervals) - 67.0) < 0.01)
    t |> success(abs(degree_to_note(5, 60, intervals) - 69.0) < 0.01)
    t |> success(abs(degree_to_note(6, 60, intervals) - 71.0) < 0.01)
}

[test]
def test_scale_octave_wrap(t : T?) {
    // degree 7 in major scale = root + 12 (next octave)
    var inscope intervals <- get_scale_intervals_by_name("major")
    t |> success(abs(degree_to_note(7, 60, intervals) - 72.0) < 0.01)
    // degree 8 = D5 = 74
    t |> success(abs(degree_to_note(8, 60, intervals) - 74.0) < 0.01)
    // degree 14 = C6 = 84
    t |> success(abs(degree_to_note(14, 60, intervals) - 84.0) < 0.01)
}

[test]
def test_scale_negative_degrees(t : T?) {
    // degree -1 in C major = B3 = 59
    var inscope intervals <- get_scale_intervals_by_name("major")
    t |> success(abs(degree_to_note(-1, 60, intervals) - 59.0) < 0.01)
    // degree -2 = A3 = 57
    t |> success(abs(degree_to_note(-2, 60, intervals) - 57.0) < 0.01)
    // degree -7 = C3 = 48
    t |> success(abs(degree_to_note(-7, 60, intervals) - 48.0) < 0.01)
}

[test]
def test_degree_to_note_pentatonic(t : T?) {
    // C4 pentatonic (minor): C Eb F G Bb = 60 63 65 67 70
    var inscope intervals <- get_scale_intervals_by_name("pentatonic")
    t |> success(abs(degree_to_note(0, 60, intervals) - 60.0) < 0.01)
    t |> success(abs(degree_to_note(1, 60, intervals) - 63.0) < 0.01)
    t |> success(abs(degree_to_note(2, 60, intervals) - 65.0) < 0.01)
    t |> success(abs(degree_to_note(3, 60, intervals) - 67.0) < 0.01)
    t |> success(abs(degree_to_note(4, 60, intervals) - 70.0) < 0.01)
    // wrap: degree 5 = C5 = 72
    t |> success(abs(degree_to_note(5, 60, intervals) - 72.0) < 0.01)
}

// ─── set_scale setter ───

[test]
def test_set_scale_c_major(t : T?) {
    // n("0 2 4") with scale "C4:major" → notes 60, 64, 67 (C E G)
    let pat <- n("0 2 4") |> set_scale("C4:major")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 3)
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 64.0) < 0.01)  // E4
    t |> success(abs(events[2].note - 67.0) < 0.01)  // G4
}

[test]
def test_set_scale_c_minor(t : T?) {
    // n("0 2 4") with C4:minor → 60, 63, 67 (C Eb G)
    let pat <- n("0 2 4") |> set_scale("C4:minor")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 3)
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 63.0) < 0.01)  // Eb4 (minor third)
    t |> success(abs(events[2].note - 67.0) < 0.01)  // G4
}

[test]
def test_set_scale_g_major(t : T?) {
    // n("0 1 2") with G3:major → 55, 57, 59 (G3 A3 B3)
    let pat <- n("0 1 2") |> set_scale("G3:major")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 3)
    t |> success(abs(events[0].note - 55.0) < 0.01)  // G3
    t |> success(abs(events[1].note - 57.0) < 0.01)  // A3
    t |> success(abs(events[2].note - 59.0) < 0.01)  // B3
}

[test]
def test_set_scale_no_root(t : T?) {
    // just "major" without root → defaults to C3 (default octave 3)
    let pat <- n("0 4") |> set_scale("major")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 2)
    t |> success(abs(events[0].note - 48.0) < 0.01)  // C3
    t |> success(abs(events[1].note - 55.0) < 0.01)  // G3
}

// ─── scale_pattern convenience ───

[test]
def test_scale_pattern(t : T?) {
    let pat <- scale_pattern("0 2 4", "C4:major", "sawtooth")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 3)
    t |> success(abs(events[0].note - 60.0) < 0.01)
    t |> equal(events[0].s, "sawtooth")
}

[test]
def test_scale_pattern_with_rests(t : T?) {
    // rests produce silence (no events)
    let pat <- scale_pattern("0 ~ 2 ~", "C4:pentatonic")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 2)  // only 2 actual events, rests are gaps
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 65.0) < 0.01)  // F4 (degree 2 of minor pentatonic)
}

// ─── Chaining with other setters ───

[test]
def test_scale_with_effects(t : T?) {
    let pat <- n("0 4") |> set_scale("C4:major") |> set_room(0.5) |> set_gain(0.3)
    var inscope events <- query_all(pat)
    t |> equal(length(events), 2)
    t |> success(abs(events[0].note - 60.0) < 0.01)
    t |> success(abs(events[0].room - 0.5) < 0.001)
    t |> success(abs(events[0].gain - 0.3) < 0.001)
}

// ─── Angle bracket alternation with scale ───

[test]
def test_scale_with_alternation(t : T?) {
    // <0 4> alternates per cycle: cycle 0 → degree 0, cycle 1 → degree 4
    let pat <- n("<0 4>") |> set_scale("C4:major")
    var haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> success(abs(haps0[0].value.note - 60.0) < 0.01)  // C4
    delete haps0
    var haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> success(abs(haps1[0].value.note - 67.0) < 0.01)  // G4
    delete haps1
}

// ─── Colon-separated format (strudel compat) ───

[test]
def test_scale_colon_format(t : T?) {
    // "C4:minor:pentatonic" same as "C4:minor_pentatonic"
    let pat <- n("0 1 2 3 4") |> set_scale("C4:minor:pentatonic")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 5)
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 63.0) < 0.01)  // Eb4
    t |> success(abs(events[2].note - 65.0) < 0.01)  // F4
}

[test]
def test_scale_colon_major_pentatonic(t : T?) {
    // "C4:major:pentatonic" same as "C4:major_pentatonic"
    let pat <- n("0 1 2") |> set_scale("C4:major:pentatonic")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 3)
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 62.0) < 0.01)  // D4
    t |> success(abs(events[2].note - 64.0) < 0.01)  // E4
}

[test]
def test_scale_colon_harmonic_minor(t : T?) {
    // "C4:harmonic:minor" same as "C4:harmonic_minor"
    let pat <- n("0 6") |> set_scale("C4:harmonic:minor")
    var inscope events <- query_all(pat)
    t |> equal(length(events), 2)
    t |> success(abs(events[0].note - 60.0) < 0.01)  // C4
    t |> success(abs(events[1].note - 71.0) < 0.01)  // B4 (raised 7th)
}
