// Tutorial 49: Async / Await
//
// This tutorial covers:
//   - The [async] annotation (builds on Tutorial 40: Coroutines)
//   - Void async functions and await_next_frame()
//   - Typed async functions that yield values via variant
//   - await — waiting for an async result
//   - Struct returns with move semantics
//   - Iterating async generators
//   - Running tasks: async_run, async_run_all, async_timeout, async_race
//   - Mixing [async] with [coroutine]
//
// Prerequisites: Tutorial 15 (Iterators and Generators),
//                Tutorial 40 (Coroutines)
//
// Run: daslang.exe tutorials/language/49_async.das

options gen2
options no_unused_function_arguments = false

require daslib/async_boost
require daslib/coroutines

// ============================================================
// Section 1: Void async — the simplest form
// ============================================================
// An [async] function with `: void` return type becomes a
// generator<bool> state machine — identical to a [coroutine].
// Use await_next_frame() to suspend until the next step.

[async]
def greet(name : string) : void {
    print("  hello, ")
    await_next_frame()
    print("{name}!\n")
}

def demo_void_async() {
    print("=== void async ===\n")
    // Create the async task — returns iterator<bool>
    var it <- greet("world")
    // Step through manually
    var step = 1
    for (running in it) {
        print("  -- step {step} --\n")
        step ++
    }
    print("  -- done --\n")
}

// ============================================================
// Section 2: Typed async — yielding values
// ============================================================
// An [async] function with a non-void return type yields values
// wrapped in variant<res:T; wait:bool>. Each await_next_frame()
// yields variant(wait=true); each `yield value` yields
// variant(res=value).

[async]
def compute(x : int) : int {
    await_next_frame()  // simulate one frame of work
    yield x * 2
}

def demo_typed_async() {
    print("\n=== typed async ===\n")
    for (v in compute(21)) {
        if (v is wait) {
            print("  (waiting...)\n")
        } elif (v is res) {
            print("  result = {v as res}\n")  // 42
        }
    }
}

// ============================================================
// Section 3: Await — waiting for an async result
// ============================================================
// Inside an [async] function you can await another async call.
// `let result <- await <| fn(args)` suspends the parent until
// the child completes.

[async]
def add_one(x : int) : int {
    await_next_frame()
    yield x + 1
}

[async]
def chained_math() : void {
    var a = 0
    // Copy-assign await
    a = await <| add_one(0)     // a = 1
    // Move-assign await
    a <- await <| add_one(a)    // a = 2
    // Let-bind await
    let b <- await <| add_one(a) // b = 3
    print("  a={a}, b={b}\n")
}

def demo_await() {
    print("\n=== await ===\n")
    var it <- chained_math()
    async_run(it)
}

// ============================================================
// Section 4: Struct return with move semantics
// ============================================================
// Async functions can yield structs. Non-copyable data uses
// yield <- to move the result out.

struct Measurement {
    sensor_id : int
    value : float
    tag : string
}

[async]
def read_sensor(id : int) : Measurement {
    await_next_frame()  // simulate I/O delay
    var m : Measurement
    m.sensor_id = id
    m.value = 3.14
    m.tag = "temperature"
    yield <- m
}

[async]
def process_sensors() : void {
    let m <- await <| read_sensor(1)
    print("  sensor {m.sensor_id}: {m.value} ({m.tag})\n")
}

def demo_struct_return() {
    print("\n=== struct return ===\n")
    var it <- process_sensors()
    async_run(it)
}

// ============================================================
// Section 5: Iterating async generators
// ============================================================
// A typed async function can yield multiple values, acting as
// an asynchronous generator. Consumers iterate and check
// `v is res` to extract values.

[async]
def fibonacci_async(n : int) : int {
    var a = 0
    var b = 1
    for (i in range(n)) {
        yield a
        let next_val = a + b
        a = b
        b = next_val
        await_next_frame()  // simulate work each step
    }
}

def demo_async_iteration() {
    print("\n=== async iteration ===\n")
    print("  fibonacci: ")
    for (v in fibonacci_async(8)) {
        if (v is res) {
            print("{v as res} ")
        }
    }
    print("\n")
}

// ============================================================
// Section 6: Running tasks — async_run, async_run_all,
//            async_timeout, async_race
// ============================================================

[async]
def worker(name : string; frames : int) : void {
    for (i in range(frames)) {
        print("  {name}: frame {i + 1}/{frames}\n")
        await_next_frame()
    }
}

def demo_runners() {
    print("\n=== async_run ===\n")
    var single <- worker("solo", 3)
    async_run(single)

    print("\n=== async_run_all (cooperative) ===\n")
    var tasks : array<iterator<bool>>
    tasks |> emplace <| worker("alpha", 2)
    tasks |> emplace <| worker("beta", 3)
    async_run_all(tasks)
    print("  all done\n")

    print("\n=== async_timeout ===\n")
    var fast <- worker("fast", 2)
    let completed = async_timeout(fast, 10)
    print("  fast completed: {completed}\n")  // true

    var slow <- worker("slow", 100)
    let timed_out = async_timeout(slow, 3)
    print("  slow completed: {timed_out}\n")  // false (timeout)

    print("\n=== async_race ===\n")
    var racer_a <- worker("A", 2)
    var racer_b <- worker("B", 5)
    let winner = async_race(racer_a, racer_b)
    print("  winner: {winner}\n")  // 0 (A finishes first)
}

// ============================================================
// Section 7: Mixing async with coroutines
// ============================================================
// An [async] function can await a [coroutine]. This is useful
// for composing low-level coroutine logic with high-level
// async orchestration.

[coroutine]
def tick_counter(n : int) {
    for (i in range(n)) {
        print("  tick {i + 1}\n")
        co_continue()
    }
}

[async]
def orchestrator() : void {
    print("  orchestrator: start\n")
    await_next_frame()
    print("  orchestrator: awaiting coroutine\n")
    await <| tick_counter(3)
    print("  orchestrator: coroutine done\n")
    await_next_frame()
    print("  orchestrator: finish\n")
}

def demo_mixed() {
    print("\n=== mixing async + coroutine ===\n")
    var it <- orchestrator()
    async_run(it)
}

[export]
def main() {
    demo_void_async()
    demo_typed_async()
    demo_await()
    demo_struct_return()
    demo_async_iteration()
    demo_runners()
    demo_mixed()
    print("done\n")
}

// expected output:
// === void async ===
//   hello,   -- step 1 --
// world!
//   -- done --
//
// === typed async ===
//   (waiting...)
//   result = 42
//
// === await ===
//   a=2, b=3
//
// === struct return ===
//   sensor 1: 3.14 (temperature)
//
// === async iteration ===
//   fibonacci: 0 1 1 2 3 5 8 13
//
// === async_run ===
//   solo: frame 1/3
//   solo: frame 2/3
//   solo: frame 3/3
//
// === async_run_all (cooperative) ===
//   beta: frame 1/3
//   alpha: frame 1/2
//   beta: frame 2/3
//   alpha: frame 2/2
//   beta: frame 3/3
//   all done
//
// === async_timeout ===
//   fast: frame 1/2
//   fast: frame 2/2
//   fast completed: true
//   slow: frame 1/100
//   slow: frame 2/100
//   slow: frame 3/100
//   slow completed: false
//
// === async_race ===
//   A: frame 1/2
//   B: frame 1/5
//   A: frame 2/2
//   B: frame 2/5
//   winner: 0
//
// === mixing async + coroutine ===
//   orchestrator: start
//   orchestrator: awaiting coroutine
//   tick 1
//   tick 2
//   tick 3
//   orchestrator: coroutine done
//   orchestrator: finish
// done
