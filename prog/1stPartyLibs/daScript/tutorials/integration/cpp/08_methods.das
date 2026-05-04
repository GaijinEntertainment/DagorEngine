options gen2

// Companion script for the "Binding Methods" tutorial.
// The module "tutorial_08_cpp" exposes Counter and StringBuffer types
// with C++ methods bound as daslang functions.

require tutorial_08_cpp

[export]
def test() {
    // --- Counter: basic method calls ---
    print("=== Counter ===\n")

    unsafe {
        var c = make_counter(0, 1)
        print("initial: {get(c)}\n")

        increment(c)
        increment(c)
        increment(c)
        print("after 3 increments: {get(c)}\n")

        decrement(c)
        print("after decrement: {get(c)}\n")

        add(c, 10)
        print("after add(10): {get(c)}\n")

        print("is_positive: {is_positive(c)}\n")
        print("is_zero: {is_zero(c)}\n")

        // Change step and increment
        set_step(c, 5)
        increment(c)
        print("after set_step(5) + increment: {get(c)}\n")

        reset(c)
        print("after reset: {get(c)}\n")
        print("is_zero: {is_zero(c)}\n")
    }

    // --- Counter: pipe syntax (idiomatic daslang) ---
    print("\n=== Counter with pipe syntax ===\n")

    unsafe {
        var c = make_counter(100, 10)
        c |> decrement()
        c |> decrement()
        c |> decrement()
        print("100 - 3*10 = {c |> get()}\n")
    }

    // --- StringBuffer: method chaining ---
    print("\n=== StringBuffer ===\n")

    unsafe {
        var sb = make_string_buffer()
        print("empty: {buf_empty(sb)}\n")

        sb |> append("Hello")
        sb |> append(", ")
        sb |> append("World!")
        print("content: {c_str(sb)}\n")
        print("length: {buf_size(sb)}\n")

        sb |> clear()
        print("after clear, empty: {buf_empty(sb)}\n")

        sb |> append("daslang")
        print("content: {c_str(sb)}\n")
    }
}
