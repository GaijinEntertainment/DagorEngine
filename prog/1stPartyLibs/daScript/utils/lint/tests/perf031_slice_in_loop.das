options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _enable_default_off_rules = true   // fixture mode: immune to repo .lint_config rule toggles
// PERF031: slice/chop on a loop-invariant string inside a loop
//
// Problem:
//   `slice(s, i, j)` and `chop(s, i, n)` call strlen on the WHOLE source string
//   on every call (das strings cache no length), and the result is a fresh heap
//   string. Slicing the same string per iteration is O(length * iterations).
//
// Bad pattern:
//   while (i < n) {
//       let cp = slice(text, i, j)   // strlen(text) per iteration -> O(n^2)
//   }
//
// Good pattern:
//   peek_data(text) $(d) {                // one strlen up front
//       let cp = slice(d, i, j)           // the view carries its length
//   }

expect 31208:9

require daslib/perf_lint
require strings

def bad_slice_for(text : string) : int {
    var total = 0
    for (i in range(10)) {
        total += length(slice(text, i, i + 1))      // PERF031
    }
    return total
}

def bad_slice_while(text : string) : int {
    var total = 0
    var i = 0
    while (i < 8) {
        total += length(slice(text, i, i + 2))      // PERF031 (the shape that hid in the SPM tokenizer)
        i++
    }
    return total
}

def bad_chop_for(text : string) : int {
    var total = 0
    for (i in range(10)) {
        total += length(chop(text, i, 2))           // PERF031: chop shares the per-call strlen
    }
    return total
}

def bad_slice_in_block(text : string) : int {
    var total = 0
    for (_i in range(4)) {
        peek_data(text) $(d : array<uint8> const#) {
            total += length(slice(text, 0, length(d) % 4 + 1))   // PERF031: block args run inline in the loop
        }
    }
    return total
}

def bad_slice_in_block_loop(text : string) : int {
    var total = 0
    peek_data(text) $(d : array<uint8> const#) {
        for (i in range(length(d))) {
            total += length(slice(text, i, i + 1))  // PERF031: the loop inside an inline block runs in place
        }
    }
    return total
}

def bad_tail_return_in_block(text : string) : int {
    var total = 0
    for (_i in range(4)) {
        peek_data(text) $(d : array<uint8> const#) {
            total += length(slice(text, 0, length(d) % 3 + 1))   // PERF031: the return ends the block invocation, not the loop
            return
        }
    }
    return total
}

def with_length(nm : string; blk : block<(s : string) : int>) : int {
    return invoke(blk, nm)
}

def bad_return_slice_in_block(names : array<string>; text : string) : int {
    var total = 0
    for (nm in names) {
        total += with_length(nm) $(s) {
            return length(slice(text, 0, length(s) % 3 + 1))     // PERF031: same - a block return does not leave the loop
        }
    }
    return total
}

def bad_slice_in_returned_block(parts : array<string>; text : string) : string {
    return build_string() $(var w) {
        for (i in range(length(parts))) {
            w |> write(slice(text, i, i + 1))                    // PERF031: the loop lives inside the returned expression, so the return bounds nothing
        }
    }
}

def bad_tail_break_nested(lines : array<string>; text : string) : int {
    var total = 0
    for (_ln in lines) {
        for (i in range(8)) {
            if (i == 7) {
                total += length(slice(text, 0, i))  // PERF031: break leaves the inner loop only - repeats per outer iteration
                break
            }
        }
    }
    return total
}

def good_slice_of_loop_var(parts : array<string>) : int {
    var total = 0
    for (p in parts) {
        total += length(slice(p, 0, 1))             // bounded by the element, not the invariant source
    }
    return total
}

def with_name(nm : string; blk : block<(s : string) : void>) {
    invoke(blk, nm)
}

def good_slice_of_block_arg(names : array<string>) : int {
    var total = 0
    for (nm in names) {
        with_name(nm) $(s) {
            total += length(slice(s, 0, 1))         // block arg is a per-iteration value, not the invariant source
        }
    }
    return total
}

def good_block_local_string(parts : array<string>) : string {
    return build_string() $(var w) {
        for (p in parts) {
            let head = slice(p, 0, 2)               // let inside an inline block tracks like any local
            w |> write(head)
        }
    }
}

def good_return_slice(text : string) : int {
    for (i in range(16)) {
        if (i == 3) {
            return length(slice(text, i, i + 2))    // return leaves the loop - at most one slice per call
        }
    }
    return 0
}

def good_extract_and_return(text : string) : string {
    var found = ""
    for (i in range(16)) {
        if (i == 3) {
            found = slice(text, i, i + 2)           // block ends with return - one slice per call
            return found
        }
    }
    return found
}

def good_block_loop_return(text : string) : int {
    var total = 0
    peek_data(text) $(d : array<uint8> const#) {
        for (i in range(length(d))) {
            total += length(slice(text, i, i + 2))  // the block runs once and the return leaves its only loop
            return
        }
    }
    return total
}

def good_extract_and_break(text : string) : string {
    var found = ""
    for (i in range(16)) {
        if (i == 3) {
            found = slice(text, i, i + 2)           // break leaves the only loop - one slice per call
            break
        }
    }
    return found
}

def good_slice_as_for_source(text : string) : int {
    var total = 0
    for (i in range(length(slice(text, 4)))) {      // for sources evaluate once, at enclosing depth
        total += i
    }
    return total
}

def good_lambda_body(texts : array<string>; text : string) : array<lambda<() : int>> {
    var out : array<lambda<() : int>>
    out |> reserve(length(texts))
    for (_t in texts) {
        var lam = @() => length(slice(text, 0, 2))  // lambda body runs deferred, not in this loop
        out |> emplace(lam)
    }
    return <- out
}

def good_hoisted(text : string) : int {
    let head = slice(text, 0, 4)
    var total = 0
    for (_i in range(10)) {
        total += length(head)
    }
    return total
}

def good_no_loop(text : string) : int {
    return length(slice(text, 1, 3))
}
