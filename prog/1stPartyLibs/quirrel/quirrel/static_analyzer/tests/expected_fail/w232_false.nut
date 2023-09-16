//expect:w232
local a = 1
::XEnum <- {
  PARAM_A = 1
  PARAM_B = 2
}
print(a == ::XEnum.PARAM_A && a == ::XEnum.PARAM_B)
