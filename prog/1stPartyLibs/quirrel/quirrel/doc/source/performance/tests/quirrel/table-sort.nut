// Public domain, ported from lua

function create_rng(seed) {
    local Rm = array(17, 0);  // Initialize an array of 17 zeros
    local Rj = 1;

    for (local i = 17; i >= 1; i--) {
        seed = (seed * 9069) % (1 << 31);
        Rm[i - 1] = seed;  // Adjusted index for Squirrel (0-based)
    }

    return function() {
        local j = Rj;
        local m = Rm;
        local h = j - 5;
        if (h < 0) h += 17;
        local k = m[h] - m[j - 1];  // Adjusted index for Squirrel (0-based)
        if (k < 0) k += 2147483647;
        m[j - 1] = k;  // Adjusted index for Squirrel (0-based)
        Rj = (j < 17) ? j + 1 : 1;
        return k;
    }
}

local rand = create_rng(12345);

local n = 100000;  // No direct `arg` equivalent in Squirrel; set manually
local t = [];
for (local i = 0; i < n; i++) {
    t.append(rand());
}

function cmp(lhs, rhs) {
  return lhs > rhs;
}

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

function test(){
  local arr = clone t
  t.sort(cmp)
}

print("\"sort\", " + profile_it(10, test) + ", 10\n");