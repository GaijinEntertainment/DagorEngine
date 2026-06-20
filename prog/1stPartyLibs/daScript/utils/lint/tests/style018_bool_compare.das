options gen2
// STYLE018: redundant boolean comparison ('b == true' / 'b != false')
//
// Problem: Comparing a bool to a boolean literal is redundant — the bool already IS
// the value. Drop the comparison.
//
// Bad pattern:
//   if (flag == true)  { ... }
//   if (flag != false) { ... }
//
// Good pattern:
//   if (flag)          { ... }
//   if (!flag)         { ... }

expect 31209:6

require daslib/style_lint

// --- Bad patterns ---

def bad_eq_true(flag : bool) : bool {
    return flag == true                            // STYLE018 → flag
}

def bad_eq_false(flag : bool) : bool {
    return flag == false                           // STYLE018 → !flag
}

def bad_ne_true(flag : bool) : bool {
    return flag != true                            // STYLE018 → !flag
}

def bad_ne_false(flag : bool) : bool {
    return flag != false                           // STYLE018 → flag
}

def bad_yoda_eq_true(flag : bool) : bool {
    return true == flag                            // STYLE018 (Yoda)
}

def bad_yoda_eq_false(flag : bool) : bool {
    return false == flag                           // STYLE018 (Yoda)
}

// --- Good patterns (no warnings) ---

def good_direct(flag : bool) : bool {
    return flag
}

def good_negated(flag : bool) : bool {
    return !flag
}

def good_two_bools(a, b : bool) : bool {
    return a == b                                  // bool-bool compare — not flagged
}

def good_two_literals() : bool {
    return true == true                            // both literals — not flagged
}
