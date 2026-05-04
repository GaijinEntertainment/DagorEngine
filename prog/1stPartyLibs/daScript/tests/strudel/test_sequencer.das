options gen2
options indenting = 4
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require strudel/strudel
require math

def query_cycle(var pat : Pattern; cycle_start : double; cycle_end : double) : array<Hap> {
    return <- pat(TimeSpan(start = cycle_start, stop = cycle_end))
}

def query_one_cycle(var pat : Pattern) : array<Hap> {
    return <- query_cycle(pat, 0.0lf, 1.0lf)
}

// ─── Tokenizer ───

[test]
def test_pipe_token(t : T?) {
    t |> run("pipe token is recognized") <| @(t : T?) {
        let tokens <- tokenize("c4 | e4")
        // c4 PIPE e4 EOF = 4 tokens
        t |> equal(length(tokens), 4)
        t |> equal(tokens[1].kind, TokenKind.PIPE)
        t |> equal(tokens[1].text, "|")
    }
    t |> run("multiple pipes") <| @(t : T?) {
        let tokens <- tokenize("a | b | c")
        // a PIPE b PIPE c EOF = 6 tokens
        t |> equal(length(tokens), 6)
        t |> equal(tokens[1].kind, TokenKind.PIPE)
        t |> equal(tokens[3].kind, TokenKind.PIPE)
    }
}

// ─── seq() with bars ───

[test]
def test_seq_single_bar(t : T?) {
    t |> run("single bar same as s()") <| @(t : T?) {
        let pat <- seq("c4 e4 g4")
        let haps <- query_one_cycle(pat)
        t |> equal(length(haps), 3)
        // c4=60, e4=64, g4=67
        t |> success(abs(haps[0].value.note - 60.0) < 0.01)
        t |> success(abs(haps[1].value.note - 64.0) < 0.01)
        t |> success(abs(haps[2].value.note - 67.0) < 0.01)
    }
}

[test]
def test_seq_two_bars(t : T?) {
    t |> run("bar 1 in cycle 0") <| @(t : T?) {
        let pat <- seq("c4 e4 | g4 a4")
        let haps <- query_cycle(pat, 0.0lf, 1.0lf)
        t |> equal(length(haps), 2)
        t |> success(abs(haps[0].value.note - 60.0) < 0.01, "c4=60")
        t |> success(abs(haps[1].value.note - 64.0) < 0.01, "e4=64")
    }
    t |> run("bar 2 in cycle 1") <| @(t : T?) {
        let pat <- seq("c4 e4 | g4 a4")
        let haps <- query_cycle(pat, 1.0lf, 2.0lf)
        t |> equal(length(haps), 2)
        t |> success(abs(haps[0].value.note - 67.0) < 0.01, "g4=67")
        t |> success(abs(haps[1].value.note - 69.0) < 0.01, "a4=69")
    }
}

[test]
def test_seq_uneven_bars(t : T?) {
    t |> run("1 note vs 3 notes") <| @(t : T?) {
        let pat <- seq("c4 | e4 g4 a4")
        let haps0 <- query_cycle(pat, 0.0lf, 1.0lf)
        t |> equal(length(haps0), 1, "bar 1 has 1 note")
        t |> success(abs(haps0[0].value.note - 60.0) < 0.01)
        let haps1 <- query_cycle(pat, 1.0lf, 2.0lf)
        t |> equal(length(haps1), 3, "bar 2 has 3 notes")
    }
}

[test]
def test_seq_rests(t : T?) {
    t |> run("tilde creates silence slot") <| @(t : T?) {
        let pat <- seq("c4 ~ e4 | g4")
        let haps <- query_cycle(pat, 0.0lf, 1.0lf)
        // c4, silence, e4 — silence produces no haps
        t |> equal(length(haps), 2, "2 notes (silence has no hap)")
        t |> success(abs(haps[0].value.note - 60.0) < 0.01, "c4")
        t |> success(abs(haps[1].value.note - 64.0) < 0.01, "e4")
    }
}

[test]
def test_seq_sound_parameter(t : T?) {
    t |> run("sound is set on all events") <| @(t : T?) {
        let pat <- seq("c4 e4", "triangle")
        let haps <- query_one_cycle(pat)
        t |> equal(length(haps), 2)
        t |> equal(haps[0].value.s, "triangle")
        t |> equal(haps[1].value.s, "triangle")
    }
}

[test]
def test_seq_note_values(t : T?) {
    t |> run("MIDI note numbers correct") <| @(t : T?) {
        let pat <- seq("c4 d4 e4 f4 g4 a4 b4")
        let haps <- query_one_cycle(pat)
        t |> equal(length(haps), 7)
        t |> success(abs(haps[0].value.note - 60.0) < 0.01, "c4=60")
        t |> success(abs(haps[1].value.note - 62.0) < 0.01, "d4=62")
        t |> success(abs(haps[2].value.note - 64.0) < 0.01, "e4=64")
        t |> success(abs(haps[3].value.note - 65.0) < 0.01, "f4=65")
        t |> success(abs(haps[4].value.note - 67.0) < 0.01, "g4=67")
        t |> success(abs(haps[5].value.note - 69.0) < 0.01, "a4=69")
        t |> success(abs(haps[6].value.note - 71.0) < 0.01, "b4=71")
    }
}

[test]
def test_seq_empty_bar(t : T?) {
    t |> run("empty bar produces silence cycle") <| @(t : T?) {
        let pat <- seq("c4 | | e4")
        // cycle 0: c4, cycle 1: silence, cycle 2: e4
        let haps0 <- query_cycle(pat, 0.0lf, 1.0lf)
        t |> equal(length(haps0), 1, "bar 1 has c4")
        let haps1 <- query_cycle(pat, 1.0lf, 2.0lf)
        t |> equal(length(haps1), 0, "bar 2 is silent")
        let haps2 <- query_cycle(pat, 2.0lf, 3.0lf)
        t |> equal(length(haps2), 1, "bar 3 has e4")
    }
}

[test]
def test_seq_subdivisions_in_bars(t : T?) {
    t |> run("[] grouping works inside bars") <| @(t : T?) {
        let pat <- seq("[c4 e4] g4 | a4")
        let haps <- query_cycle(pat, 0.0lf, 1.0lf)
        // [c4 e4] subdivides into first half, g4 takes second half
        t |> equal(length(haps), 3, "3 notes in bar 1")
    }
}

[test]
def test_seq_sharps_flats(t : T?) {
    t |> run("sharp and flat note names") <| @(t : T?) {
        let pat <- seq("cs4 eb4 fs4")
        let haps <- query_one_cycle(pat)
        t |> equal(length(haps), 3)
        t |> success(abs(haps[0].value.note - 61.0) < 0.01, "cs4=61")
        t |> success(abs(haps[1].value.note - 63.0) < 0.01, "eb4=63")
        t |> success(abs(haps[2].value.note - 66.0) < 0.01, "fs4=66")
    }
}

[test]
def test_seq_bars_cycle(t : T?) {
    t |> run("bars repeat via cat cycling") <| @(t : T?) {
        let pat <- seq("c4 | e4")
        // cat cycles: cycle 0 = bar 0, cycle 1 = bar 1, cycle 2 = bar 0 again
        let haps2 <- query_cycle(pat, 2.0lf, 3.0lf)
        t |> equal(length(haps2), 1)
        t |> success(abs(haps2[0].value.note - 60.0) < 0.01, "c4 repeats at cycle 2")
    }
}
