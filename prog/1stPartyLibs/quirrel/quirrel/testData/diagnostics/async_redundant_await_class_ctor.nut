// `await SomeClass()` produces a plain class instance, never a Promise, so
// the redundant-await diagnostic MUST still fire. Counterpart to
// async_redundant_await_instance_ok.nut (function annotated `: instance`).

class Widget {
  function constructor() {}
}

async function main() {
  let w = await Widget()
  print(w)
}

main()
