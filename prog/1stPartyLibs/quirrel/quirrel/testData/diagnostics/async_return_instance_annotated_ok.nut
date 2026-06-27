// An async function annotated to return an instance hands back the inner
// Future deliberately, so w336 opts out (result mask admits instance).
// Counterpart: async_return_future.nut.

async function inner() {
  return 42
}

async function outer(): instance {
  return inner()
}

outer()
