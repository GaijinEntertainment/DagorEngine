// Tutorial 40: Coroutines
//
// This tutorial covers:
//   - What coroutines are (generator-based cooperative multitasking)
//   - The [coroutine] annotation
//   - co_continue() to yield control and keep running
//   - cr_run to drive a single coroutine to completion
//   - cr_run_all to drive multiple coroutines cooperatively
//   - co_await to wait for a sub-coroutine
//   - yeild_from to delegate to a sub-generator
//   - Practical example: cooperative task processing
//
// Prerequisites: Tutorial 15 (Iterators and Generators)
//
// Run: daslang.exe tutorials/language/40_coroutines.das

options gen2
options no_unused_function_arguments = false

require daslib/coroutines
require math

// ============================================================
// Section 1: Basic coroutines
// ============================================================
// A coroutine is a function annotated with [coroutine].
// Under the hood, it becomes a generator<bool> state machine.
// Use co_continue() instead of yield to signal "still running".

[coroutine]
def counting_coroutine() {
    print("  count: 1\n")
    co_continue()
    print("  count: 2\n")
    co_continue()
    print("  count: 3\n")
    // Falls through — coroutine finishes
}

// cr_run drives the coroutine to completion (consumes all yields).
def demo_basic_coroutine() {
    print("=== basic coroutine ===\n")
    // Create and run
    var c <- counting_coroutine()
    cr_run(c)
}

// ============================================================
// Section 2: Manual stepping
// ============================================================
// Since a coroutine is just an iterator<bool>, you can step
// through it manually with next().

[coroutine]
def step_coroutine() {
    print("  step A\n")
    co_continue()
    print("  step B\n")
    co_continue()
    print("  step C\n")
}

def demo_manual_stepping() {
    print("\n=== manual stepping ===\n")
    var c <- step_coroutine()
    for (running in c) {
        print("  -- yielded (running={running}) --\n")
    }
    print("  -- coroutine done --\n")
}

// ============================================================
// Section 3: Coroutine with state
// ============================================================
// Coroutines can have local variables that persist across yields.
// Each co_continue() suspends, and on resume the locals are intact.

[coroutine]
def countdown(n : int) {
    var i = n
    while (i > 0) {
        print("  {i}...\n")
        i --
        if (i > 0) {
            co_continue()
        }
    }
}

def demo_stateful_coroutine() {
    print("\n=== stateful coroutine ===\n")
    var c <- countdown(4)
    cr_run(c)
    print("  liftoff!\n")
}

// ============================================================
// Section 4: Running multiple coroutines with cr_run_all
// ============================================================
// cr_run_all takes an array of Coroutine (iterator<bool>) and
// drives them round-robin until all are done.

[coroutine]
def worker(name : string; steps : int) {
    for (i in range(steps)) {
        print("  {name}: step {i + 1}/{steps}\n")
        if (i < steps - 1) {
            co_continue()
        }
    }
}

def demo_run_all() {
    print("\n=== cr_run_all (cooperative scheduling) ===\n")
    var tasks : Coroutines
    tasks |> emplace <| worker("alpha", 3)
    tasks |> emplace <| worker("beta", 2)
    tasks |> emplace <| worker("gamma", 4)
    cr_run_all(tasks)
    print("  all tasks done\n")
}

// ============================================================
// Section 5: co_await — waiting for a sub-coroutine
// ============================================================
// co_await(sub) suspends the current coroutine until sub finishes.
// This is useful for composing coroutines hierarchically.

[coroutine]
def load_data() {
    print("  loading data... (tick 1)\n")
    co_continue()
    print("  loading data... (tick 2)\n")
    co_continue()
    print("  data loaded!\n")
}

[coroutine]
def process_pipeline() {
    print("  pipeline: start\n")
    co_continue()
    print("  pipeline: awaiting load_data\n")
    co_await <| load_data()
    print("  pipeline: processing loaded data\n")
    co_continue()
    print("  pipeline: done\n")
}

def demo_co_await() {
    print("\n=== co_await ===\n")
    var c <- process_pipeline()
    // Step manually to see the interleaving
    var step = 1
    for (running in c) {
        print("  -- tick {step} (running={running}) --\n")
        step ++
    }
    print("  -- pipeline finished --\n")
}

// ============================================================
// Section 6: yeild_from — delegating to a sub-generator
// ============================================================
// yeild_from(iter) yields all values from a sub-iterator.
// It works with any generator, not just coroutines.

def numbers_gen() : iterator<int> {
    return <- generator<int>() <| $() {
        yield 10
        yield 20
        yield 30
        return false
    }
}

def letters_gen() : iterator<int> {
    return <- generator<int>() <| $() {
        yield 100
        yield 200
        return false
    }
}

def combined_gen() : iterator<int> {
    return <- generator<int>() <| $() {
        yield 1
        yeild_from <| numbers_gen()
        yield 2
        yeild_from <| letters_gen()
        yield 3
        return false
    }
}

def demo_yield_from() {
    print("\n=== yeild_from ===\n")
    print("  combined: ")
    for (v in combined_gen()) {
        print("{v} ")
    }
    print("\n")
}

// ============================================================
// Section 7: Practical example — cooperative task processing
// ============================================================
// Simulate processing items in batches, with multiple workers
// cooperatively sharing time via coroutines.

[coroutine]
def batch_processor(name : string; total_items : int) {
    let batch_size = 2
    var pos = 0
    while (pos < total_items) {
        var end = min(pos + batch_size, total_items)
        print("  {name}: batch [{pos}..{end - 1}]\n")
        pos = end
        if (pos < total_items) {
            co_continue()
        }
    }
}

def demo_practical() {
    print("\n=== practical: cooperative batch processing ===\n")
    var tasks : Coroutines
    tasks |> emplace <| batch_processor("worker-A", 5)
    tasks |> emplace <| batch_processor("worker-B", 3)
    cr_run_all(tasks)
    print("  all batches done\n")
}

[export]
def main() {
    demo_basic_coroutine()
    demo_manual_stepping()
    demo_stateful_coroutine()
    demo_run_all()
    demo_co_await()
    demo_yield_from()
    demo_practical()
    print("done\n")
}

// expected output:
// === basic coroutine ===
//   count: 1
//   count: 2
//   count: 3
//
// === manual stepping ===
//   step A
//   -- yielded (running=true) --
//   step B
//   -- yielded (running=true) --
//   step C
//   -- coroutine done --
//
// === stateful coroutine ===
//   4...
//   3...
//   2...
//   1...
//   liftoff!
//
// === cr_run_all (cooperative scheduling) ===
//   gamma: step 1/4
//   beta: step 1/2
//   alpha: step 1/3
//   gamma: step 2/4
//   beta: step 2/2
//   alpha: step 2/3
//   gamma: step 3/4
//   alpha: step 3/3
//   gamma: step 4/4
//   all tasks done
//
// === co_await ===
//   pipeline: start
//   -- tick 1 (running=true) --
//   pipeline: awaiting load_data
//   loading data... (tick 1)
//   -- tick 2 (running=true) --
//   loading data... (tick 2)
//   -- tick 3 (running=true) --
//   data loaded!
//   pipeline: processing loaded data
//   -- tick 4 (running=true) --
//   pipeline: done
//   -- pipeline finished --
//
// === yeild_from ===
//   combined: 1 10 20 30 2 100 200 3
//
// === practical: cooperative batch processing ===
//   worker-B: batch [0..1]
//   worker-A: batch [0..1]
//   worker-B: batch [2..2]
//   worker-A: batch [2..3]
//   worker-A: batch [4..4]
//   all batches done
// done
