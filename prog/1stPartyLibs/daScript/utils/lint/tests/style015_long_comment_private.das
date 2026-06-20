options gen2
options _comment_hygiene = true

//! STYLE015: comment block longer than 1 line inside a `def private`.
//! Reason: private functions don't get public docs, so multi-line prose
//! there is dead weight.

// good: 1-line `//` inside `def private` is at the cap (== 1, not >).
def private good_one_liner() {
    // single line — silent
    let _ = 1
}

// bad: 2-line `//` block inside `def private` — fires.
def private bad_two_lines() {
    // first line
    // second line — exceeds the 1-line cap inside private
    let _ = 1
}

// bad: 5-line `//` block deep inside private body.
def private bad_long_block() {
    // first line of multi-line WHY note
    // second line
    // third line
    // fourth line
    // fifth line
    let _ = 1
}

// good: same long block, suppressed with `// nolint:STYLE015` on first line.
def private good_suppressed() {
    // nolint:STYLE015
    // The marker on the first line tells STYLE015 to skip this block.
    // Third line.
    // Fourth line.
    let _ = 1
}

// good: 2-line `//` block in PUBLIC function is governed by STYLE014, not 015.
// First line.
// Second line.
def good_public_two_lines() {
    let _ = 1
}

// good: 2-line block at module scope stays silent (STYLE014 fires above 3).
// Second line.

def good_module_scope_two_lines() {
    let _ = 1
}
