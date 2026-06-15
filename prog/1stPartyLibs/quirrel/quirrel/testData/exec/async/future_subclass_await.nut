// Subclassing Future works through the internal async machinery, not just the
// public methods: getFutureFromInstance now walks the base chain so a subclass
// instance is recognized by await / waiter-dispatch / chain-unwrap. Before that,
// `await sub` returned the wrapper object and `return sub` did not chain-unwrap.
// See the TODO(review later) note in sqasync.cpp.

from "async" import Future

class MyFuture(Future) {}   // inherits Future's constructor

// 1) Park on a PENDING subclass future, then settle it from another task: the
//    awaiter must resume with the unwrapped value, not the wrapper instance.
let f = MyFuture()
async function awaiter() {
    let v = await f
    print("await subclass, value == 7: " + (v == 7) + "\n")
}
async function producer() {
    f.resolve(7)
}

// 2) Chain-unwrap: returning a subclass future from an async fn adopts its value.
async function viaReturn() {
    let inner = MyFuture()
    inner.resolve(9)
    return inner
}
async function unwrapper() {
    let v = await viaReturn()
    print("chain-unwrap subclass, value == 9: " + (v == 9) + "\n")
}

awaiter()      // parks on the pending subclass future f
producer()     // settles f -> awaiter resumes
unwrapper()    // awaits a task that chain-unwraps a subclass future
print("script done\n")
