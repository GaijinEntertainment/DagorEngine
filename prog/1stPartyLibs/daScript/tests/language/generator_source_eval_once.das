options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/coroutines

// Pins the contract that a `for` source expression is evaluated exactly once.
// Shapes the double-eval bug hit: a yielding for-loop in a generator body, and the
// co_await/yeild_from macros that inline a call as a for source. Shapes that must not
// regress: array-returning call source, yieldless coroutine for-loop, comprehension.

var g_iter_count : int = 0
var g_arr_count : int = 0
var g_bump_count : int = 0

def noisy_iter() : iterator<int> {
    g_iter_count ++
    return <- generator<int> {
        yield 1
        yield 2
        yield 3
        return false
    }
}

def noisy_arr() : array<int> {
    g_arr_count ++
    return <- [10, 20, 30]
}

def bump(n : int) : int {
    g_bump_count ++
    return n
}

[coroutine]
def sub_task(_n : int) {
    co_continue()
}

[coroutine]
def awaiting_task() {
    co_await <| sub_task(bump(7))
}

[coroutine]
def delegating_gen() : int {
    yeild_from <| noisy_iter()
    return false
}

[coroutine]
def yieldless_for_sum() : int {
    var total = 0
    for (x in noisy_iter()) {
        total += x
    }
    yield total
    return false
}

[test]
def test_generator_for_source_eval_once(t : T?) {
    t |> run("iterator call in a yielding for-loop runs once") @(t : T?) {
        g_iter_count = 0
        var total = 0
        var gen <- generator<int> {
            for (x in noisy_iter()) {
                yield x
            }
            return false
        }
        for (v in gen) {
            total += v
        }
        t |> equal(g_iter_count, 1)
        t |> equal(total, 6)
    }
    t |> run("zip sources each run once") @(t : T?) {
        g_iter_count = 0
        g_arr_count = 0
        var total = 0
        var gen <- generator<int> {
            for (a, b in noisy_iter(), noisy_arr()) {
                yield a + b
            }
            return false
        }
        for (v in gen) {
            total += v
        }
        t |> equal(g_iter_count, 1)
        t |> equal(g_arr_count, 1)
        t |> equal(total, 66)
    }
    t |> run("co_await argument runs once") @(t : T?) {
        g_bump_count = 0
        var c <- awaiting_task()
        cr_run(c)
        t |> equal(g_bump_count, 1)
    }
    t |> run("yeild_from source runs once") @(t : T?) {
        g_iter_count = 0
        var total = 0
        for (v in delegating_gen()) {
            total += v
        }
        t |> equal(g_iter_count, 1)
        t |> equal(total, 6)
    }
    t |> run("array-returning call runs once") @(t : T?) {
        g_arr_count = 0
        var total = 0
        var gen <- generator<int> {
            for (x in noisy_arr()) {
                yield x
            }
            return false
        }
        for (v in gen) {
            total += v
        }
        t |> equal(g_arr_count, 1)
        t |> equal(total, 60)
    }
    t |> run("for-loop in a yieldless coroutine runs the source once") @(t : T?) {
        g_iter_count = 0
        var res = 0
        for (v in yieldless_for_sum()) {
            res = v
        }
        t |> equal(g_iter_count, 1)
        t |> equal(res, 6)
    }
    t |> run("comprehension source runs once") @(t : T?) {
        g_iter_count = 0
        let vals <- [for (x in noisy_iter()); x]
        t |> equal(g_iter_count, 1)
        t |> equal(length(vals), 3)
    }
}
