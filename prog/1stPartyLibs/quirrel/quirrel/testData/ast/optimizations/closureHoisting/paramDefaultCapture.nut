// Test 1: parameter default captures sibling from immediate parent scope.
// 'bar' captures 'foo' from immediate parent -> blocks hoisting of bar.
// 'foo' has no captures -> hoisted to root.

function root() {
  function foo(...) {
    return "xyz"
  }

  function bar(source=foo) {
    return source
  }

  return bar()
}

// Test 2: parameter default captures from grandparent, body local shadows same name.
// inner's default `a = x` captures outer's x. inner's body also declares `let x = 2`.
// The body local must NOT shadow the outer capture during analysis.
// inner should NOT be hoisted above outer.

function outer() {
  let x = 1
  function inner(a = x) {
    let x = 2
    return a + x
  }
  return inner
}
