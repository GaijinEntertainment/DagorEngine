// `await` is only valid inside an `async function`. Using it in a plain
// function silently turned that function into a generator before; codegen
// now rejects it with a clear error.

function f() {
  let x = await 1
  return x
}

f()
