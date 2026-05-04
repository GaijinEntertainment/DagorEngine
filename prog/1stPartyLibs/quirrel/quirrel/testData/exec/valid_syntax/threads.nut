// Valid syntax: Threads/Coroutines

// Create thread
function thread_fn(a, b) {
    local val = suspend(a + b)
    return val * 2
}

let t = newthread(thread_fn)
assert(typeof t == "thread")

// Start thread
let first = t.call(3, 4)
assert(first == 7) // a + b = 7

// Resume thread
let result = t.wakeup(10)
assert(result == 20) // 10 * 2

// Thread status
function status_fn() {
    suspend("first")
    suspend("second")
    return "done"
}

let st = newthread(status_fn)
assert(st.getstatus() == "idle")
let v1 = st.call()
assert(v1 == "first")
assert(st.getstatus() == "suspended")
let v2 = st.wakeup()
assert(v2 == "second")
assert(st.getstatus() == "suspended")
let v3 = st.wakeup()
assert(v3 == "done")

// Multiple coroutines
function counter(start) {
    local n = start
    while (true) {
        suspend(n)
        n++
    }
}

let c1 = newthread(counter)
let c2 = newthread(counter)

assert(c1.call(0) == 0)
assert(c2.call(100) == 100)
assert(c1.wakeup() == 1)
assert(c2.wakeup() == 101)
assert(c1.wakeup() == 2)
assert(c2.wakeup() == 102)

// Passing values to suspended thread
function receiver() {
    local val = suspend("ready")
    return val
}

let recv = newthread(receiver)
let ready = recv.call()
assert(ready == "ready")
let final_val = recv.wakeup(42)
assert(final_val == 42)

print("threads: OK\n")
