options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

[test]
def test_splitSpans(t : T?) {
    var spans <- splitSpans(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(spans), 1)
    t |> success(abs(spans[0].start - 0.0lf) < 0.001lf)
    t |> success(abs(spans[0].stop - 1.0lf) < 0.001lf)
    var spans2 <- splitSpans(TimeSpan(start = 0.75lf, stop = 1.25lf))
    t |> equal(length(spans2), 2)
    t |> success(abs(spans2[0].start - 0.75lf) < 0.001lf)
    t |> success(abs(spans2[0].stop - 1.0lf) < 0.001lf)
    t |> success(abs(spans2[1].start - 1.0lf) < 0.001lf)
    t |> success(abs(spans2[1].stop - 1.25lf) < 0.001lf)
    var spans3 <- splitSpans(TimeSpan(start = 0.75lf, stop = 2.25lf))
    t |> equal(length(spans3), 3)
}

[test]
def test_pure(t : T?) {
    var pat <- pure(Event(s = "bd"))
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 1.0lf) < 0.001lf)
    t |> success(abs(haps[0].part.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].part.stop - 1.0lf) < 0.001lf)
}

[test]
def test_pure_partial(t : T?) {
    var pat <- pure(Event(s = "bd"))
    var haps <- pat(TimeSpan(start = 0.25lf, stop = 0.75lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 1.0lf) < 0.001lf)
    t |> success(abs(haps[0].part.start - 0.25lf) < 0.001lf)
    t |> success(abs(haps[0].part.stop - 0.75lf) < 0.001lf)
}

[test]
def test_silence(t : T?) {
    var pat <- silence()
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 10.0lf))
    t |> equal(length(haps), 0)
}

[test]
def test_atom(t : T?) {
    var pat <- atom("sd")
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "sd")
}

[test]
def test_fast(t : T?) {
    var pat <- atom("bd") |> fast(2.0lf)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.5lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.5lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 1.0lf) < 0.001lf)
}

[test]
def test_slow(t : T?) {
    var pat <- atom("bd") |> slow(2.0lf)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 2.0lf) < 0.001lf)
}

[test]
def test_stack(t : T?) {
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("hh")
    pats |> emplace <| p0
    pats |> emplace <| p1
    var pat <- stack(pats)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "hh")
}

[test]
def test_cat(t : T?) {
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    var pat <- cat(pats)
    var haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> equal(haps0[0].value.s, "bd")
    var haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> equal(haps1[0].value.s, "sd")
    var haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 1)
    t |> equal(haps2[0].value.s, "bd")
}

[test]
def test_fastcat(t : T?) {
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    var p2 <- atom("hh")
    var p3 <- atom("cp")
    pats |> emplace <| p0
    pats |> emplace <| p1
    pats |> emplace <| p2
    pats |> emplace <| p3
    var pat <- fastcat(pats)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 4)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
    t |> equal(haps[2].value.s, "hh")
    t |> equal(haps[3].value.s, "cp")
    t |> success(abs(haps[0].whole.start - 0.0lf) < 0.001lf)
    t |> success(abs(haps[0].whole.stop - 0.25lf) < 0.001lf)
    t |> success(abs(haps[1].whole.start - 0.25lf) < 0.001lf)
    t |> success(abs(haps[1].whole.stop - 0.5lf) < 0.001lf)
}

[test]
def test_fmap(t : T?) {
    var fn <- @(e : Event) : Event { return Event(s = e.s, gain = 0.5); }
    var pat <- atom("bd") |> fmap(fn)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
    t |> success(abs(haps[0].value.gain - 0.5) < 0.001)
}

[test]
def test_degrade(t : T?) {
    var pat <- atom("bd") |> degrade(1.0)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 0)
    var pat2 <- atom("bd") |> degrade(0.0)
    var haps2 <- pat2(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps2), 1)
}

[test]
def test_bjorklund(t : T?) {
    var rhythm <- bjorklund(3, 8)
    t |> equal(length(rhythm), 8)
    var count = 0
    for (b in rhythm) {
        if (b) {
            count += 1
        }
    }
    t |> equal(count, 3)
}

[test]
def test_euclid(t : T?) {
    var pat <- atom("bd") |> euclid(3, 8)
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 3)
}

[test]
def test_cross_cycle_query(t : T?) {
    var pats : array<Pattern>
    var p0 <- atom("bd")
    var p1 <- atom("sd")
    pats |> emplace <| p0
    pats |> emplace <| p1
    var pat <- cat(pats)
    var haps <- pat(TimeSpan(start = 0.5lf, stop = 1.5lf))
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
}
