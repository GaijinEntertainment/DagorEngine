options gen2

require strings
require daslib/strings_boost
require daslib/fio
require ./review_nttp // compile-time gate, no runtime symbol to reference
require dastest/review_gate

// The mechanical half of src/builtin/REVIEW.md (contract: REVIEW_COMMON.md at the
// repo root). Run from the repo root: bin/daslang src/builtin/REVIEW.das.

// The text between the first `opening` and the next `closing` after it, empty when
// either marker is missing - a missing marker is a finding, never a silent pass.
def private segment(text, opening, closing : string) : string {
    let a = find(text, opening)
    return "" if (a < 0)
    let rest = slice(text, a + length(opening))
    let b = find(rest, closing)
    return b < 0 ? "" : slice(rest, 0, b)
}

// Split on a delimiter that comes in pairs and keep what sits between each pair:
// backticks in the rule prose, double quotes in the gate's array literal.
def private delimited(seg, delim : string) : array<string> {
    var out : array<string>
    var inscope parts <- split(seg, delim)
    for (p, i in parts, count()) {
        out |> push(p) if (i % 2 == 1)
    }
    return <- out
}

// The Inline module list lives in two places by necessity: the rule is what a reviewer
// applies without reading code, review_nttp.das is what enforces it. A split between them
// makes the reviewer report a defect the gate cannot see, or miss one it does.
def private check_inline_module_list {
    let rule_list = segment(fread("src/builtin/REVIEW.md"), "The Inline modules are", ".")
    let gate_list = segment(fread("src/builtin/review_nttp.das"), "let inlineOnlyModules <- [", "]")
    var inscope from_rule <- delimited(rule_list, "`")
    var inscope from_gate <- delimited(gate_list, "\"")
    if (empty(from_rule)) {
        gate_finding("src/builtin/REVIEW.md", "no Inline module list found - the rule names it as \"The Inline modules are `a`, `b` ... .\"")
        return
    }
    if (empty(from_gate)) {
        gate_finding("src/builtin/review_nttp.das", "no inlineOnlyModules found - the gate's policy list is a literal array of module names")
        return
    }
    for (m in from_rule) {
        if (find_index(from_gate, m) < 0) {
            gate_finding("src/builtin/review_nttp.das", "REVIEW.md names {m} an Inline module; inlineOnlyModules does not")
        }
    }
    for (m in from_gate) {
        if (find_index(from_rule, m) < 0) {
            gate_finding("src/builtin/REVIEW.md", "inlineOnlyModules carries {m}; the rule's Inline module list does not name it")
        }
    }
}

[export]
def main {
    check_inline_module_list()
    return gate_verdict("src/builtin")
}
