//expect:w203
local condition1 = ::x
local condition2 = ::y
local condition3 = ::z
local condition4 = ::w

if (condition1 || condition2 || condition3 | condition4)
  ::print("ok")

//-file:undefined-global