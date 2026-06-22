options gen2
// STYLE005 is default-off via daslib/lint_config; force it on so this fixture
// can assert the 5 expected error sites below.
options _enable_default_off_rules = true
// STYLE005: braces around a single-statement early exit are noise
//
// Problem:
//   `if (cond) { return }` (or `{ break }` / `{ continue }`) wraps a single
//   terminator in a block — visual weight that says nothing. Two equivalent
//   forms are cleaner:
//     - braceless: `if (cond) return`
//     - postfix:   `return if (cond)` / `break if (cond)` / `continue if (cond)`
//
//   AST shape distinguishes braced vs braceless: the parser preserves the
//   block's LineInfo when source braces are present (Block.at points at the
//   `{`, distinct from the inner terminator's at). Synthetic blocks wrapping
//   a braceless body — and postfix-desugared bodies — share LineInfo with
//   the inner terminator, so neither fires this rule.
//
// Bad patterns:
//   if (cond) { return; }       // braced single-stmt return
//   if (cond) { break; }        // braced single-stmt break
//   if (cond) { continue; }     // braced single-stmt continue
//
// Good patterns:
//   if (cond) return            // braceless one-liner
//   return if (cond)            // postfix conditional
//   if (cond) { a; b }          // multi-statement block stays fine

expect 31209:5

require daslib/style_lint

// --- Bad: braced single-terminator ---

def bad_return_no_value(x : int) {
    if (x > 0) {
        return
    }
}

def bad_return_with_value(x : int) : int {
    if (x > 0) {
        return x
    }
    return -x
}

def bad_break_in_loop(arr : array<int>) {
    for (a in arr) {
        if (a < 0) {
            break
        }
    }
}

def bad_continue_in_loop(arr : array<int>) {
    for (a in arr) {
        if (a < 0) {
            continue
        }
        print("{a}\n")
    }
}

def bad_braced_one_liner(x : int) {
    if (x > 0) { return ; }  // one-line braced form — still bad
}

// --- Good: braceless and postfix forms ---

def good_braceless(x : int) {
    if (x > 0) return
}

def good_postfix(x : int) {
    return if (x > 0)
}

def good_postfix_with_value(x : int) : int {
    return x if (x > 0)
    return -x
}

def good_postfix_break(arr : array<int>) {
    for (a in arr) {
        break if (a < 0)
    }
}

def good_multi_stmt_block(x : int) {
    if (x > 0) {
        print("positive\n")
        return
    }
}
