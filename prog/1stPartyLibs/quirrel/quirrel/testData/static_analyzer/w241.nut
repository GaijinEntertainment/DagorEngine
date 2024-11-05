if (__name__ == "__analysis__")
  return

//expect:w241

local str1 = require("string")
local str2 = require("string")
::print(str1, str2)
