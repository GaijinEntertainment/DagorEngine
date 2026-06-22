options gen2
// STYLE023: int_cast(bf & BfT.bit) ==/!= 0  →  [!]bf.bit
//
// Problem: testing a single bit via `uint(bf & BfT.bit) != 0u` (or any of
// the `int`/`uint`/`int64`/`uint64` cast forms compared to 0) is the
// mask-arithmetic counterpart of the bool field-access daslang already
// supports on bitfields. `bf.bit` is bool — read it directly.
//
// Bad pattern:
//   if (uint(io.flags & Mode.read) != 0u) { ... }
//   if (int(io.flags & Mode.write) == 0)  { ... }
//
// Good pattern:
//   if (io.flags.read)  { ... }
//   if (!io.flags.write) { ... }
//
// The rule fires only when the RHS of `&` resolves to exactly one named
// bit. Multi-bit masks are left alone (their `!= 0` semantics differ from
// any single field access).

expect 31209:6

require daslib/style_lint

bitfield Mode {
    read
    write
    exec
}

struct Io {
    flags : Mode
}

// --- Bad patterns ---

def bad_uint_cast_neq_zero(io : Io) : bool {
    return uint(io.flags & Mode.read) != 0u             // STYLE023 — `io.flags.read`
}

def bad_int_cast_neq_zero(io : Io) : bool {
    return int(io.flags & Mode.write) != 0              // STYLE023 — `io.flags.write`
}

def bad_uint_cast_eq_zero(io : Io) : bool {
    return uint(io.flags & Mode.exec) == 0u             // STYLE023 — `!io.flags.exec`
}

def bad_int64_cast_neq_zero(io : Io) : bool {
    return int64(io.flags & Mode.read) != 0l            // STYLE023 — `io.flags.read`
}

def bad_uint64_cast_eq_zero(io : Io) : bool {
    return uint64(io.flags & Mode.write) == 0ul         // STYLE023 — `!io.flags.write`
}

def bad_reversed_zero_lhs(io : Io) : bool {
    return 0u != uint(io.flags & Mode.exec)             // STYLE023 — reversed operands
}

// --- Good patterns (no warnings) ---

def good_direct_access(io : Io) : bool {
    return io.flags.read                                // idiomatic — no warning
}

def good_negated_direct(io : Io) : bool {
    return !io.flags.write                              // idiomatic — no warning
}

def good_multi_bit_mask(io : Io) : bool {
    return uint(io.flags & (Mode.read | Mode.write)) != 0u // multi-bit — left alone
}

def good_compare_to_non_zero(io : Io) : bool {
    return uint(io.flags & Mode.read) != 1u             // not `0` — left alone
}

def good_dynamic_mask(io : Io; mask : Mode) : bool {
    return uint(io.flags & mask) != 0u                  // dynamic RHS (not a named bit) — left alone
}
