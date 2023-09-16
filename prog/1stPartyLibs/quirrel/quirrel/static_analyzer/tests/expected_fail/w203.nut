//expect:w203
::x <- 123.0
::y <- 123.1
::z <- 123.2
::w <- 123.3

local condition1 = ::x
local condition2 = ::y
local condition3 = ::z
local condition4 = ::w

if (condition1 || condition2 || condition3 | condition4)
  print("ok")