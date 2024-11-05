class A {
  function _get(key) {
    println("begin A._get() call")
    throw "I am an error!"
    println("end A._get() call")
  }
}

function testField() {
  let a = A()
  println($"a.foo = {a?.foo}")
}

function testIndex() {
  let a = A()
  println($"a[10] = {a?[10]}")
}

try {
  testField()
  println("testField failed")
} catch (e) {
  println("testField passed")
}

try {
  testIndex()
  println("testIndex failed")
} catch (e) {
  println("testIndex passed")
}