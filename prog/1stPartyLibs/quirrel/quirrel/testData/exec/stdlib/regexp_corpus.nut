let { regexp } = require("string")

// Behavioral corpus for the Quirrel (T-Rex) regex engine. Cross-checked against
// Python re on the shared syntax subset; the few intentional divergences from
// Perl/PCRE semantics are marked DIVERGE below. capture()[0] is the whole match,
// [1..] are the user groups; an unmatched group is reported as 0..0.
let cases = [
  // literals / anchors
  @"^abc",            "abcdef",
  @"abc$",            "abcz",
  @"a.c",             "axc",
  // classes
  @"[abc]+",          "zzabcabx",
  @"[^abc]+",         "abcXYZabc",
  @"[a-z]+",          "ABCdefGHI",
  @"[0-9]{2,4}",      "ab12345cd",
  @"\w+",             "  foo_bar123!! ",
  @"\d+",             "abc007xyz",
  @"\s+",             "ab   cd",
  // quantifiers
  @"a*",              "aaab",
  @"a*",              "bbb",
  @"a+",              "baaab",
  @"a?b",             "b",
  @"a{3}",            "aaaaa",
  @"a{2,3}",          "aaaaa",
  @"ab{0,2}c",        "ac",
  // groups & captures
  @"(a)(b)(c)",       "abc",
  @"(a(b)c)",         "abc",
  @"(?:abc)+",        "abcabc",
  @"(a*)(b+)",        "xxbb",       // empty group 1 keeps matched position 2..2
  @"(a)?b",           "b",          // optional group absent -> 0..0
  // alternation (leftmost-first, like Perl/Python, NOT POSIX-longest)
  @"abc|abd",         "abd",
  @"(a|ab)",          "ab",
  @"(foo|foobar)",    "foobar",
  // alternation must not leak captures from the losing branch
  @"x(a*)c|xb",       "xb",         // group 1 unmatched -> 0..0
  @"(a*)c|(b)",       "xb",         // group 1 unmatched, group 2 = "b"
  @"(ab)|(cd)|(ef)",  "cd",
  // a repeated alternation retries branches when the tail fails
  @"(ab|a)*bc",       "abc",        // one repetition of "a", then "bc"
  // word boundaries (\w transitions, not whitespace)
  @"\bfoo\b",         "foo!bar",
  @"\Bfoo",           "abfoo",
  // a repeated capturing group keeps the LAST iteration, like Perl/PCRE/Python
  @"(ab)+",           "ababab",
  // A capturing group AFTER a quantified group must still be recorded; the
  // greedy look-ahead must not drop it.
  @"(a)+(b)",         "aab",        // group 2 = "b" at 2..3
  @"(?:a)+(b)",       "aab",        // group 1 = "b" at 2..3
  @"(ab)+(cd)",       "ababcd",     // group 2 = "cd" at 4..6
  // empty input and the empty position at the end of the input are valid
  // match starts, like in Perl/PCRE/Python
  @"^$",              "",
  @"a?",              "",
  @"a*$",             "bbb",
  // T-Rex-only: \m balanced-delimiter match
  @"\m()",            "(a(b)c)d",
  @"\m()",            "x(a)b",
  @"\m()",            "(unbalanced",
  // T-Rex-only character classes
  @"\a+",             "12abcXY!!",
  @"\l+",             "ABCdefGHI",
  @"\u+",             "abcDEFghi",
  @"\p+",             "ab!?.,cd",
  @"\x+",             "zzDEADbeefGG",
]

for (local i = 0; i < cases.len(); i += 2) {
  let p = cases[i], s = cases[i + 1]
  let caps = regexp(p).capture(s)
  if (caps == null) {
    println($"/{p}/ on '{s}' -> NOMATCH")
  } else {
    local out = ""
    foreach (idx, m in caps) { if (idx != 0) out += ";"; out += $"{m.begin}..{m.end}" }
    println($"/{p}/ on '{s}' -> {out}")
  }
}
