let { get_stack_top } = require("debug")
let { blob } = require("iostream")

let refStackTop = get_stack_top()

function testInvalidAvailableClasses()
{
  class ValidClass {
    x = 1
    function __getstate(_available_classes) { return this.x }
    function __setstate(s, _available_classes) { this.x = s }
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

function testClassNotFound()
{
  class ValidClass {
    x = 1
    function __getstate(_available_classes) { return this.x }
    function __setstate(s, _available_classes) { this.x = s }
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

function testInvalidConstructor()
{
  class InvalidConstructor {
    x = 1
    constructor(_a, _b, _c) {} // requires 3 params
    function __getstate(_available_classes) { return this.x }
    function __setstate(s, _available_classes) { this.x = s }
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

testInvalidAvailableClasses()
testClassNotFound()
testInvalidConstructor()

let stackTop = get_stack_top()
assert(stackTop == refStackTop)
