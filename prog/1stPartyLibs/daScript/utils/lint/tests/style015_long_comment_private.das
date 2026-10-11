options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _comment_hygiene = true

//! STYLE015: comment block inside a `def private` over its cap.
//! A `//!` contract doc caps at 3 lines like a public doc; a plain `//`
//! block caps at one WHY line.

expect 31209:3

require daslib/style_lint

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

// good: 3-line `//!` contract doc inside `def private` is at the doc cap (== 3, not >).
def private good_private_contract_doc() {
    //! Returns nothing.
    //! Takes nothing.
    //! Never fails.
    let _ = 1
}

// bad: 4-line `//!` contract doc inside `def private` — over the 3-line doc cap.
def private bad_private_contract_doc() {
    //! Returns nothing.
    //! Takes nothing.
    //! Never fails.
    //! Fourth line — exceeds the doc cap.
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
