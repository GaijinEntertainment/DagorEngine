let { regexp } = require("string")

// Bug 1: Empty pattern should error, not crash (heap-buffer-overflow)
try {
  regexp("")
  println("BUG: empty pattern did not error")
} catch(e) {
  println($"empty pattern error: {e}")
}

// Bug 2: [A-] should error "unfinished range" (dead range-end check)
try {
  regexp("[A-]")
  println("BUG: [A-] did not error")
} catch(e) {
  println($"unfinished range error: {e}")
}

// Bug 3: Large quantifier should error, not silently truncate
try {
  regexp(@"^a{70000}$")
  println("BUG: large quantifier did not error")
} catch(e) {
  println($"quantifier error: {e}")
}

// Bug 4: Word boundary correctness at various positions
let wb = regexp(@"\bword\b")
println(wb.match("word"))
println(wb.search("hello word bye") != null)
println(wb.match("wordy"))

// Bug 5: Trailing backslash in escape should error
try {
  regexp("\\")
  println("BUG: trailing backslash did not error")
} catch(e) {
  println($"trailing backslash error: {e}")
}

// Bug 6: \m with incomplete args should error
try {
  regexp("\\m")
  println("BUG: incomplete \\m did not error")
} catch(e) {
  println($"incomplete \\m error: {e}")
}

try {
  regexp("\\m(")
  println("BUG: incomplete \\m( did not error")
} catch(e) {
  println($"incomplete \\m( error: {e}")
}

// Bug 7: Unclosed [ should error
try {
  regexp("[abc")
  println("BUG: unclosed [ did not error")
} catch(e) {
  println($"unclosed bracket error: {e}")
}

// Bug 8: Search finds matches at non-zero positions
let sr = regexp(@"ab")
let res = sr.search("xab")
println($"search begin={res.begin} end={res.end}")

// Bug 9: Character range matching correctness
let rng = regexp("[A-Z]")
println(rng.match("M"))
println(rng.match("a"))

// Bug 10: Range validation with escape sequences
try {
  let r = regexp("[A-\\n]")
  // Should not reach here - A(65) > \n(10) is an invalid range
  println("BUG: [A-\\n] should be invalid range")
} catch(e) {
  println($"[A-\\n] correctly rejected: {e}")
}

let r_az = regexp("[a-\\z]")
println($"[a-\\z] match m: {r_az.match("m")}")

// Bug 11: Pattern complexity limit
try {
  local longpat = "aaaaaaaaaa" // 10
  for (local i = 0; i < 12; i++)
    longpat += longpat // 10 * 2^12 = 40960
  longpat += longpat.slice(0, 9040) // 50000
  regexp(longpat)
  println("BUG: 50000 char pattern should error")
} catch(e) {
  println($"long pattern error: {e}")
}

// Bug 12: Nested zero-width quantifiers terminate
let zw = regexp(@"\b{0,3}word")
println($"zero-width bounded: {zw.match("word")}")

// Bug 14: {n,m} where n > m should error at compile time
try {
  regexp("a{5,2}")
  println("BUG: a{5,2} should be invalid range")
} catch(e) {
  println($"quantifier min>max error: {e}")
}

// Bug 15: Large quantifier packing correctness (UB fix for p0 >= 32768)
let lq = regexp(@"^a{40000}$")
local s40k = ""
for (local i = 0; i < 15; i++)
  s40k += "aaaaaaaaaa" // build in chunks of 10
// s40k = 150 chars, won't match {40000} but should not crash/UB
println($"large quantifier no crash: {lq.match(s40k) == false}")

// Bug 16: Greedy '*' must backtrack so the rest of the pattern can match
// (Squirrel upstream issue #181). Previously it stopped at the first following match,
// giving too-short matches or spurious failures.
let g1 = regexp("a.*bc")
foreach (s in ["a bc", "a bc bc", "a b bc"]) {
  let c = g1.capture(s)
  println(c == null ? "no match" : $"[{c[0].begin},{c[0].end}]")
}

// Bug 17: Greedy quantifiers inside capture groups backtrack correctly
// (Squirrel upstream issue #76): the digit run must be captured whole.
let g2 = regexp(@"(OAB)([^\d]{0,10})(\d[\d.]+)(.)")
let m = g2.capture("- OAB/SP no.:205844;")
foreach (i, v in m)
  println($"m{i}={"- OAB/SP no.:205844;".slice(v.begin, v.end)}")

// Bug 18: greedy '+' backtracks to leave input for a trailing literal
let g3 = regexp(@"\d+5")
let c3 = g3.capture("12345")
println(c3 == null ? "no match" : $"plus [{c3[0].begin},{c3[0].end}]")

// Bug 19: when a greedy repeat backtracks to fewer iterations, capture ranges
// must reflect the chosen count, not the deeper repetition of the maximal scan.
let g4 = regexp(@"((a))*a")
let c4 = g4.capture("aaa")
foreach (i, v in c4)
  println($"g{i}=[{v.begin},{v.end}]")

// Bug 20: a group that ends up matching zero times reports no range, not a
// stale range left over from the maximal scan.
let g5 = regexp(@"(b)*a")
let c5 = g5.capture("a")
foreach (i, v in c5)
  println($"z{i}=[{v.begin},{v.end}]")

// Bug 21: a greedy repeat inside a group backtracks through the enclosing
// continuation, not just the nodes remaining inside the group.
let g6 = regexp(@"(.*b)c")
let c6 = g6.capture("abcbx")
println(c6 == null ? "no match" : $"encl [{c6[0].begin},{c6[0].end}] g1=[{c6[1].begin},{c6[1].end}]")

// Bug 22: an alternation branch is retried when a node after it fails.
println($"alt retry: {regexp(@"a*(?:b|bc)d").match("abcd")}")

// Bug 23: captures written by failed continuation probes must not leak into
// the result when another alternation branch wins.
let g7 = regexp(@"((a)*b(c)d|a.*)")
let c7 = g7.capture("abcx")
println(c7 == null ? "no match" : $"leak g2=[{c7[2].begin},{c7[2].end}] g3=[{c7[3].begin},{c7[3].end}]")

// Bug 24: zero repetitions of a capturing group must not shift the capture
// slots of the groups that follow it.
let g8 = regexp(@"((a)(b))?(c)")
let c8 = g8.capture("c")
println(c8 == null ? "no match" : $"zero-rep g1=[{c8[1].begin},{c8[1].end}] g4=[{c8[4].begin},{c8[4].end}]")

// Bug 25: search also tries the empty position at the end of the input, so
// anchors and possibly-empty patterns match there like in Perl/PCRE/Python
println($"empty input: {regexp(@"^$").capture("") != null}")
let c9 = regexp(@"a*$").capture("bbb")
println($"empty at end: [{c9[0].begin},{c9[0].end}]")

// Bug 26: lazy quantifier syntax is rejected at compile time instead of
// silently treating the trailing '?' as a literal
try {
  regexp(@"(a*?)b")
  println("BUG: lazy quantifier compiled")
} catch(e) {
  println($"lazy quantifier error: {e}")
}

// Bug 27: backreference syntax is rejected at compile time instead of
// silently matching a literal digit
try {
  regexp(@"(a)\1")
  println("BUG: backreference compiled")
} catch(e) {
  println($"backreference error: {e}")
}

// Bug 28: a repeated atom retries its other alternatives on tail failure,
// not only smaller repetition counts
let c10 = regexp(@"(?:ab|a)*bc").capture("abc")
println(c10 == null ? "no match" : $"atom alt [{c10[0].begin},{c10[0].end}]")
let c11 = regexp(@"(ab|a)*bc").capture("abc")
println(c11 == null ? "no match" : $"atom alt cap [{c11[0].begin},{c11[0].end}] g1=[{c11[1].begin},{c11[1].end}]")

// Bug 29: match() requires covering the whole string through the matcher
// continuation, so alternation backtracks instead of failing the end check
println($"fullmatch alt: {regexp(@"(a|ab)").match("ab")}")
println($"fullmatch greedy: {regexp(@"a*aa").match("aa")}")
println($"fullmatch neg: {regexp(@"(a|ab)").match("ac")}")

// Bug 31: zero-width repetitions still satisfy a finite minimum count
println($"zw min empty: {regexp(@"(?:a?){2}").match("")}")
println($"zw min one: {regexp(@"(?:a?){2}").match("a")}")
println($"zw min over: {regexp(@"(?:a?){2}").match("aaa")}")
println($"zw min open: {regexp(@"(?:a?){2,}").match("")}")
let c12 = regexp(@"(a?){2}b").capture("b")
println(c12 == null ? "no match" : $"zw min cap [{c12[0].begin},{c12[0].end}] g1=[{c12[1].begin},{c12[1].end}]")

// Bug 32: the required minimum is reached even when a repetition is switched
// to a zero-width atom alternative during backtracking
println($"zw bt min: {regexp(@"(?:a?){2}a").match("a")}")
println($"zw bt two: {regexp(@"(?:a?){2}a").match("aaa")}")
println($"zw bt over: {regexp(@"(?:a?){2}a").match("aaaa")}")
let c13 = regexp(@"(a?){2}a").capture("a")
println(c13 == null ? "no match" : $"zw bt cap [{c13[0].begin},{c13[0].end}] g1=[{c13[1].begin},{c13[1].end}]")

// Bug 33: a transparent (?:...) wrapper around a simple atom repeats through
// the deterministic fast path, identical to the bare atom
println($"nocap unwrap: {regexp(@"(?:a)*b").match("aaab")}")
let c14 = regexp(@"x(?:\d)*y").capture("ax123yb")
println(c14 == null ? "no match" : $"nocap unwrap cap [{c14[0].begin},{c14[0].end}]")

// Bug 34: a captured single-width repeat (a)+ uses the deterministic fast
// path and reports the last iteration's char, unset when it matched zero times
let c15 = regexp(@"([0-9])+$").capture("num=12345")
println(c15 == null ? "no match" : $"cap w1 [{c15[0].begin},{c15[0].end}] g1=[{c15[1].begin},{c15[1].end}]")
let c16 = regexp(@"(a)*b").capture("b")
println(c16 == null ? "no match" : $"cap w1 zero g1=[{c16[1].begin},{c16[1].end}]")
let c17 = regexp(@"(\d)+3").capture("1233")
println(c17 == null ? "no match" : $"cap w1 bt [{c17[0].begin},{c17[0].end}] g1=[{c17[1].begin},{c17[1].end}]")

// Bug 30: runaway backtracking hits the step budget and raises an error
// instead of hanging or silently reporting a mismatch
local pathological = ""
for (local i = 0; i < 30; i++)
  pathological += "aaaaaaaaaa"
try {
  regexp(@".*.*.*b").capture(pathological)
  println("BUG: pathological match finished")
} catch(e) {
  println($"budget error: {e}")
}
