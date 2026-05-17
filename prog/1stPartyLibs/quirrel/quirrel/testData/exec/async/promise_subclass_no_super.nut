from "async" import Promise

// A subclass of Promise whose constructor does not invoke super.constructor()
// leaves _userpointer NULL while _typetag still matches Promise's. The
// script-callable methods used to dereference NULL on this; they must now
// throw "invalid 'this'" instead.

let class Sub(Promise) {
  constructor() {
    // intentionally skip super.constructor()
  }
}

let s = Sub()

try { s.getState() }    catch (e) { print("getState: " + e + "\n") }
try { s.resolve(1) }    catch (e) { print("resolve: " + e + "\n") }
try { s.reject("oops") } catch (e) { print("reject: " + e + "\n") }

// A plain Promise still works.
let p = Promise()
print("plain: " + p.getState() + "\n")
