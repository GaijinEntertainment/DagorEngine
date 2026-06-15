// Coverage: regex character classes and class matching in sqstdrex.cpp
// (sqstd_rex_charclass, sqstd_rex_matchcclass, escapechar) plus the regex
// compile-error paths (sqstd_rex_error) that the existing tests skip.

let { regexp } = require("string")

function m(pattern, str) {
  return regexp(pattern).match(str) ? "Y" : "N"
}

// explicit sets, ranges, negation
println("set:      " + m(@"[abc]+", "cabba"))          // Y
println("range:    " + m(@"[a-z]+", "hello"))          // Y
println("range2:   " + m(@"[A-Za-z0-9]+", "Ab9"))      // Y
println("neg:      " + m(@"[^0-9]+", "abc"))            // Y
println("neg.fail: " + m(@"[^0-9]+", "123"))           // N
println("mixed:    " + m(@"[a-c1-3]+", "a2c1"))        // Y

// predefined classes \d \D \w \W \s \S \l \u \x \p
println("d:  " + m(@"\d+", "2024"))                    // Y
println("D:  " + m(@"\D+", "abc"))                     // Y
println("w:  " + m(@"\w+", "ab_12"))                   // Y
println("W:  " + m(@"\W+", "!@#"))                     // Y
println("s:  " + m(@"\s+", "   "))                     // Y
println("S:  " + m(@"\S+", "xyz"))                     // Y
println("l:  " + m(@"\l+", "abc"))                     // Y
println("u:  " + m(@"\u+", "ABC"))                     // Y
println("x:  " + m(@"\x+", "1aF9"))                    // Y
println("p:  " + m(@"\p+", ",.;!"))                    // Y

// remaining predefined classes: \a \A \C \P \X (alpha/non-alpha/non-cntrl/
// non-punct/non-hexdigit) so sqstd_rex_matchcclass covers every case
println("a:  " + m(@"\a+", "abc"))                     // Y
println("A:  " + m(@"\A+", "123"))                     // Y (non-alpha)
println("C:  " + m(@"\C+", "abc"))                     // Y (non-control)
println("P:  " + m(@"\P+", "abc"))                     // Y (non-punct)
println("X:  " + m(@"\X+", "ghi"))                     // Y (non-hexdigit)

// escape sequences \t \n \r \f \v (sqstd_rex_escapechar)
println("esc-t: " + m(@"a\tb", "a\tb"))                // Y (tab)
println("esc-n: " + m(@"a\nb", "a\nb"))                // Y (newline)
println("esc-r: " + m(@"\r", "\r"))                    // Y
println("esc-f: " + m(@"\f", "\f"))                    // Y
println("esc-v: " + m(@"\v", "\v"))                    // Y

// predefined class inside a set
println("set+class: " + m(@"[\d\.]+", "3.14"))         // Y

// anchors, quantifiers, alternation, groups, escapes, dot
println("anchored:  " + m(@"^\d{4}$", "2024"))         // Y
println("quant{n,m}:" + m(@"a{2,4}", "aaa"))           // Y
println("alt:       " + m(@"(cat|dog)s?", "dogs"))     // Y
println("escdot:    " + m(@"a\.b", "a.b"))             // Y
println("dot:       " + m(@"a.c", "axc"))              // Y
println("optional:  " + m(@"colou?r", "color"))        // Y

// search returns begin/end positions
let r = regexp(@"\d+")
let found = r.search("abc123def")
println($"search begin/end: {found.begin}/{found.end}")  // 3/6
println($"search miss: {r.search("abcdef")}")            // (null)

// capture with sub-expressions
let cap = regexp(@"(\w+)=(\d+)").capture("port=8080")
println($"capture groups: {cap.len()}")                  // 3 (whole + 2 subs)

// --- compile errors (sqstd_rex_error) ---
function badRegex(label, pattern) {
  try { regexp(pattern); println($"FAIL: {label} compiled") }
  catch (e) { println($"err: {label}") }
}
badRegex("unbalanced-bracket", @"[abc")
badRegex("unbalanced-paren",   @"(abc")
badRegex("bad-quantifier",     @"a{")
badRegex("class-in-range",     @"[\d-z]")
badRegex("dangling-escape",    "abc\\")

println("regexp charclass: OK")
