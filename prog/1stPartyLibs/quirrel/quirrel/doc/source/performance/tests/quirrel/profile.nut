//-file:plus-string
local clock_func
try {
  clock_func = getroottable()["clock"]
}
catch (e) {
  clock_func = require("datetime").clock
}

function profile_it(cnt, f) {//for quirrel version
  local res = 0
  for (local i = 0; i < cnt; ++i) {
    local start = clock_func()
    f()
    local measured = clock_func() - start
    if (i == 0 || measured < res)
      res = measured;
  }
  return res / 1.0
}

return profile_it