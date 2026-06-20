options gen2
options _comment_hygiene = true

//! Module-level header docstring — always allowed regardless of length.
//! Even though this `//!` block is more than three lines, STYLE014 must
//! treat it as the module-leading docstring and skip it. This is the
//! "regex_boost.das line 9" exception.

// good: 3-line `//` block at module scope is at the threshold (== 3, not >).
// STYLE014 fires only when count > 3.
// Three lines stays silent.
def good_threshold() {
    let _ = 1
}

// bad: 4-line block at module scope — fires once.
// Second line of the block.
// Third line of the block.
// Fourth line — block size now exceeds the 3-line cap.
def bad_module_scope() {
    let _ = 1
}

//! bad: 4-line `//!` docstring on a public function exceeds 3-line cap.
//! Second line.
//! Third line.
//! Fourth line.
def bad_public_doc() {
    let _ = 1
}

//!@nolint
//! Allowed: `//!@nolint` on the first line suppresses STYLE014 even
//! though this block has more than 3 doc lines. The marker line itself
//! is stripped from generated RST by `daslib/rst_comment`.
def good_suppressed_doc() {
    let _ = 1
}

// nolint:STYLE014
// Allowed: per-block `// nolint:STYLE014` on the first line suppresses
// STYLE014 even though this block has more than 3 lines.
// Fourth line.
// Fifth line.
def good_suppressed_plain() {
    let _ = 1
}

// good: trailing 1-line comments, even many of them, stay silent because
// each is a separate block (separated by blank lines).

// 1-line block.

// another 1-line block.
def good_one_liners() {
    let _ = 1
}
