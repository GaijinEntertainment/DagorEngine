from "math" import sin, cos, abs
from "string" import format, printf
from "io" import *
import "datetime"
import "debug" as Debug

// Test that keywords work as identifiers
local import = 123
local from = [1, 2, 3]
function useKeywords() {
    println("import value:", import)
    println("from length:", from.len())
}

useKeywords()
println(sin(0), abs(-5))
