from "iostream" import casti2f
from "string" import format
from "math" import fabs

let cases = [
  [0.0, "0"],
  [casti2f(0x80000000), "-0"],
  [1.0, "1"],
  [-1.0, "-1"],
  [1.25, "1.25"],
  [-1.25, "-1.25"],
  [1.234567, "1.23457"],
  [-1.234567, "-1.23457"],
  [123456.0, "123456"],
  [1234567.0, "1.23457e+06"],
  [1234.125, "1234.12"],
  [casti2f(0xfb18b891), "-7.92973e+35"],
  [999999.0, "999999"],
  [999999.5, "1e+06"],
  [0.0001, "0.0001"],
  [0.00001, "1e-05"],
  [casti2f(0x00800000), "1.17549e-38"],
  [casti2f(0x7f7fffff), "3.40282e+38"],
  [casti2f(0xff7fffff), "-3.40282e+38"]
]

foreach (item in cases) {
  let value = item[0]
  let expected = item[1]
  assert(value.tostring() == expected)
  assert("".concat(value) == expected)
  assert($"{value}" == expected)
  println(expected)
}

foreach (bits in [0x00000001, 0x007fffff, 0x80000001, 0x807fffff,
    0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7fffffff, 0xffffffff]) {
  let value = casti2f(bits)
  assert(value.tostring() == format("%g", value))
}

local bits = 1
for (local i = 0; i < 4096; i++) {
  bits = (bits * 1664525 + 1013904223) & 0xffffffff
  let value = casti2f(bits)
  let exponent = bits & 0x7f800000
  let text = value.tostring()
  assert("".concat(value) == text)
  assert($"{value}" == text)
  if (exponent <= 0x00800000 || exponent == 0x7f800000)
    assert(text == format("%g", value))
  else
    assert(fabs(text.tofloat() / value - 1.0) < 0.0000052)
}

println("PASSED")
