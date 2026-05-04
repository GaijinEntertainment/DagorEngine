// Test for unused imported fields diagnostic
from "math" import sin, cos, tan
from "string" import format, printf
from "io" import *
import "datetime"
import "debug" as Debug

// Only sin and format are used
let _x = sin(0.5)
let _str = format("value: %d", 42)

// Unused imports (should trigger warnings):
// - cos, tan (from math)
// - printf (from string)
// - datetime (whole module)
// - Debug (whole module alias)
