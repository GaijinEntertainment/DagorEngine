options gen2

require dastest/testing_boost public

// Fixture for the isolated-mode stderr-capture regression test. A hard C++
// crash (wild-pointer write) is unguarded by the daslang runtime, so it trips
// the OS crash handler (src/hal/crash_handler.cpp), which prints "CRASH: ..."
// plus a stack to the worker's STDERR. dastest's isolated runner must fold
// that stderr into the failing test's log instead of dropping it. Lives under
// a `_`-prefixed dir so the normal suite skips it — only test_isolated_mode.das
// runs it, as a spawned subprocess.
[test]
def test_crash(t : T?) {
    unsafe {
        var p = reinterpret<int?>(uint64(0xdeadbeef))
        *p = 13
    }
    t |> success(true)
}
