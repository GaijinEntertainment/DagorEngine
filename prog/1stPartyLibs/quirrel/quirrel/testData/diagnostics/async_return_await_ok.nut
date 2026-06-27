// `return await <async-call>` forwards inner's value (await peels one level),
// so w336 does not fire (the arg is TO_AWAIT, not TO_CALL). inner is async,
// so the await is not redundant either.

async function inner() {
  return 42
}

async function outer() {
  return await inner()
}

outer()
