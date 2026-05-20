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
    let tokens <- tokenize("bd sd [hh hh] cp")
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
    let pat <- s("bd")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
}

[test]
def test_sequence(t : T?) {
    // "bd sd" = fastcat([bd, sd]) = 2 events per cycle
    let pat <- s("bd sd")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
}

[test]
def test_subsequence(t : T?) {
    // "bd [hh hh]" = fastcat([bd, fastcat([hh, hh])])
    // = 2 slots: bd takes 0.0-0.5, [hh hh] takes 0.5-1.0
    // within the bracket, two hh's subdivide that half
    let pat <- s("bd [hh hh]")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "hh")
    t |> equal(haps[2].value.s, "hh")
}

[test]
def test_silence(t : T?) {
    // "bd ~ sd ~" = 4 slots, 2 silent
    let pat <- s("bd ~ sd ~")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)  // silence produces no events
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "sd")
}

[test]
def test_fast_modifier(t : T?) {
    // "bd*3" = fast(3, atom("bd")) = 3 events per cycle
    let pat <- s("bd*3")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "bd")
}

[test]
def test_slow_modifier(t : T?) {
    // "bd/2" = slow(2, atom("bd")) = spans 2 cycles
    let pat <- s("bd/2")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].whole.stop - 2.0lf) < 0.001lf)
}

[test]
def test_degrade_modifier(t : T?) {
    // "bd?" = degrade(0.5, bd)
    let pat <- s("bd?1")  // degrade with prob=1.0 removes everything
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 0)
}

[test]
def test_sample_index(t : T?) {
    // "bd:2" = atom("bd") with n=2
    let pat <- s("bd:2")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[0].value.n, 2)
}

[test]
def test_stack(t : T?) {
    // "bd, hh" = stack([bd, hh]) = 2 events at same time
    let pat <- s("bd, hh")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> equal(haps[0].value.s, "bd")
    t |> equal(haps[1].value.s, "hh")
}

[test]
def test_alternate(t : T?) {
    // "<bd sd>" = cat([bd, sd]) = bd on cycle 0, sd on cycle 1
    let pat <- s("<bd sd>")
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 1)
    t |> equal(haps0[0].value.s, "bd")
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    t |> equal(haps1[0].value.s, "sd")
}

[test]
def test_full_pattern(t : T?) {
    // the classic: "bd sd [hh hh] cp"
    let pat <- s("bd sd [hh hh] cp")
    let haps <- query_one_cycle(pat)
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
    let pat <- s("c3 e3 g3")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)
    t |> equal(haps[0].value.s, "sine")
    t |> success(abs(haps[0].value.note - 48.0) < 0.001)  // c3 = 48
    t |> success(abs(haps[1].value.note - 52.0) < 0.001)  // e3 = 52
    t |> success(abs(haps[2].value.note - 55.0) < 0.001)  // g3 = 55
}

[test]
def test_note_names_sharps_flats(t : T?) {
    let pat <- s("cs3 eb3")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> success(abs(haps[0].value.note - 49.0) < 0.001)  // c#3 = 49
    t |> success(abs(haps[1].value.note - 51.0) < 0.001)  // eb3 = 51
}

[test]
def test_every(t : T?) {
    // every(2, fast_bd, normal_bd) — fast on even cycles, normal on odd
    let on <- atom("bd") |> fast(2.0lf)
    let off <- atom("bd")
    let pat <- every(2, on, off)
    // cycle 0: on (fast) = 2 events
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 2)
    // cycle 1: off (normal) = 1 event
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 1)
    // cycle 2: on again
    let haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 2)
}

[test]
def test_every_fast(t : T?) {
    let pat <- every_fast(3, 2.0lf, "bd sd")
    // cycle 0: fast(2) = 4 events
    let haps0 <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps0), 4)
    // cycle 1: normal = 2 events
    let haps1 <- pat(TimeSpan(start = 1.0lf, stop = 2.0lf))
    t |> equal(length(haps1), 2)
    // cycle 2: normal = 2 events
    let haps2 <- pat(TimeSpan(start = 2.0lf, stop = 3.0lf))
    t |> equal(length(haps2), 2)
}

[test]
def test_note_pattern(t : T?) {
    let pat <- note_pattern("c3 ~ e3 g3")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 3)  // ~ produces silence
    t |> equal(haps[0].value.s, "sine")
    t |> equal(haps[1].value.s, "sine")
    t |> equal(haps[2].value.s, "sine")
}

// ─── Tokenizer: accidentals must stay inside the word ───

[test]
def test_tokenize_sharp_in_word(t : T?) {
    // Previously '#' was silently skipped, splitting "d#3" into "d" + "3".
    // It must now be part of the WORD so note-name parsing sees "d#3".
    let tokens <- tokenize("d#3 eb4 f#")
    t |> equal(tokens[0].kind, TokenKind.WORD)
    t |> equal(tokens[0].text, "d#3")
    t |> equal(tokens[1].kind, TokenKind.WORD)
    t |> equal(tokens[1].text, "eb4")
    t |> equal(tokens[2].kind, TokenKind.WORD)
    t |> equal(tokens[2].text, "f#")
    t |> equal(tokens[3].kind, TokenKind.EOF_TOKEN)
}

// ─── parse_note_name: explicit octave required ───

[test]
def test_parse_note_name_requires_octave(t : T?) {
    t |> success(abs(parse_note_name("d3") - 50.0) < 0.01)
    t |> success(abs(parse_note_name("d#3") - 51.0) < 0.01)
    t |> success(abs(parse_note_name("eb4") - 63.0) < 0.01)
    t |> success(abs(parse_note_name("c4") - 60.0) < 0.01)
    // Bare letter without octave returns -1 so s("a e i") stays as sound tokens.
    t |> success(parse_note_name("d") < 0.0)
    t |> success(parse_note_name("d#") < 0.0)
    t |> success(parse_note_name("a") < 0.0)
}

// ─── parse_note_name_default: defaults octave to 3 ───

[test]
def test_parse_note_name_default(t : T?) {
    // Bare "d" → D3 (MIDI 50) via default octave 3
    t |> success(abs(parse_note_name_default("d", 3) - 50.0) < 0.01)
    t |> success(abs(parse_note_name_default("d#", 3) - 51.0) < 0.01)
    t |> success(abs(parse_note_name_default("eb", 3) - 51.0) < 0.01)  // Eb3 = 48 + (4 - 1)
    // Explicit octave overrides the default
    t |> success(abs(parse_note_name_default("d3", 3) - 50.0) < 0.01)
    t |> success(abs(parse_note_name_default("d#3", 3) - 51.0) < 0.01)
    t |> success(abs(parse_note_name_default("c4", 3) - 60.0) < 0.01)
    t |> success(abs(parse_note_name_default("d5", 3) - 74.0) < 0.01)
    // Non-note strings return -1 (for sound-name disambiguation)
    t |> success(parse_note_name_default("bd", 3) < 0.0)
    t |> success(parse_note_name_default("hh", 3) < 0.0)
}

// ─── note(): bare-letter note names resolve to default octave 3 ───

[test]
def test_note_bare_letters(t : T?) {
    // note("d d d# d") → D3, D3, D#3, D3 (default octave 3)
    let pat <- note("d d d# d", "supersaw")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 4)
    t |> success(abs(haps[0].value.note - 50.0) < 0.01, "note 0 should be D3=50, got {haps[0].value.note}")
    t |> success(abs(haps[1].value.note - 50.0) < 0.01)
    t |> success(abs(haps[2].value.note - 51.0) < 0.01, "note 2 should be D#3=51, got {haps[2].value.note}")
    t |> success(abs(haps[3].value.note - 50.0) < 0.01)
    // .s gets set by set_sound("supersaw"), not left as the note name
    t |> equal(haps[0].value.s, "supersaw")
    t |> equal(haps[2].value.s, "supersaw")
}

[test]
def test_note_mixed_octaves(t : T?) {
    // Mixing bare and explicit forms should round-trip.
    let pat <- note("d d#3 e4 f", "sine")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 4)
    t |> success(abs(haps[0].value.note - 50.0) < 0.01)  // D3 (default oct)
    t |> success(abs(haps[1].value.note - 51.0) < 0.01)  // D#3 explicit
    t |> success(abs(haps[2].value.note - 64.0) < 0.01)  // E4
    t |> success(abs(haps[3].value.note - 53.0) < 0.01)  // F3 (default oct)
}

[test]
def test_note_flat_bare(t : T?) {
    // "eb" should resolve to E♭3 = MIDI 51 - 1 semitone below D4 ... = 51
    // Actually Eb3 = 4+3 chroma+octave offsets...
    // chroma(e)=4 + accidental(-1) + (3+1)*12 = 48 + 4 - 1 = 51 → Eb3
    let pat <- note("eb eb4", "sine")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 2)
    t |> success(abs(haps[0].value.note - 51.0) < 0.01, "Eb3={haps[0].value.note}")
    t |> success(abs(haps[1].value.note - 63.0) < 0.01, "Eb4={haps[1].value.note}")
}

[test]
def test_s_single_letter_stays_sound(t : T?) {
    // Crucial invariant: s("a e i o u") is sound tokens (vowel names), NOT notes.
    // parse_note_name rejects bare letters so this still produces atoms with .s set.
    let pat <- s("a e i o u")
    let haps <- query_one_cycle(pat)
    t |> equal(length(haps), 5)
    t |> equal(haps[0].value.s, "a")
    t |> equal(haps[1].value.s, "e")
    t |> equal(haps[2].value.s, "i")
    t |> equal(haps[3].value.s, "o")
    t |> equal(haps[4].value.s, "u")
}
