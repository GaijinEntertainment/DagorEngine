options gen2
// LINT010: value stored in local variable is never read (dead store)
//
// Problem:
//   A local variable is written, but the value never reaches a read —
//   either it's overwritten by a later write with no intervening read,
//   or the variable goes out of scope (return / end-of-block) with no
//   subsequent read.
//
// Bad pattern:
//   var x = 1     // dead — overwritten on the next line
//   x = 2
//   return x
//
// Good pattern:
//   var x = 2
//   return x
//
// Scope of pass-1 (this file):
//   - Local variables only — not args, globals, captures, ref-bound.
//   - Skip vars whose address is taken, that are reference-bound, or
//     that are captured into a lambda (`Variable.capture_as_ref`).
//   - Only fires when the dead store's RHS is `flags.noSideEffects`.
//   - Block-local — no cross-branch / cross-loop analysis. The
//     `neutral_*` cases below are conservatively silent, not flagged.
//   - Boundary with LINT002 (unused variable, whole-variable): each
//     positive below ensures the variable IS read somewhere, so
//     LINT002 stays silent and the diagnostic surfaces as LINT010.

expect 50503:5

require daslib/lint

// === Helpers ===

def private compute_pure() : int {
    return 42
}

def private side_effect_int() : int {
    print("hi")
    return 1
}

def private take_addr(_p : int?) {
}

def private bumper(var v : int&) {
    v++
}

def private log(s : string) {
    print("{s}\n")
}

// === Positives — these SHOULD warn ===

// P1: dead init — overwritten before any read.
def bad_dead_init() : int {
    var x = 1       // LINT010: overwritten on next line; never read
    x = 2
    return x
}

// P2: dead intermediate write — init is live (read by print), final
// write is live (read by return), but the middle write is dead.
def bad_dead_intermediate(z : int) : int {
    var s = z
    print("{s}\n")          // read — keeps init live
    s = compute_pure()      // LINT010: overwritten on next line; never read
    s = z * 2
    return s
}

// P3: dead final write — function returns a different value.
def bad_dead_final_write(z : int) : int {
    var t = z
    print("{t}\n")          // read — keeps init live
    t = compute_pure()      // LINT010: scope ends with no further read
    return z
}

// P4: dead string init.
def bad_dead_string_init() {
    var msg = "init"        // LINT010: overwritten on next line; never read
    msg = "real"
    log(msg)
}

// P5: lambda captures x BY COPY at creation site — subsequent writes to the
// local x don't reach the lambda's struct frame, so `x = 2` is dead.
def bad_dead_after_copy_capture() : int {
    var x = 1
    let f = @() => x + 1
    x = 2                   // LINT010: never read; the lambda has its own copy
    return invoke(f)
}

// === Counter-examples — these should NOT warn ===

// C1: address taken → x escapes; analysis must bail on this var.
def good_addr_taken() {
    var x = 1
    take_addr(unsafe(addr(x)))
    x = 2
    print("{x}\n")
}

// C2: reference-bound alias → writes through r are writes to x; bail.
def good_ref_bound() : int {
    var x = 1
    var r & = x
    r = 5
    return x
}

// C3: passed by mutable reference → callee may read current value.
def good_ref_arg() : int {
    var x = 1
    bumper(x)               // callee may read x before writing
    return x
}

// C4: RHS has observable side effects → eliding would change behavior.
def good_rhs_has_side_effects() : int {
    var x = side_effect_int()
    x = 2
    return x
}

// C5: read between two writes makes the first one live.
def good_read_between() : int {
    var x = 1
    log("{x}")              // read — keeps x = 1 live
    x = 2
    return x
}

// C6: block-arg callback may not fire — init is the fallback path.
def good_block_arg_callback() : int {
    var qi = -1             // live on the "block didn't run" path
    let tab : table<int; int> <- { 7 => 42 }
    tab |> get(99) $(v) {
        qi = v
    }
    return qi
}

// === Out-of-scope — cross-block, pass-1 must NOT flag ===

// N1: write in only one branch — the init reaches the other path.
def neutral_write_in_branch(c : bool) : int {
    var x = 1               // live on the !c path; pass-1 must be silent
    if (c) {
        x = 2
    }
    return x
}

// N2: write inside a loop body — only the LAST iteration's value
// reaches the return; pass-1 cross-block-blind must not flag.
def neutral_loop_body() : int {
    var last = 0
    for (i in range(10)) {
        last = i
    }
    return last
}
