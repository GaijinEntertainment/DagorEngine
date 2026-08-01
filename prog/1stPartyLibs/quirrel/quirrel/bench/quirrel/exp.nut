//-file:plus-string
local exp
try {
  exp = getroottable()["exp"]
} catch (e) {
  exp = require("math").exp
}

function expLoop(n) {
    local sum = 0
    for (local i = 0; i < n; ++i)
      sum += exp(1./(1.+i))
    return sum
}


local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"exp loop\", " + profile_it(20, function() {expLoop(1000000)}) + ", 20\n");
