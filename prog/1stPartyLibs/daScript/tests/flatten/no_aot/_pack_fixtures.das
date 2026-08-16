options gen2
options indenting = 4
options no_unused_function_arguments = false

require flat_pack_macro
require math

//! Fixtures for the scalar-packing pass (`pack_map_reduce` via `flatten_pack`). Each `[flat_pack]`
//! function produces a `*_packflat` twin (flattened → packed → folded). test_flatten_pack.das
//! compiles this file and asserts the min/max reduction re-rolls into `_pk*` vector vars, while
//! the fail-closed shapes (a const-threshold select, a lane that reads the accumulator) do NOT.

// PACKS: a min reduction over 4 independent, isomorphic scalar lanes (each lane = sin/mul chain
// of `seed` + a per-lane const). The unrolled `?:` min spine re-rolls into width-4 vector ops.
[flat_pack]
def pk_min4(seed : float) : float {
    var acc = 100.0
    for (i in range(4)) {
        let x = sin(seed + float(i) * 1.3) * 7.0
        let d = x * x
        acc = d < acc ? d : acc   // nolint:PERF015 — ternary-min spine is the exact shape the pass recognizes
    }
    return acc
}

// BAILS (classification, fail-closed): the select compares `d` against a CONST threshold, not the
// accumulator — `acc = (d < 5) ? d : acc` is NOT a min reduction. classify_link must reject it.
[flat_pack]
def pk_thresh(seed : float) : float {
    var acc = 100.0
    for (i in range(4)) {
        let x = sin(seed + float(i) * 1.3) * 7.0
        let d = x * x
        acc = d < 5.0 ? d : acc
    }
    return acc
}

// BAILS (independence, fail-closed): each lane reads the running accumulator (`sin(acc + ...)`),
// so the iterations are genuinely sequential — the per-lane independence check must reject it.
[flat_pack]
def pk_dep(seed : float) : float {
    var acc = 100.0
    for (i in range(4)) {
        let x = sin(acc + seed + float(i) * 1.3) * 7.0
        let d = x * x
        acc = d < acc ? d : acc   // nolint:PERF015 — ternary-min spine is the exact shape the pass recognizes
    }
    return acc
}
