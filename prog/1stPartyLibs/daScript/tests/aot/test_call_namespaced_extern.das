options gen2
require dastest/testing_boost public
require UnitTest

// Regression for issue #3065 (fuzzer, AOT): a call to the UnitTest module function
// `testFoo`, which is defined inside `namespace das` but was declared nowhere in the
// module's AOT header. AOT-emitted callers `#include "unitTest.h"` then emit a bare
// `testFoo(...)` -> `error: use of undeclared identifier 'testFoo'`. Fixed by declaring
// testFoo in unitTest.h (inside namespace das, so AOT callers link). makeDummy sets
// fooData=1; testFoo sets it to 1234 -- so the side effect proves the call really runs.

[test]
def test_call_namespaced_extern(t : T?) {
    t |> run("aot call to namespace-das module extern testFoo") @(t : T?) {
        var foo = makeDummy()
        t |> equal(foo.fooData, 1)
        testFoo(foo)
        t |> equal(foo.fooData, 1234)
    }
}
