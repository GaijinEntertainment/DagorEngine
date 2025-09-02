let { get_stack_top } = require("debug")
let { blob } = require("iostream")

function testValidStringIndex()
{
  // Create an array with multiple strings to test string table references
  local strings = [
    "first string",
    "second string",
    "third string"
  ]

  local b = blob()
  b.writeobject(strings)

  b.seek(0)
  try {
    local result = b.readobject({})

    // Verify all strings were deserialized correctly
    if (result.len() != 3) {
      println("FAIL: Valid string index test - wrong array length")
      return
    }

    if (result[0] != "first string" ||
      result[1] != "second string" ||
      result[2] != "third string") {
      println("FAIL: Valid string index test - string content mismatch")
      return
    }

    println("PASS: Valid string index test")
  } catch (e) {
    println("FAIL: Valid string index test - unexpected error: " + e)
  }
}

function testValidClassIndex()
{
  class TestClassA {
    value = null
    constructor(v = null) { this.value = v }
    function __getstate(_available_classes) { return this.value }
    function __setstate(v) { this.value = v }
  }
  class TestClassB {
    data = null
    constructor(d = null) { this.data = d }
    function __getstate() { return this.data }
    function __setstate(d, _available_classes) { this.data = d }
  }

  // Create an array with multiple instances to test class table references
  local objects = [
    TestClassA(100),
    TestClassB("test"),
    TestClassA(200),
    TestClassB("data")
  ]

  local b = blob()
  b.writeobject(objects, { TestClassA = TestClassA, TestClassB = TestClassB })

  b.seek(0)
  try {
    local result = b.readobject({ TestClassA = TestClassA, TestClassB = TestClassB })

    // Verify all objects were deserialized correctly
    if (result.len() != 4) {
      println("FAIL: Valid class index test - wrong array length")
      return
    }

    if (result[0].value != 100 ||
      result[1].data != "test" ||
      result[2].value != 200 ||
      result[3].data != "data") {
      println("FAIL: Valid class index test - object content mismatch")
      return
    }

    println("PASS: Valid class index test")
  } catch (e) {
    println("FAIL: Valid class index test - unexpected error: " + e)
  }
}

let refStackTop = get_stack_top()
// Run the valid tests
testValidStringIndex()
testValidStringIndex()
testValidClassIndex()
testValidClassIndex()
let stackTop = get_stack_top()
assert(refStackTop == stackTop)
