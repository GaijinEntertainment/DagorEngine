// `yield` inside an `async function` used to silently emit the same OP_YIELD
// as `await`, making the two observationally identical. Codegen now rejects
// `yield` here so users pick the right keyword.

async function f() {
  yield 1
}

f()
