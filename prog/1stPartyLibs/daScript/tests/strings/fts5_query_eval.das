options gen2

require dastest/testing_boost
require daslib/fts5_query


[test]
def test_eval_subset(t : T?) {
    t |> run("Empty query matches everything") @(t : T?) {
        t |> equal(text_match("hello world", ""), true)
        t |> equal(text_match("", ""), true)
    }
    t |> run("Single term match") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "quick"), true)
        t |> equal(text_match("the quick brown fox", "slow"), false)
    }
    t |> run("Prefix term match") @(t : T?) {
        t |> equal(text_match("running jumps", "run*"), true)
        t |> equal(text_match("running jumps", "jump*"), true)
        t |> equal(text_match("running jumps", "walk*"), false)
        t |> equal(text_match("running jumps", "running*"), true)
    }
    t |> run("ASCII case fold (folded at parse and tokenize)") @(t : T?) {
        t |> equal(text_match("The Quick Brown Fox", "QUICK"), true)
        t |> equal(text_match("Running", "run*"), true)
    }
    t |> run("Punctuation tokenizes via each_word") @(t : T?) {
        t |> equal(text_match("the quick, brown fox.", "quick"), true)
        t |> equal(text_match("foo-bar baz", "foo"), true)
        t |> equal(text_match("foo-bar baz", "bar"), true)
    }
}


[test]
def test_eval_implicit_and(t : T?) {
    t |> run("Whitespace = AND") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "quick fox"), true)
        t |> equal(text_match("the quick brown fox", "quick dog"), false)
        t |> equal(text_match("the quick brown fox", "quick"), true)
    }
    t |> run("Order does not matter for AND") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "fox quick"), true)
    }
    t |> run("Empty terms are no-ops") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "  quick   "), true)
    }
}


[test]
def test_eval_or_not(t : T?) {
    t |> run("OR matches if either side matches") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "quick OR slow"), true)
        t |> equal(text_match("the quick brown fox", "slow OR fast"), false)
    }
    t |> run("NOT excludes the right side") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "fox NOT lazy"), true)
        t |> equal(text_match("the lazy dog", "lazy NOT dog"), false)
    }
    t |> run("Parens override precedence") @(t : T?) {
        // (a OR b) AND c
        t |> equal(text_match("alpha gamma", "(alpha OR beta) gamma"), true)
        t |> equal(text_match("beta gamma", "(alpha OR beta) gamma"), true)
        t |> equal(text_match("delta gamma", "(alpha OR beta) gamma"), false)
    }
}


[test]
def test_eval_phrase(t : T?) {
    t |> run("Phrase requires consecutive tokens") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "\"quick brown\""), true)
        t |> equal(text_match("the quick brown fox", "\"brown quick\""), false)
        t |> equal(text_match("the quick lazy brown fox", "\"quick brown\""), false)
    }
    t |> run("Phrase prefix matches last word as prefix") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "\"quick bro\"*"), true)
        t |> equal(text_match("the quick brown fox", "\"quick br\"*"), true)
        t |> equal(text_match("the quick brown fox", "\"quick xy\"*"), false)
    }
    t |> run("Single-word phrase = term") @(t : T?) {
        t |> equal(text_match("the quick brown fox", "\"quick\""), true)
    }
}


[test]
def test_eval_near(t : T?) {
    t |> run("NEAR(a b) with default k=10") @(t : T?) {
        // tokens within 10 positions
        t |> equal(text_match("alpha beta", "NEAR(alpha beta)"), true)
        t |> equal(text_match("alpha one two three beta", "NEAR(alpha beta)"), true)
    }
    t |> run("NEAR with explicit k limits proximity") @(t : T?) {
        t |> equal(text_match("alpha beta", "NEAR(alpha beta, 1)"), true)
        t |> equal(text_match("alpha one beta", "NEAR(alpha beta, 1)"), false)
        t |> equal(text_match("alpha one beta", "NEAR(alpha beta, 2)"), true)
    }
    t |> run("NEAR with three terms") @(t : T?) {
        t |> equal(text_match("alpha beta gamma", "NEAR(alpha beta gamma, 5)"), true)
        t |> equal(text_match("alpha gamma", "NEAR(alpha beta gamma, 5)"), false)
    }
}


[test]
def test_eval_unicode(t : T?) {
    t |> run("Latin-1 case folding through whole pipeline") @(t : T?) {
        t |> equal(text_match("Élève NIÑO café", "élève"), true)
        t |> equal(text_match("Élève NIÑO café", "niño"), true)
        t |> equal(text_match("Élève NIÑO café", "café"), true)
    }
    t |> run("Cyrillic AND") @(t : T?) {
        t |> equal(text_match("Привет Мир", "привет мир"), true)
        t |> equal(text_match("Привет Мир", "привет тест"), false)
    }
    t |> run("CJK ideographs as word chars") @(t : T?) {
        t |> equal(text_match("中文 日本語 한국어", "日本語"), true)
        t |> equal(text_match("中文，日本語", "中文"), true)
    }
}


[test]
def test_compile_then_match(t : T?) {
    t |> run("Pre-compiled query matches multiple texts") @(t : T?) {
        let q <- compile_text_match("quick fox")
        t |> equal(text_match("the quick brown fox", q), true)
        t |> equal(text_match("a quick wild fox", q), true)
        t |> equal(text_match("a slow tortoise", q), false)
    }
}
