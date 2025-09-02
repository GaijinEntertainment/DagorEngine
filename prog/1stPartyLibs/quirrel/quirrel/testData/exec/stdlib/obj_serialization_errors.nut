let { get_stack_top } = require("debug")
let { blob } = require("iostream")

let refStackTop = get_stack_top()

// Test 1: Maximum serialization depth exceeded
function testMaxDepth()
{
  local arr = []
  local current = arr
  for (local i = 0; i < 300; i++) {
    current.append([])
    current = current[0]
  }

  local b = blob()
  try {
    b.writeobject(arr)
    println("FAIL: Max depth test should have failed")
  } catch (e) {
    println("PASS: Max depth test - " + e)
  }
}

// Test 2: Unsupported class for serialization
function testUnsupportedClass()
{
  class Unsupported {
    x = 1
  }

  local obj = Unsupported()
  local b = blob()
  try {
    b.writeobject(obj)
    println("FAIL: Unsupported class test should have failed")
  } catch (e) {
    println("PASS: Unsupported class test - " + e)
  }
}

// Test 3: Missing __getstate method
function testMissingGetState()
{
  class NoGetState {
    x = 1
  }

  local obj = NoGetState()
  local b = blob()
  try {
    b.writeobject(obj, { NoGetState = NoGetState })
    println("FAIL: Missing __getstate test should have failed")
  } catch (e) {
    println("PASS: Missing __getstate test - " + e)
  }
}

// Test 4: Invalid __getstate method (not a closure)
function testInvalidGetState()
{
  class InvalidGetState {
    x = 1
    __getstate = 123 // not a function
  }

  local obj = InvalidGetState()
  local b = blob()
  try {
    b.writeobject(obj, { InvalidGetState = InvalidGetState })
    println("FAIL: Invalid __getstate test should have failed")
  } catch (e) {
    println("PASS: Invalid __getstate test - " + e)
  }
}

// Test 5: Invalid available classes (not a table)
function testInvalidAvailableClasses()
{
  class ValidClass {
    x = 1
    function __getstate() { return this.x }
    function __setstate(s) { this.x = s }
  }

  local obj = ValidClass()
  local b = blob()
  try {
    b.writeobject(obj, "not a table")
    println("FAIL: Invalid available classes test should have failed")
  } catch (e) {
    println("PASS: Invalid available classes test - " + e)
  }
}

// Test 6: Class not found in available classes during deserialization
function testClassNotFound()
{
  class ValidClass {
    x = 1
    function __getstate() { return this.x }
    function __setstate(s) { this.x = s }
  }

  local obj = ValidClass()
  local b = blob()
  b.writeobject(obj, { ValidClass = ValidClass })

  b.seek(0)
  try {
    b.readobject({}) // empty available classes
    println("FAIL: Class not found test should have failed")
  } catch (e) {
    println("PASS: Class not found test - " + e)
  }
}

// Test 7: Invalid constructor parameters
function testInvalidConstructor()
{
  class InvalidConstructor {
    x = 1
    constructor(_a, _b, _c) {} // requires 3 params
    function __getstate() { return this.x }
    function __setstate(s) { this.x = s }
  }

  local obj = InvalidConstructor(1,2,3)
  local b = blob()
  try {
    b.writeobject(obj, { InvalidConstructor = InvalidConstructor })
    b.seek(0)
    local x = b.readobject()
    println("FAIL: Invalid constructor test should have failed")
  } catch (e) {
    println("PASS: Invalid constructor test - " + e)
  }
}

// Test 8: Invalid start/end markers
function testInvalidMarkers()
{
  local b = blob()
  b.writen(1, 'c') // invalid start marker

  try {
    b.seek(0)
    b.readobject({})
    println("FAIL: Invalid start marker test should have failed")
  } catch (e) {
    println("PASS: Invalid start marker test - " + e)
  }

  // Test with valid start but invalid end marker
  local b2 = blob()
  b2.writeobject(123)

  // corrupt the end
  b2.seek(b.len() - 1)
  b2.writen(1, 'c')

  try {
    b2.seek(0)
    b2.readobject({})
    println("FAIL: Invalid end marker test should have failed")
  } catch (e) {
    println("PASS: Invalid end marker test - " + e)
  }
}

// Test 9: Unexpected end of data
function testUnexpectedEnd()
{
  local b = blob()
  b.writeobject([1,2,3])
  b.resize(b.len() - 2) // truncate last 2 bytes

  try {
    b.seek(0)
    b.readobject({})
    println("FAIL: Unexpected end test should have failed")
  } catch (e) {
    println("PASS: Unexpected end test - " + e)
  }
}

// Run all tests
testMaxDepth()
testUnsupportedClass()
testMissingGetState()
testInvalidGetState()
testInvalidAvailableClasses()
testClassNotFound()
testInvalidConstructor()
testInvalidMarkers()
testUnexpectedEnd()

let stackTop = get_stack_top()
assert(stackTop == refStackTop)
