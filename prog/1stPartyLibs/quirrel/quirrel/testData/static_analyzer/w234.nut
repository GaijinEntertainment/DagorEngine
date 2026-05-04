//expect:w234

function foo() { //-declared-never-used
  let zero = 0
  print(1 / (zero))
}
