// Subclassing Future works through the internal async machinery, not just the
// public methods: getFutureFromInstance walks the base chain so a subclass
// instance is recognized by await / waiter-dispatch. Before that, `await sub`
// returned the wrapper object instead of the unwrapped value.

from "async" import Future

class MyFuture(Future) {}   // inherits Future's constructor

// Park on a PENDING subclass future, then settle it from another task: the
// awaiter must resume with the unwrapped value, not the wrapper instance.
let f = MyFuture()
async function awaiter() {
    let v = await f
    print("await subclass, value == 7: " + (v == 7) + "\n")
}
async function producer() {
    f.resolve(7)
}

awaiter()      // parks on the pending subclass future f
producer()     // settles f -> awaiter resumes
print("script done\n")
