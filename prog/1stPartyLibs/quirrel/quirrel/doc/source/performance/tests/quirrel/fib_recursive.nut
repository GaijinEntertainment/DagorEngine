//-file:plus-string
local fibR
fibR = function(n) {
    if (n < 2) return n;
    return (fibR(n-2) + fibR(n-1));
}

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"fibonacci recursive\", " + profile_it(20, function() {fibR(31)}) + ", 20\n");
