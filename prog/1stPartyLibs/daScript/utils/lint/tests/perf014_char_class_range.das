options gen2
// PERF014: exact char-class range checks
//
// Problem:
//   Hand-written ranges reimplement the strings module's is_number / is_alpha
//   helpers. Only patterns that are EXACTLY equivalent to a helper are flagged,
//   so the suggested rewrite never changes behavior.
//
// Exact equivalences (is_alpha is DEFINED as the both-case union):
//   is_number(c)  <=>  c >= '0' && c <= '9'
//   is_alpha(c)   <=>  (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
// plus their De Morgan negations (out-of-range forms).
//
// Bad pattern:
//   if (c >= '0' && c <= '9') { ... }                         // → is_number
//   if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {}  // → is_alpha
//   if (c < '0' || c > '9') { ... }                           // → !is_number
//
// Good pattern:
//   if (is_number(c))  { ... }
//   if (is_alpha(c))   { ... }
//   if (!is_number(c)) { ... }
//
// A SINGLE case range ('a'..'z' alone) is NOT flagged — there is no is_lower /
// is_upper helper, so no exact rewrite exists. Hex extras ('a'..'f') are not
// flagged either (is_hex is broader).

expect 31208:9

require daslib/perf_lint
require strings

// --- Bad patterns (PERF014) ---

// Digit range — single bounded range, exact match to is_number.

def bad_digits_char(c : int) : bool {
    return c >= '0' && c <= '9'                // PERF014 (→ is_number)
}

def bad_digits_int(c : int) : bool {
    return c >= 48 && c <= 57                  // PERF014 (raw int form)
}

def bad_digits_yoda(c : int) : bool {
    return '0' <= c && c <= '9'                // PERF014 (const-on-left)
}

def bad_neg_digits(c : int) : bool {
    return c < '0' || c > '9'                  // PERF014 (negated → !is_number)
}

def bad_neg_digits_yoda(c : int) : bool {
    return '0' > c || '9' < c                  // PERF014 (negated, const-on-left)
}

def bad_neg_digits_cast(c : uint8) : bool {
    return int(c) < '0' || int(c) > '9'        // PERF014 (negated, with int() cast)
}

// Alpha — both-case compound, exact match to is_alpha.

def bad_alpha_compound(c : int) : bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')   // PERF014 (→ is_alpha)
}

def bad_neg_alpha_compound(c : int) : bool {
    return (c < 'a' || c > 'z') && (c < 'A' || c > 'Z')       // PERF014 (negated → !is_alpha)
}

def bad_alpha_compound_cast(c : uint8) : bool {
    // non-trivial checked expression — suggestion must name int(c), not c
    return (int(c) >= 'a' && int(c) <= 'z') || (int(c) >= 'A' && int(c) <= 'Z')   // PERF014 (→ is_alpha(int(c)))
}

// --- Good patterns (no warnings) ---

def good_is_number(c : int) : bool {
    return is_number(c)
}

def good_is_alpha(c : int) : bool {
    return is_alpha(c)
}

// Single-case ranges: no is_lower / is_upper helper exists, so NOT flagged.

def good_single_lower(c : int) : bool {
    return c >= 'a' && c <= 'z'                // single case — NOT flagged
}

def good_single_upper(c : int) : bool {
    return c >= 'A' && c <= 'Z'                // single case — NOT flagged
}

def good_neg_single_lower(c : int) : bool {
    return c < 'a' || c > 'z'                  // single case, negated — NOT flagged
}

def good_hex_lower(c : int) : bool {
    return c >= 'a' && c <= 'f'                // hex range — NOT flagged (is_hex broader)
}

def good_hex_upper(c : int) : bool {
    return c >= 'A' && c <= 'F'                // hex range — NOT flagged
}

def good_open_range(c : int) : bool {
    return c > '0' && c < '9'                  // open interval (&&-intersection) — NOT flagged
}

def good_neg_always_true(c : int) : bool {
    return c < '9' || c > '0'                  // degenerate always-true (lo>=hi) — NOT flagged
}

def good_partial_range(c : int) : bool {
    return c >= 'a' && c <= 'b'                // not a known class — NOT flagged
}

def good_different_vars(a, b : int) : bool {
    return a >= 'a' && b <= 'z'                // two different vars — NOT flagged
}

def good_alpha_compound_diff_vars(a, b : int) : bool {
    return (a >= 'a' && a <= 'z') || (b >= 'A' && b <= 'Z')   // halves on diff vars — NOT flagged
}

// Side-effectful subexpressions: replacing `f() >= '0' && f() <= '9'` with
// `is_number(f())` would call f() once instead of twice — silently changing
// semantics. The rule must NOT fire when operands aren't pure.

[unused_argument(c)]
def impure_helper(c : int) : int {
    print("called\n")                           // observable side effect
    return c
}

def good_impure_call(c : int) : bool {
    return impure_helper(c) >= '0' && impure_helper(c) <= '9'   // NOT flagged (impure)
}

def good_neg_impure(c : int) : bool {
    return impure_helper(c) < '0' || impure_helper(c) > '9'     // NOT flagged (impure, negated)
}
