options gen2

require dastest/testing_boost public

// Fixture for the --stack-on-exception regression test. A recursive panic gives
// a multi-frame call chain; with the flag set, the daslang runtime walks the
// stack AT THROW (frames intact) and prints "CALL STACK ..." to the worker's
// STDERR, which dastest's isolated runner folds into the failing test's log.
// Without the flag the post-unwind walk is suppressed, so no stack is emitted.
// Lives under a `_`-prefixed dir so the normal suite skips it — only
// test_isolated_mode.das runs it, as a spawned subprocess.
def private deep_panic(n : int) {
    if (n <= 0) {
        panic("intentional panic for the stack-walk fixture")
    }
    deep_panic(n - 1)
}

[test]
def test_panic_emits_stack(t : T?) {
    deep_panic(3)
    // deep_panic always panics, so this is unreachable. Fail loudly if a future
    // change ever lets it return — otherwise the fixture would pass silently and
    // test_isolated_mode's "no walk without the flag" assertion could go green for
    // the wrong reason (a fixture that never panicked).
    t |> success(false, "deep_panic returned without panicking")
}
