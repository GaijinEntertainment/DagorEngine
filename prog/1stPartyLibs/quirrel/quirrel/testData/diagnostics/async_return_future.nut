// `return <async-call>` without `await` settles outer's future WITH inner's
// future verbatim (no adoption), yielding a nested Future. Warns w336; the
// fix is `return await`. Counterpart: async_return_await_ok.nut.

async function inner() {
  return 42
}

async function outer() {
  return inner()
}

outer()
