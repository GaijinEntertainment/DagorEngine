from "async" import Future

// Regression: an L_Adopted waiter whose only ref lives in the parent's
// waiters list must survive the cascade through settleTerminal. Pre-fix,
// the cascade loop pushed a CascadeJob carrying a raw FutureImpl* and
// then sq_release'd the waiters-list ref, freeing the adopter before
// the worklist pop dereferenced it (use-after-free).

let outer = Future()
Future().resolve(outer)   // fresh adopter has no script binding; only ref is outer.waiters

// Same shape with a scoped local that goes out of scope before settle.
let other = Future()
{
  let tmp = Future()
  tmp.resolve(other)       // tmp adopts other; tmp_inst lives only in other.waiters
}

outer.resolve("done")
other.resolve("ok")

print("outer=")
print(outer.getState())
print(" other=")
print(other.getState())
print("\n")

print("script done\n")
