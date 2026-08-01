let { regexp } = require("string")

let wb = regexp(@"\bfoo\b")
println($"word boundary before punctuation: {wb.search("foo!bar") != null}")

let emptyCap = regexp(@"(a*)(b+)").capture("xxbb")
println($"empty capture group: {emptyCap[1].begin}..{emptyCap[1].end}")

// Group 1 lives only in the losing alternative; the winning branch "xb" must
// leave it unmatched (0..0), not expose the stale begin from the failed branch.
let staleCap = regexp(@"x(a*)c|xb").capture("xb")
println($"unmatched group from failed branch: {staleCap[1].begin}..{staleCap[1].end}")

// Same, but the failed branch starts with the group (search retries at pos 1)
// and the winning branch carries its own capture: group 1 must be unmatched
// while group 2 still reports the matched "b".
let staleCap2 = regexp(@"(a*)c|(b)").capture("xb")
println($"unmatched g1, matched g2: {staleCap2[1].begin}..{staleCap2[1].end} {staleCap2[2].begin}..{staleCap2[2].end}")
