local arr = [7, 6, 5, null]
local index = 0;

let function fn_null() { return null }
let function fn_step() { return arr[index++] }

local x;

if (x := fn_null())
  print("fail\n")
else
  print("ok\n")


while (x := fn_step())
  print(x)
