if (__name__ == "__analysis__")
  return


function foo() { return "1" }
function bar(u) {}

let {x} = foo()


bar(x)
