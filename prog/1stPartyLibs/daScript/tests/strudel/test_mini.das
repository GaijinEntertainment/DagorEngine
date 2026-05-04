options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

def query_one_cycle(var pat : Pattern) : array<Hap> {
    return <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
}

[test]
def test_tokenize(t : T?) {
    var tokens <- tokenize("bd sd [hh hh] cp")
    // bd sd [ hh hh ] cp EOF = 8 tokens
    t |> equal(tokens[0].kind, TokenKind.WORD)
    t |> equal(tokens[0].text, "bd")
    t |> equal(tokens[1].kind, TokenKind.WORD)
    t |> equal(tokens[1].text, "sd")
    t |> equal(tokens[2].kind, TokenKind.LBRACKET)
    t |> equal(tokens[3].text, "hh")
    t |> equal(tokens[4].text, "hh")
    t |> equal(tokens[5].kind, TokenKind.RBRACKET)
    t |> equal(tokens[6].text, "cp")
    t |> equal(tokens[7].kind, TokenKind.EOF_TOKEN)
}

[test]
def test_single_atom(t : T?) {
    var pat <- s("bd")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
}

[test]
def test_sequence(t : T?) {
    // "bd sd" = fastcat([bd, sd]) = 2 events per cycle
    var pat <- s("bd sd")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
}

[test]
def test_subsequence(t : T?) {
    // "bd [hh hh]" = fastcat([bd, fastcat([hh, hh])])
    // = 2 slots: bd takes 0.0-0.5, [hh hh] takes 0.5-1.0
    // within the bracket, two hh's subdivide that half
    var pat <- s("bd [hh hh]")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "hh")
    t |> equal(haps[2].value.s, "hh")
}

[test]
def test_silence(t : T?) {
    // "bd ~ sd ~" = 4 slots, 2 silent
    var pat <- s("bd ~ sd ~")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)  // silence produces no events
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
}

[test]
def test_fast_modifier(t : T?) {
    // "bd*3" = fast(3, atom("bd")) = 3 events per cycle
    var pat <- s("bd*3")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "bd")
}

[test]
def test_slow_modifier(t : T?) {
    // "bd/2" = slow(2, atom("bd")) = spans 2 cycles
    var pat <- s("bd/2")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].whole.stop - 2.0lf) < 0.001lf)
}

[test]
def test_degrade_modifier(t : T?) {
    // "bd?" = degrade(0.5, bd)
    var pat <- s("bd?1")  // degrade with prob=1.0 removes everything
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 0)
}

[test]
def test_sample_index(t : T?) {
    // "bd:2" = atom("bd") with n=2
    var pat <- s("bd:2")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[0].value.n, 2)
}

[test]
def test_stack(t : T?) {
    // "bd, hh" = stack([bd, hh]) = 2 events at same time
    var pat <- s("bd, hh")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "hh")
}

[test]
def test_alternate(t : T?) {
    // "<bd sd>" = cat([bd, sd]) = bd on cycle 0, sd on cycle 1
    var pat <- s("<bd sd>")
    var haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> equal(haps0[0].value.s, "bd")
    var haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> equal(haps1[0].value.s, "sd")
}

[test]
def test_full_pattern(t : T?) {
    // the classic: "bd sd [hh hh] cp"
    var pat <- s("bd sd [hh hh] cp")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 5)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "hh")
    t |> equal(haps[4].value.s, "cp")
}

[test]
def test_note_names(t : T?) {
    // note names parse to MIDI + sound="sine"
    var pat <- s("c3 e3 g3")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "sine")
    t |> success(abs(haps[0].value.note - 48.0) < 0.001)  // c3 = 48
    t |> success(abs(haps[1].value.note - 52.0) < 0.001)  // e3 = 52
    t |> success(abs(haps[2].value.note - 55.0) < 0.001)  // g3 = 55
}

[test]
def test_note_names_sharps_flats(t : T?) {
    var pat <- s("cs3 eb3")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> success(abs(haps[0].value.note - 49.0) < 0.001)  // c#3 = 49
    t |> success(abs(haps[1].value.note - 51.0) < 0.001)  // eb3 = 51
}

[test]
def test_every(t : T?) {
    // every(2, fast_bd, normal_bd) — fast on even cycles, normal on odd
    var on <- atom("bd") |> fast(2.0lf)
    var off <- atom("bd")
    var pat <- every(2, on, off)
    // cycle 0: on (fast) = 2 events
    var haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 2)
    // cycle 1: off (normal) = 1 event
    var haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    // cycle 2: on again
    var haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 2)
}

[test]
def test_every_fast(t : T?) {
    var pat <- every_fast(3, 2.0lf, "bd sd")
    // cycle 0: fast(2) = 4 events
    var haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 4)
    // cycle 1: normal = 2 events
    var haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 2)
    // cycle 2: normal = 2 events
    var haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 2)
}

[test]
def test_note_pattern(t : T?) {
    var pat <- note_pattern("c3 ~ e3 g3")
    var haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)  // ~ produces silence
    t |> equal(haps[0].value.s, "sine")
    t |> equal(haps[1].value.s, "sine")
    t |> equal(haps[2].value.s, "sine")
}
