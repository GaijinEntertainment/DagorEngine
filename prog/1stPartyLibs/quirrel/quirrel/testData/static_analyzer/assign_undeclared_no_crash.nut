// Assigning to an undeclared name inside a block synthesizes a nodeless
// symbol. checkUnusedSymbols must not sort or report it by source location.
let arr = [1, 2, 3]
foreach (v in arr) {
  undeclaredVar = v
}

// Same, with several declared symbols in scope so the location sort runs.
foreach (v in arr) {
  let a = v
  let b = a + 1
  anotherUndeclared = b
  println(b)
}
