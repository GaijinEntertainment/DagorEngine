options gen2
options indenting = 4
options gc
options persistent_heap
options no_aot

require dastest/testing_boost public

// pins the honest-frames marking contract: caller locals survive a collect no matter
// where in the chain it fires - inlined callee, same frame, block/lambda/function
// pointer indirection, argument evaluation, or an inlined callee's own for loop.

def churn_strings(tag : string) : int {
    var junk <- [for (i in range(200)); "junk/{tag}/segment_{i}/{i * 3}_tail"]
    var total = 0
    for (s in junk) {
        total += length(s)
    }
    delete junk
    return total
}

// tiny and inline-eligible on purpose: the collect call site gets spliced into the caller
def collect_every(var since : int&; period : int) {
    since++
    if (since >= period) {
        since = 0
        unsafe(heap_collect(true, true))
    }
}

[never_inline]
def apply_block_noinline(b : block) {
    var pad = 1
    invoke(b)
    pad++
}

def collect_now_regular() {
    var pad = 1
    unsafe(heap_collect(true, true))
    pad++
}

def make_files(var files : array<string>) {
    files |> reserve(60)
    for (d in range(12)) {
        for (i in range(5)) {
            files |> push("root/dir_{d}/file_{i}.das")
        }
    }
}

def verify_files(t : T?; files : array<string>; tag : string) {
    var idx = 0
    for (f in files) {
        if (f != "root/dir_{idx / 5}/file_{idx % 5}.das") {
            t |> failure("{tag}: corrupt entry at index {idx}")
            return
        }
        idx++
    }
    t |> success(idx == 60, "{tag}: walked all 60 entries")
}

// arg 0's initializer collects (validate on) while arg 1's temp slot is still raw
// stack garbage - its position interval must keep that slot invisible to the walk
def churn_and_collect_str(seed : int) : string {
    var junk <- [for (i in range(400)); "garbage_{seed}_{i}_padding_padding"]
    delete junk
    let keep = "kept_{seed}"
    unsafe(heap_collect(true, true))
    return keep
}

def two_args(a, b : string) : int {
    let pad = length(a) + length(b)
    return pad
}

// a collect while a LATER argument evaluates - the earlier temp holds the only reference.
// the two arms below are probabilistic instruments: a regression frees the temp's string,
// and the assert fires only when the freed block gets reused (pre-fix: 1 in 20 iterations)
def collect_and_tail(seed : int) : string {
    var junk <- [for (i in range(400)); "garbage2_{seed}_{i}_padding_padding"]
    delete junk
    unsafe(heap_collect(true, true))
    return "tail_{seed}"
}

def pick_first(a, _b : string) : string {
    let r = a
    return r
}

// inline-eligible callee whose own `for` hosts the collect - pins the iterator
// variable's position interval through the splice
def scan_with_collect(names : array<string>; var since : int&) : int {
    var total = 0
    for (nm in names) {
        total += length(nm)
        collect_every(since, 3)
    }
    return total
}

[test]
def test_collect_frame_marking(t : T?) {  // nolint:STYLE038 - flat list of independent test arms
    t |> run("collect in an inlined callee keeps caller locals") @(t : T?) {
        var files : array<string>
        make_files(files)
        var since = 0
        var churned = 0
        for (f in files) {
            churned += churn_strings(f)
            collect_every(since, 5)
        }
        verify_files(t, files, "inlined callee")
        t |> success(churned > 0)
    }
    t |> run("same-frame collect keeps locals") @(t : T?) {
        var files : array<string>
        make_files(files)
        var since = 0
        for (f in files) {
            churn_strings(f)
            since++
            if (since >= 5) {
                since = 0
                unsafe(heap_collect(true, true))
            }
        }
        verify_files(t, files, "same frame")
    }
    t |> run("collect through a real block invoke keeps caller locals") @(t : T?) {
        var files : array<string>
        make_files(files)
        var since = 0
        for (f in files) {
            churn_strings(f)
            since++
            if (since >= 5) {
                since = 0
                apply_block_noinline() {
                    unsafe(heap_collect(true, true))
                }
            }
        }
        verify_files(t, files, "block invoke")
    }
    t |> run("collect through a lambda keeps caller locals") @(t : T?) {
        var files : array<string>
        make_files(files)
        let lam <- @() {
            var pad = 1
            unsafe(heap_collect(true, true))
            pad++
        }
        var since = 0
        for (f in files) {
            churn_strings(f)
            since++
            if (since >= 5) {
                since = 0
                invoke(lam)
            }
        }
        verify_files(t, files, "lambda")
    }
    t |> run("collect through a function pointer keeps caller locals") @(t : T?) {
        var files : array<string>
        make_files(files)
        let fp = @@collect_now_regular
        var since = 0
        for (f in files) {
            churn_strings(f)
            since++
            if (since >= 5) {
                since = 0
                invoke(fp)
            }
        }
        verify_files(t, files, "function pointer")
    }
    t |> run("collect during argument evaluation of an inlined call") @(t : T?) {
        for (i in range(20)) {
            let r = two_args(churn_and_collect_str(i), "tail_{i}")
            t |> equal(r, length("kept_{i}") + length("tail_{i}"))
        }
    }
    t |> run("earlier argument temp survives a collect in a later argument") @(t : T?) {
        for (i in range(20)) {
            let kept = pick_first("kept_{i}", collect_and_tail(i))
            t |> equal(kept, "kept_{i}")
        }
    }
    t |> run("collect inside an inlined callee's for loop") @(t : T?) {
        var names : array<string>
        make_files(names)
        var since = 0
        var expected = 0
        for (nm in names) {
            expected += length(nm)
        }
        let total = scan_with_collect(names, since)
        t |> equal(total, expected)
        verify_files(t, names, "inlined for")
    }
}
