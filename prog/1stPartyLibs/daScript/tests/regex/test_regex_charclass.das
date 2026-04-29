options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/regex

// helper: match from start, return end position or -1
def regex_test_match(pattern, input : string) : int {
    var re <- regex_compile(pattern)
    return regex_match(re, input)
}

// ── \w (word characters) ────────────────────────────────────

[test]
def test_word_class(t : T?) {
    t |> run("\\w matches letters") @(t : T?) {
        t |> equal(regex_test_match("\\w", "a"), 1)
        t |> equal(regex_test_match("\\w", "Z"), 1)
    }
    t |> run("\\w matches digits") @(t : T?) {
        t |> equal(regex_test_match("\\w", "0"), 1)
        t |> equal(regex_test_match("\\w", "9"), 1)
    }
    t |> run("\\w matches underscore") @(t : T?) {
        t |> equal(regex_test_match("\\w", "_"), 1)
    }
    t |> run("\\w rejects non-word") @(t : T?) {
        t |> equal(regex_test_match("\\w", " "), -1)
        t |> equal(regex_test_match("\\w", "!"), -1)
        t |> equal(regex_test_match("\\w", "-"), -1)
    }
}

// ── \W (non-word characters) ────────────────────────────────

[test]
def test_non_word_class(t : T?) {
    t |> run("\\W matches non-word chars") @(t : T?) {
        t |> equal(regex_test_match("\\W", " "), 1)
        t |> equal(regex_test_match("\\W", "!"), 1)
        t |> equal(regex_test_match("\\W", "-"), 1)
    }
    t |> run("\\W rejects word chars") @(t : T?) {
        t |> equal(regex_test_match("\\W", "a"), -1)
        t |> equal(regex_test_match("\\W", "0"), -1)
        t |> equal(regex_test_match("\\W", "_"), -1)
    }
}

// ── \d (digit) ──────────────────────────────────────────────

[test]
def test_digit_class(t : T?) {
    t |> run("\\d matches digits") @(t : T?) {
        t |> equal(regex_test_match("\\d", "0"), 1)
        t |> equal(regex_test_match("\\d", "5"), 1)
        t |> equal(regex_test_match("\\d", "9"), 1)
    }
    t |> run("\\d rejects non-digits") @(t : T?) {
        t |> equal(regex_test_match("\\d", "a"), -1)
        t |> equal(regex_test_match("\\d", " "), -1)
    }
}

// ── \D (non-digit) ──────────────────────────────────────────

[test]
def test_non_digit_class(t : T?) {
    t |> run("\\D matches non-digits") @(t : T?) {
        t |> equal(regex_test_match("\\D", "a"), 1)
        t |> equal(regex_test_match("\\D", " "), 1)
        t |> equal(regex_test_match("\\D", "!"), 1)
    }
    t |> run("\\D rejects digits") @(t : T?) {
        t |> equal(regex_test_match("\\D", "0"), -1)
        t |> equal(regex_test_match("\\D", "5"), -1)
    }
}

// ── \s (whitespace) ─────────────────────────────────────────

[test]
def test_space_class(t : T?) {
    t |> run("\\s matches whitespace") @(t : T?) {
        t |> equal(regex_test_match("\\s", " "), 1)
        t |> equal(regex_test_match("\\s", "\t"), 1)
        t |> equal(regex_test_match("\\s", "\n"), 1)
        t |> equal(regex_test_match("\\s", "\r"), 1)
    }
    t |> run("\\s rejects non-whitespace") @(t : T?) {
        t |> equal(regex_test_match("\\s", "a"), -1)
        t |> equal(regex_test_match("\\s", "0"), -1)
    }
}

// ── \S (non-whitespace) ─────────────────────────────────────

[test]
def test_non_space_class(t : T?) {
    t |> run("\\S matches non-whitespace") @(t : T?) {
        t |> equal(regex_test_match("\\S", "a"), 1)
        t |> equal(regex_test_match("\\S", "0"), 1)
        t |> equal(regex_test_match("\\S", "!"), 1)
    }
    t |> run("\\S rejects whitespace") @(t : T?) {
        t |> equal(regex_test_match("\\S", " "), -1)
        t |> equal(regex_test_match("\\S", "\t"), -1)
        t |> equal(regex_test_match("\\S", "\n"), -1)
    }
}

// ── \t \n \r standalone escapes ─────────────────────────────

[test]
def test_escape_chars(t : T?) {
    t |> run("\\t matches tab") @(t : T?) {
        t |> equal(regex_test_match("\\t", "\t"), 1)
        t |> equal(regex_test_match("\\t", " "), -1)
    }
    t |> run("\\n matches newline") @(t : T?) {
        t |> equal(regex_test_match("\\n", "\n"), 1)
        t |> equal(regex_test_match("\\n", "n"), -1)
    }
    t |> run("\\r matches carriage return") @(t : T?) {
        t |> equal(regex_test_match("\\r", "\r"), 1)
        t |> equal(regex_test_match("\\r", "r"), -1)
    }
}

// ── character classes combined ──────────────────────────────

[test]
def test_combined_classes(t : T?) {
    t |> run("\\d+ matches digits") @(t : T?) {
        t |> equal(regex_test_match("\\d+", "12345"), 5)
        t |> equal(regex_test_match("\\d+", "abc"), -1)
    }
    t |> run("\\w+@\\w+ email-like") @(t : T?) {
        t |> equal(regex_test_match("\\w+@\\w+", "a@b"), 3)
    }
    t |> run("\\s+\\S+ whitespace then word") @(t : T?) {
        let inp = " hi"
        t |> equal(regex_test_match("\\s+\\S+", inp), 3)
    }
}

// ── hex escapes ─────────────────────────────────────────────

[test]
def test_hex_escapes_charclass(t : T?) {
    t |> run("hex in character set") @(t : T?) {
        t |> equal(regex_test_match("[\\x41-\\x5a]", "A"), 1)  // A-Z
        t |> equal(regex_test_match("[\\x41-\\x5a]", "M"), 1)
        t |> equal(regex_test_match("[\\x41-\\x5a]", "Z"), 1)
        t |> equal(regex_test_match("[\\x41-\\x5a]", "a"), -1)
    }
    t |> run("hex 2-digit standalone") @(t : T?) {
        t |> equal(regex_test_match("\\x41", "A"), 1)
        t |> equal(regex_test_match("\\x41", "B"), -1)
    }
    t |> run("hex 1-digit standalone") @(t : T?) {
        t |> equal(regex_test_match("\\x9", "\t"), 1)
    }
}

// ── classes inside character sets ───────────────────────────

[test]
def test_classes_in_sets(t : T?) {
    t |> run("\\d in set") @(t : T?) {
        t |> equal(regex_test_match("[\\d_]+", "123_456"), 7)
        t |> equal(regex_test_match("[\\d_]+", "abc"), -1)
    }
    t |> run("\\w in set") @(t : T?) {
        t |> equal(regex_test_match("[\\w.]+", "foo.bar"), 7)
    }
}
