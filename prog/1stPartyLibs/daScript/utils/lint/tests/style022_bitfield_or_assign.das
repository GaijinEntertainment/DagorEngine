options gen2
// STYLE022: bitfield compound-bit-set/clear → field-assign
//
// Problem: `foo |= BfT.m` and `foo &= ~BfT.m` are the mask-arithmetic forms
// of setting and clearing a single bitfield member. daslang lets you assign
// a bool directly to the member: `foo.m = true` / `foo.m = false`. The
// field form names the bit instead of the mask, drops the `~` for clears,
// and reads more naturally.
//
// Bad pattern:
//   var f : Mode
//   f |= Mode.read
//   f &= ~Mode.read
//
// Good pattern:
//   var f : Mode
//   f.read = true
//   f.read = false
//
// The rule fires only when the RHS resolves to exactly one named bit.
// Multi-bit masks (`Mode.read | Mode.write`) and dynamic RHS are left alone.

expect 31209:6

require daslib/style_lint

bitfield Mode {
    read
    write
    exec
}

struct Settings {
    flags : Mode
}

// --- Bad patterns ---

def bad_or_assign() {
    var f : Mode
    f |= Mode.read                              // STYLE022 — bit-set
}

def bad_and_clear() {
    var f : Mode
    f &= ~Mode.write                            // STYLE022 — bit-clear
}

def bad_or_struct_field() {
    var s : Settings
    s.flags |= Mode.exec                        // STYLE022 — bit-set on struct field
}

def bad_and_struct_field() {
    var s : Settings
    s.flags &= ~Mode.read                       // STYLE022 — bit-clear on struct field
}

def bad_chained() {
    var f : Mode
    f |= Mode.read                              // STYLE022
    f |= Mode.write                             // STYLE022 — second hit, same shape
}

// --- Good patterns (no warnings) ---

def good_field_assign() {
    var f : Mode
    f.read = true                               // idiomatic — no warning
    f.write = false                             // idiomatic — no warning
}

def good_multi_bit_mask() {
    var f : Mode
    f |= Mode.read | Mode.write                 // multi-bit RHS — left alone
}

def good_int_or() {
    var i : int = 0
    i |= 1                                      // not a bitfield — no warning
    i &= ~1                                     // not a bitfield — no warning
}

def good_and_without_complement() {
    var f : Mode
    f &= Mode.read                              // mask-down (rarely useful), but NOT the bit-clear idiom — no warning
}
