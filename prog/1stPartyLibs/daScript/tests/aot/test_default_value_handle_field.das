options gen2
options gc
require dastest/testing_boost public
require daslib/fio

//! Regression: Boris's original failure case.
//!
//! Initializing a value-annotation handle (``ManagedValueAnnotation<T>``)
//! field with ``= default<T>`` triggers an ``ExprMakeLocal`` defaulting
//! path. In the AOT emitter, the resulting stack-temp expression carries
//! ``flags.ref = true`` but ``isRefType = false`` (POD), so
//! ``describeCppTypeEx`` emitted ``T *`` via the ``substitute_ref=true``
//! local-var pre-decl path while the consumer expected ``T &`` -- a
//! pointer-vs-reference mismatch that broke AOT C++ compilation.
//!
//! This test must compile under AOT and produce the expected runtime
//! result. It deliberately uses ``= default<clock>`` without the
//! ``@safe_when_uninitialized`` workaround Boris applied at the source
//! level, so a regression in the narrow stack-temp codegen surfaces here.

class ClockHolder {
    c : clock = default<clock>
    def init(t : clock) {
        c = t
    }
    def get() : clock {
        return c
    }
}

[test]
def test_default_value_handle_field(t : T?) {
    t |> run("class field = default<clock> compiles and works under AOT") @(t : T?) {
        var h = new ClockHolder()
        h->init(mktime(2024, 1, 1, 0, 0, 0))
        t |> equal(int64(h->get()), int64(mktime(2024, 1, 1, 0, 0, 0)))
    }
}
