// Test: async and coroutine interop
// Tests mixing [async] with [coroutine], awaiting coroutines
// from async functions, and running both with shared runners.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost
require daslib/coroutines
require strings

// ---- await coroutine from async ----

var g_coroutine_steps : int = 0

[coroutine]
def simple_coroutine(n : int) {
    for (i in range(n)) {
        g_coroutine_steps ++
        co_continue()
    }
}

[async]
def async_with_coroutine() : void {
    await_next_frame()
    await <| simple_coroutine(3)
    await_next_frame()
}

[test]
def test_await_coroutine(t : T?) {
    t |> run("await coroutine from async function") @(t : T?) {
        g_coroutine_steps = 0
        var it <- async_with_coroutine()
        async_run(it)
        t |> equal(g_coroutine_steps, 3)
    }
}

// ---- both in same file ----

[coroutine]
def counting_co() : int {
    yield 1
    yield 2
    yield 3
    return false
}

[async]
def counting_async() : int {
    yield 10
    yield 20
    yield 30
}

[test]
def test_both_in_same_file(t : T?) {
    t |> run("coroutine yields direct values") @(t : T?) {
        var vals : array<int>
        for (v in counting_co()) {
            vals |> push(v)
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 1)
        t |> equal(vals[1], 2)
        t |> equal(vals[2], 3)
    }
    t |> run("async yields variant-wrapped values") @(t : T?) {
        var vals : array<int>
        for (v in counting_async()) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 10)
        t |> equal(vals[1], 20)
        t |> equal(vals[2], 30)
    }
}

// ---- async_run on coroutine ----

var g_ran_steps : int = 0

[coroutine]
def work_coroutine() {
    g_ran_steps ++
    co_continue()
    g_ran_steps ++
    co_continue()
    g_ran_steps ++
}

[test]
def test_async_run_on_coroutine(t : T?) {
    t |> run("async_run drives coroutine to completion") @(t : T?) {
        g_ran_steps = 0
        var it <- work_coroutine()
        async_run(it)
        t |> equal(g_ran_steps, 3)
    }
}

// ---- nested: async awaits async which awaits coroutine ----

var g_deep_log : array<string>

[coroutine]
def deep_coroutine() {
    g_deep_log |> push("co:1")
    co_continue()
    g_deep_log |> push("co:2")
}

[async]
def mid_async() : void {
    g_deep_log |> push("mid:start")
    await <| deep_coroutine()
    g_deep_log |> push("mid:end")
}

[async]
def top_async() : void {
    g_deep_log |> push("top:start")
    await <| mid_async()
    g_deep_log |> push("top:end")
}

[test]
def test_deep_nesting(t : T?) {
    t |> run("async -> async -> coroutine nesting") @(t : T?) {
        g_deep_log |> clear()
        var it <- top_async()
        async_run(it)
        t |> equal(length(g_deep_log), 6)
        t |> equal(g_deep_log[0], "top:start")
        t |> equal(g_deep_log[1], "mid:start")
        t |> equal(g_deep_log[2], "co:1")
        t |> equal(g_deep_log[3], "co:2")
        t |> equal(g_deep_log[4], "mid:end")
        t |> equal(g_deep_log[5], "top:end")
    }
}

// ---- cr_run_all with mix of coroutines ----

var g_mix_log : array<string>

[coroutine]
def mix_co_a() {
    g_mix_log |> push("a:1")
    co_continue()
    g_mix_log |> push("a:2")
}

[coroutine]
def mix_co_b() {
    g_mix_log |> push("b:1")
    co_continue()
    g_mix_log |> push("b:2")
    co_continue()
    g_mix_log |> push("b:3")
}

[test]
def test_cr_run_all_coroutines(t : T?) {
    t |> run("cr_run_all with multiple coroutines") @(t : T?) {
        g_mix_log |> clear()
        var tasks : Coroutines
        tasks |> emplace <| mix_co_a()
        tasks |> emplace <| mix_co_b()
        cr_run_all(tasks)
        // Both should complete
        var a_count = 0
        var b_count = 0
        for (entry in g_mix_log) {
            if (starts_with(entry, "a:")) {
                a_count ++
            }
            if (starts_with(entry, "b:")) {
                b_count ++
            }
        }
        t |> equal(a_count, 2)
        t |> equal(b_count, 3)
    }
}
