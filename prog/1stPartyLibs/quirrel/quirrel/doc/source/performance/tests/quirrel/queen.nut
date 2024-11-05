//-file:plus-string
local N = 8;  // Set the board size; Squirrel does not use arg by default

function isplaceok(a, n, c, cmn, cpn) {
    for (local i = 0; i < n; i++) {
        local ai = a[i];
        if (ai == c || ai == cmn + i || ai == cpn - i) {
            return false;
        }
    }
    return true;
}

local solutions = 0;

function addqueen(a, n) {
    for (local c = 0; c < N; c++) {
        if (isplaceok(a, n, c, c - n, c + n)) {
            a[n] = c;
            if (n == (N - 1)) {
                solutions++;
            } else {
                addqueen(a, n + 1);
            }
        }
    }
}

function test() {
    solutions = 0;
    local a = array(8, null);  // Initialize an array of size 8 with nulls
    addqueen(a, 0);
    assert(solutions == 92);
}

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"queen\", " + profile_it(20, test) + ", 20\n");
