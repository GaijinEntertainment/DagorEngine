function foo() {
  const x = "asdf"
  println(x)

  function bar() {
    let a = x
  }
  bar()
}
