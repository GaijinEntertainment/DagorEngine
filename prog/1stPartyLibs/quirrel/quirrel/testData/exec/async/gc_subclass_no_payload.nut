// The mark-hook is per-class and inherited by subclasses (SQClass ctor copies it
// from _base). A Future subclass whose constructor skips super.constructor() thus
// inherits the hook but never installs a FutureImpl (_userpointer stays NULL).
// SQInstance::Mark must guard on _userpointer before invoking the inherited hook,
// or the GC below would dereference NULL. This is the subclass counterpart to the
// _refs_table->mark-edge migration: it proves the inherited hook is null-safe.

from "async" import Future
let dbg = require("debug")

class Sub(Future) {
    constructor() {
        // intentionally skip super.constructor() -> no FutureImpl payload
    }
}

let s = Sub()
dbg.collectgarbage()    // marks `s`: inherited hook is reached but _userpointer is NULL
// `s` must still be alive here (held by this local) - so the GC really did walk it.
print("collected ok: " + (s != null) + "\n")
