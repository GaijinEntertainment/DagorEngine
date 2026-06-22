options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten

//! M4.1 differential tests: `break` / `continue` inside an unrolled loop. break
//! narrows a loop-scoped live mask that persists across unrolled copies (rest of the
//! breaking copy + every later copy masks off); continue narrows a per-copy mask
//! (rest of THIS copy masks off, next copy resets it). Each twin is run against its
//! original over an input grid AND (via the macro's cycle-2 scan) proven branchless
//! + call-free. The interesting interactions: break/continue composing with the
//! function-return mask, with nested loops (inner break must not touch the outer
//! loop), and with an inlined callee whose own early `return` must NOT kill any of
//! the caller's masks (frame isolation).

def sq(a : int) : int {
    return a * a
}

// ----- runtime break (depends on x) -----
[flatten]
def f_break(x : int) : int {
    var acc = 0
    for (i in range(6)) {
        if (i > x) {
            break
        }
        acc += i
    }
    return acc
}

// ----- runtime continue (depends on x) -----
[flatten]
def f_continue(x : int) : int {
    var acc = 0
    for (i in range(6)) {
        if (i == x) {
            continue
        }
        acc += i
    }
    return acc
}

// ----- const-foldable break (cond is a function of the unroll constant only) -----
[flatten]
def f_break_const(x : int) : int {
    var acc = 0
    for (i in range(8)) {
        if (i >= 4) {
            break
        }
        acc += x + i
    }
    return acc
}

// ----- const-foldable continue -----
[flatten]
def f_continue_const(x : int) : int {
    var acc = 0
    for (i in range(6)) {
        if (i % 2 == 0) {
            continue
        }
        acc += x * i
    }
    return acc
}

// ----- both in one loop -----
[flatten]
def f_break_and_continue(x : int) : int {
    var acc = 0
    for (i in range(8)) {
        if (i == 1) {
            continue
        }
        if (i > x) {
            break
        }
        acc += i
    }
    return acc
}

// ----- break inside a nested if (structPred non-null at the break) -----
[flatten]
def f_break_nested_if(x : int) : int {
    var acc = 0
    for (i in range(6)) {
        if (i > 0) {
            if (i * 2 > x) {
                break
            }
            acc += i
        }
    }
    return acc
}

// ----- nested loops, inner break only (inner mask must not stop the outer loop) -----
[flatten]
def f_nested_inner_break(x : int) : int {
    var acc = 0
    for (i in range(3)) {
        for (j in range(4)) {
            if (j > i) {
                break
            }
            acc += x + i * 10 + j
        }
        acc += 100          // runs every outer iteration — inner break must not reach here
    }
    return acc
}

// ----- nested loops, break in the OUTER body after the inner loop -----
[flatten]
def f_nested_outer_break(x : int) : int {
    var acc = 0
    for (i in range(4)) {
        for (j in range(2)) {
            acc += j
        }
        if (i >= x) {
            break
        }
        acc += i * 10
    }
    return acc
}

// ----- break then return: a return after the loop must still fire -----
[flatten]
def f_break_then_return(x : int) : int {
    var acc = 0
    for (i in range(5)) {
        if (i > x) {
            break
        }
        acc += i
    }
    return acc + 1000       // break leaves the loop; this return must run
}

// ----- return INSIDE the loop (interacts with break mask in the same body) -----
[flatten]
def f_return_in_loop(x : int) : int {
    var acc = 0
    for (i in range(6)) {
        if (i == x) {
            return acc + 50     // early function return from inside the loop
        }
        if (i >= 4) {
            break
        }
        acc += i
    }
    return acc
}

// ----- raymarch idiom: accumulate, break on a runtime hit, use the result after -----
[flatten]
def f_raymarch_like(x : int) : int {
    var hit = -1
    var steps = 0
    for (i in range(8)) {
        steps ++
        if (i * i >= x) {
            hit = i
            break
        }
    }
    return hit * 100 + steps
}

// ===== callee-return frame isolation =====
// An inlined callee's early `return` narrows the callee's OWN frame mask only. It
// must not kill the caller's funcLive / loopLive / iterLive — so the caller's work
// after the call (same copy, later copies, after the loop) still runs.

def hlp(x : int) : int {
    if (x > 5) {
        return 100          // early return inside the callee
    }
    return x * 2
}

// no loop: callee return must not kill the caller's funcLive
[flatten]
def f_callee_ret(x : int) : int {
    var acc = hlp(x)
    acc += x                // MUST run regardless of hlp's internal return
    return acc + 7          // …and this
}

def hlp2(x : int; i : int) : int {
    if (i > x) {
        return 0            // callee early-returns internally
    }
    return i
}

// inside a loop WITH break/continue: callee return must not touch loopLive/iterLive/funcLive
[flatten]
def f_loop_callee_ret(x : int) : int {
    var acc = 0
    for (i in range(5)) {
        if (i == 4) {
            continue
        }
        if (i > x + 3) {
            break
        }
        acc += hlp2(x, i)   // callee return must not kill any caller mask
        acc ++              // must run after the call (same copy)
    }
    acc += 100              // must run after the loop
    return acc
}

// ===== void entry: break gating a global write =====
var g_sink : int

def f_void_break_impl(x : int) {
    g_sink = 0
    for (i in range(6)) {
        if (i > x) {
            break
        }
        g_sink = g_sink + i
    }
}

[flatten]
def f_void_break(x : int) {
    g_sink = 0
    for (i in range(6)) {
        if (i > x) {
            break
        }
        g_sink = g_sink + i
    }
}

// ----- driver -----

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-3, -1, 0, 1, 2, 3, 4, 5, 7, 10]
}

[test]
def test_flatten_break_continue(t : T?) {
    t |> run("runtime break") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_break_flat(v), f_break(v))
        }
    }
    t |> run("runtime continue") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_continue_flat(v), f_continue(v))
        }
    }
    t |> run("const-foldable break") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_break_const_flat(v), f_break_const(v))
        }
    }
    t |> run("const-foldable continue") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_continue_const_flat(v), f_continue_const(v))
        }
    }
    t |> run("break and continue in one loop") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_break_and_continue_flat(v), f_break_and_continue(v))
        }
    }
    t |> run("break inside a nested if") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_break_nested_if_flat(v), f_break_nested_if(v))
        }
    }
    t |> run("nested loops, inner break only") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_nested_inner_break_flat(v), f_nested_inner_break(v))
        }
    }
    t |> run("nested loops, break in the outer body") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_nested_outer_break_flat(v), f_nested_outer_break(v))
        }
    }
    t |> run("break then a return after the loop") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_break_then_return_flat(v), f_break_then_return(v))
        }
    }
    t |> run("return inside the loop, alongside break") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_return_in_loop_flat(v), f_return_in_loop(v))
        }
    }
    t |> run("raymarch idiom: accumulate then break on hit") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_raymarch_like_flat(v), f_raymarch_like(v))
        }
    }
    t |> run("callee early return does not kill caller funcLive") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_callee_ret_flat(v), f_callee_ret(v))
        }
    }
    t |> run("callee return does not kill loop masks") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_loop_callee_ret_flat(v), f_loop_callee_ret(v))
        }
    }
    t |> run("void entry: break gates a global write") @(t : T?) {
        for (v in GRID) {
            f_void_break_impl(v)
            let ref = g_sink
            f_void_break_flat(v)
            t |> equal(g_sink, ref)
        }
    }
}
