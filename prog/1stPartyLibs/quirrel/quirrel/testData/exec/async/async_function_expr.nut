from "async" import Promise

// `async function` is also valid in expression position. Anywhere a regular
// `function` expression appears (rvalue, table value, etc.), prefixing it
// with `async` produces a Promise-returning generator, same as the statement
// form `async function f() {...}`.

let f = async function() {
  let p = Promise()
  p.resolve("ok")
  let x = await p
  print("inner got: ")
  print(x)
  print("\n")
  return x
}

let task = f()
print("script done\n")
