//-file:plus-string

function fibI(n) {
    local last = 0;
    local cur = 1;
    while(--n) {
        local tmp = cur;
        cur += last;
        last = tmp;
    }
    return cur;
}
local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"fibonacci loop\", " + profile_it(30, function() {fibI(6511134)}) + ", 30\n");
