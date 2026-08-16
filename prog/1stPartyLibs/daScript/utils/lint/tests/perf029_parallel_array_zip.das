options gen2
// PERF029: parallel-array indexing via range(length(X)) — zip the arrays instead
//
// Problem: `for (i in range(length(X)))` walking sibling arrays by subscript hides
// the length coupling — one unguarded sibling of a different length is an
// out-of-bounds panic. The zip form iterates in lockstep by construction.
//
// Bad pattern:
//   for (i in range(length(xs))) { ys[i] = xs[i] * 2.0 }
//
// Good pattern:
//   for (x, y in xs, ys) { y = x * 2.0 }

expect 31208:3

require daslib/perf_lint

def bad_two_arrays(xs : array<float>; var ys : array<float>) {
    for (i in range(length(xs))) {          // PERF029
        ys[i] = xs[i] * 2.0
    }
}

def bad_three_arrays(xs, ys : array<float>; var zs : array<float>) {
    for (i in range(length(xs))) {          // PERF029
        zs[i] = xs[i] + ys[i]
    }
}

def bad_two_arrays_long(xs : array<float>; var ys : array<float>) {
    for (i in range64(long_length(xs))) {   // PERF029 - 64-bit spelling
        ys[i] = xs[i] * 2.0
    }
}

def good_zip(xs : array<float>; var ys : array<float>) {
    for (x, y in xs, ys) {
        y = x * 2.0
    }
}

def good_offset_stays_silent(xs : array<float>; var ys : array<float>) {
    // i appears in index arithmetic - the zip form cannot express it
    for (i in range(length(xs) - 1)) {
        ys[i] = xs[i + 1]
    }
}

def good_index_needed(xs : array<float>; var total : float&) {
    // i escapes as a value - not zippable
    for (i in range(length(xs))) {
        total += xs[i] * float(i)
    }
}

[export]
def main {
    var xs, ys, zs : array<float>
    xs |> resize(4)
    ys |> resize(4)
    zs |> resize(4)
    var total = 0.0
    bad_two_arrays(xs, ys)
    bad_three_arrays(xs, ys, zs)
    bad_two_arrays_long(xs, ys)
    good_zip(xs, ys)
    good_offset_stays_silent(xs, ys)
    good_index_needed(xs, total)
}
