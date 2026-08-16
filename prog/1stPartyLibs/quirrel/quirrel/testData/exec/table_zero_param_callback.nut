function assertNullThis() {
  assert(this == null)
}

function falseWithNullThis() {
  assert(this == null)
  return false
}

function trueWithNullThis() {
  assert(this == null)
  return true
}

let tableValues = { a = 1, b = 2 }

tableValues.each(assertNullThis)
assert(tableValues.findindex(falseWithNullThis) == null)
assert(tableValues.findvalue(falseWithNullThis) == null)
assert(tableValues.filter(trueWithNullThis).len() == 2)
let mappedTable = tableValues.map(trueWithNullThis)
assert(mappedTable.a && mappedTable.b)
assert(tableValues.reduce(trueWithNullThis, false))

let arrayValues = [1, 2]

arrayValues.each(assertNullThis)
assert(arrayValues.findindex(falseWithNullThis) == null)
assert(arrayValues.findvalue(falseWithNullThis) == null)
assert(arrayValues.filter(trueWithNullThis).len() == 2)
let mappedArray = arrayValues.map(trueWithNullThis)
assert(mappedArray[0] && mappedArray[1])
assert(arrayValues.reduce(trueWithNullThis, false))

println("ok")
