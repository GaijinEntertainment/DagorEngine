from "async" import Future

// A subclass of Future whose constructor does not invoke super.constructor()
// leaves _userpointer NULL while _typetag still matches Future's. The
// script-callable methods used to dereference NULL on this; they must now
// throw "invalid 'this'" instead.

let class Sub(Future) {
  constructor() {
    // intentionally skip super.constructor()
  }
}

let s = Sub()

try { s.getState() }    catch (e) { print("getState: " + e + "\n") }
try { s.resolve(1) }    catch (e) { print("resolve: " + e + "\n") }

// A plain Future still works.
let p = Future()
print("plain: " + p.getState() + "\n")
