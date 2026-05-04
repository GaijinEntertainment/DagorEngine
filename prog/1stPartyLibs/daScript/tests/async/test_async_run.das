// Test: async runner functions
// Tests async_run, async_run_all, async_timeout, async_race.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost
require strings

// ---- helpers ----

var g_log : array<string>

[async]
def logging_task(name : string; steps : int) : void {
    for (i in range(steps)) {
        g_log |> push("{name}:{i}")
        await_next_frame()
    }
}

[async]
def quick_task() : void {
    await_next_frame()
}

[async]
def slow_task() : void {
    for (i in range(10)) {
        await_next_frame()
    }
}

// ---- async_run ----

[test]
def test_async_run(t : T?) {
    t |> run("async_run drives single task to completion") @(t : T?) {
        g_log |> clear()
        var it <- logging_task("a", 3)
        async_run(it)
        t |> equal(length(g_log), 3)
        t |> equal(g_log[0], "a:0")
        t |> equal(g_log[1], "a:1")
        t |> equal(g_log[2], "a:2")
    }
}

// ---- async_run_all ----

[test]
def test_async_run_all(t : T?) {
    t |> run("async_run_all drives multiple tasks") @(t : T?) {
        g_log |> clear()
        var tasks : array<iterator<bool>>
        tasks |> emplace <| logging_task("x", 2)
        tasks |> emplace <| logging_task("y", 3)
        async_run_all(tasks)
        // Both tasks should have run to completion
        var x_count = 0
        var y_count = 0
        for (entry in g_log) {
            if (starts_with(entry, "x:")) {
                x_count ++
            }
            if (starts_with(entry, "y:")) {
                y_count ++
            }
        }
        t |> equal(x_count, 2)
        t |> equal(y_count, 3)
    }
}

// ---- async_timeout: completes ----

[test]
def test_async_timeout_completes(t : T?) {
    t |> run("async_timeout returns true when task completes in time") @(t : T?) {
        var it <- quick_task()
        let completed = async_timeout(it, 100)
        t |> success(completed)
    }
}

// ---- async_timeout: expires ----

[test]
def test_async_timeout_expires(t : T?) {
    t |> run("async_timeout returns false when task exceeds limit") @(t : T?) {
        var it <- slow_task()
        let completed = async_timeout(it, 3)
        t |> success(!completed)
    }
}

// ---- async_timeout: zero frames ----

[test]
def test_async_timeout_zero(t : T?) {
    t |> run("async_timeout with 0 frames always times out") @(t : T?) {
        var it <- quick_task()
        let completed = async_timeout(it, 0)
        t |> success(!completed)
    }
}

// ---- async_race: first finishes first ----

[test]
def test_async_race_first_wins(t : T?) {
    t |> run("async_race returns 0 when first task is faster") @(t : T?) {
        var a <- quick_task()
        var b <- slow_task()
        let winner = async_race(a, b)
        t |> equal(winner, 0)
    }
}

// ---- async_race: second finishes first ----

[test]
def test_async_race_second_wins(t : T?) {
    t |> run("async_race returns 1 when second task is faster") @(t : T?) {
        var a <- slow_task()
        var b <- quick_task()
        let winner = async_race(a, b)
        t |> equal(winner, 1)
    }
}

// ---- async_race: both same length ----

[async]
def two_frame_task() : void {
    await_next_frame()
    await_next_frame()
}

[test]
def test_async_race_tie(t : T?) {
    t |> run("async_race with equal tasks returns 0 (first checked)") @(t : T?) {
        var a <- two_frame_task()
        var b <- two_frame_task()
        let winner = async_race(a, b)
        // Both finish at same time, but 'a' is checked first
        t |> equal(winner, 0)
    }
}
