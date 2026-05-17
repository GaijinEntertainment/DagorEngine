options gen2
options indenting = 4
options persistent_heap
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require strudel/strudel
require math

// Measure per-query heap growth rate.
// Runs warmup queries, then two measurement batches.
// Returns (bytes_per_query_batch1, bytes_per_query_batch2).
// If batch2 rate is much lower than batch1, the allocator is reusing memory.
// If batch2 rate equals batch1, there's a real leak.
def measure_leak_rate(var pat : Pattern; warmup, batch : int) : tuple<int64; int64> {
    for (i in range(warmup)) {
        var haps <- pat(TimeSpan(start = double(i), stop = double(i + 1)))
        delete haps
    }
    let h1 = heap_bytes_allocated()
    for (i in range(batch)) {
        let cycle = warmup + i
        var haps <- pat(TimeSpan(start = double(cycle), stop = double(cycle + 1)))
        delete haps
    }
    let h2 = heap_bytes_allocated()
    for (i in range(batch)) {
        let cycle = warmup + batch + i
        var haps <- pat(TimeSpan(start = double(cycle), stop = double(cycle + 1)))
        delete haps
    }
    let h3 = heap_bytes_allocated()
    let rate1 = (int64(h2) - int64(h1)) / int64(batch)
    let rate2 = (int64(h3) - int64(h2)) / int64(batch)
    return rate1 => rate2
}

// With persistent_heap, pattern queries have zero steady-state leaks.
// Batch2 rate should be exactly 0. We allow 1 byte as a safety margin
// (integer division of 0 is still 0, but just in case).
let MAX_BYTES_PER_QUERY = 1l

[test]
def test_pure_memory(t : T?) {
    t |> run("pure pattern memory regression") @(t : T?) {
        let pat <- pure(Event(s = "bd"))
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_atom_memory(t : T?) {
    t |> run("atom pattern memory regression") @(t : T?) {
        let pat <- atom("sd")
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_stack_memory(t : T?) {
    t |> run("stack pattern memory regression") @(t : T?) {
        let pat <- stack([atom("bd"), atom("sd"), atom("hh"), atom("cp")])
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_cat_memory(t : T?) {
    t |> run("cat pattern memory regression") @(t : T?) {
        let pat <- cat([atom("bd"), atom("sd"), atom("hh"), atom("cp")])
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_fast_memory(t : T?) {
    t |> run("fast pattern memory regression") @(t : T?) {
        let pat <- fast(atom("bd"), 4.0lf)
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_fmap_memory(t : T?) {
    t |> run("fmap with lambda memory regression") @(t : T?) {
        let pat <- fmap(atom("bd"), @(e : Event) : Event {
            var r = e
            r.gain = 0.5
            return <- r
        })
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_degrade_memory(t : T?) {
    t |> run("degrade pattern memory regression") @(t : T?) {
        let pat <- degrade(atom("bd"), 0.5)
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_mini_memory(t : T?) {
    t |> run("mini-notation pattern memory regression") @(t : T?) {
        let pat <- s("bd sd [hh hh] cp")
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}

[test]
def test_complex_memory(t : T?) {
    t |> run("complex nested pattern memory regression") @(t : T?) {
        let pat <- stack([
            fast(s("bd sd [hh hh] cp"), 2.0lf),
            slow(atom("sd"), 2.0lf) |> set_gain(0.5),
            cat([atom("hh"), atom("cp")])
        ])
        let rates = measure_leak_rate(pat, 1000, 1000)
        t |> success(rates._1 <= MAX_BYTES_PER_QUERY, "batch2 rate {rates._1} bytes/query exceeds threshold")
    }
}
