//-file:plus-string
const TOTAL_NUMBERS = 10000;
const TOTAL_TIMES = 100;

function mk_float(i) {
    return i + (i / TOTAL_NUMBERS);
}

function update(nums) {
    local summ = 0;
    for (local i = 0; i < TOTAL_TIMES; i++) {
        foreach (num in nums) {
            summ += num.tostring().len();
        }
    }
    return summ;
}

local nums = [];
for (local i = 1; i <= TOTAL_NUMBERS; i++) {
    nums.append(mk_float(i));
}
local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"string2float\", " + profile_it(20, @() update(nums) ) + ", 20\n");
