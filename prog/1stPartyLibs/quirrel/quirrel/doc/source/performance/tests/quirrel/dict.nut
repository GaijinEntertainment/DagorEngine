//-file:plus-string
function dict(tab, src) {
  local maxOcc = 0
  local n = 0
  foreach (l in src) {
    if (!(l in tab))
      tab[l] <- 0
    local tabV = ++tab[l]
    maxOcc = tabV > maxOcc ? tabV : maxOcc
  }
  return maxOcc
}

local TAB = {}
local SRC = []
local n = 500000
local modn = n
for (local i = 0; i < n; ++i) {
  local num = (271828183 ^ i*119)%modn
  SRC.append("_" + num)
}


local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"dictionary\", " + profile_it(25, function () {TAB={}; dict(TAB, SRC) }) + ", 25\n")
