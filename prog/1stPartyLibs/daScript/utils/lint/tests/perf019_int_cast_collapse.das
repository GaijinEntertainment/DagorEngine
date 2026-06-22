options gen2
// PERF019: int(T.a) | int(T.b) on bitfield/enum-with-operator-| can collapse to int(T.a | T.b)
//
// Problem:
//   `int(T.a) | int(T.b)` performs two int casts and one OR on ints. When the
//   underlying type T natively supports `|` — bitfields always do; enums when
//   an `operator |` overload exists — one cast suffices: `int(T.a | T.b)`.
//
// Bad pattern:
//   def configure(modes : Mode) : int {
//       return int(modes) | int(Mode.read)              // PERF019
//   }
//
// Good pattern:
//   def configure(modes : Mode) : int {
//       return int(modes | Mode.read)
//   }
//
// Note for dastest readers: this test exercises only the runtime forms.
// The "canonical" all-constant form `int(T.a) | int(T.b)` is const-folded
// to a single ExprConstInt during inference under normal compile policies
// (which dastest uses), so the rule has nothing to attach to here. Under
// the lint runner — which sets `lint_check=true`, `no_optimizations=true`,
// `no_infer_time_folding=true` — the const form is preserved and the rule
// fires. End-to-end coverage of the const-fold case lives in the CI lint
// gate run, not in this dastest file.

expect 31208:3

require daslib/perf_lint
require strings

bitfield Mode {
    read
    write
    exec
}

bitfield Other {
    one
    two
}

enum EnumOr : int {
    a = 1
    b = 2
    c = 4
}

def operator |(x, y : EnumOr) : EnumOr {
    // Test fixture: the rule probes the program for `operator |` on EnumOr.
    // Body semantics are irrelevant — only the symbol's existence matters.
    return unsafe(reinterpret<EnumOr>(int(x) | int(y))) // nolint:PERF019 — fixture body, not the rule's target
}

enum EnumNoOr : int {
    a = 1
    b = 2
}

// --- Bad patterns (runtime — survive folding) ---

def bad_bitfield_runtime(m1, m2 : Mode) : int {
    return int(m1) | int(m2)                                // PERF019 — bitfield
}

def bad_enum_with_or(a, b : EnumOr) : int {
    return int(a) | int(b)                                  // PERF019 — enum with operator|
}

def bad_bitfield_chain(m1, m2, m3 : Mode) : int {
    // Left-associative: outer `(int|int) | int(m3)` has lhs=ExprOp2 (not an ExprCall),
    // so unwrap_int_cast fails and the outer pair is skipped. Inner pair fires once.
    return int(m1) | int(m2) | int(m3)                      // PERF019 — inner pair only
}

// --- Good patterns (no warnings) ---

def good_mismatched_bitfields(m : Mode; o : Other) : int {
    return int(m) | int(o)                                  // different bitfield types — rewrite would not compile
}

def good_enum_without_or(a, b : EnumNoOr) : int {
    return int(a) | int(b)                                  // no operator| on EnumNoOr — rewrite would not compile
}

def good_nested_cast(widen : int64; m2 : Mode) : int {
    return int(widen) | int(m2)                             // outer LHS argument is int64 (not bitfield/enum) — PERF019's unwrap succeeds then inner_type_supports_or bails
}

def good_plain_ints(x, y : int) : int {
    return x | y                                            // neither side is an int(...) cast
}
