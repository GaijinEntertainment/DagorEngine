//-file:plus-string
local sqrt
try{
  sqrt = getroottable()["sqrt"]
}
catch (e) sqrt = require("math").sqrt

function test() {

    function A(i, j) {
        local ij = i + j - 1;
        return 1.0 / (ij * (ij - 1) * 0.5 + i);
    }

    function Av(x, y, N) {
        for (local i = 0; i < N; i++) {
            local a = 0;
            for (local j = 0; j < N; j++) a += x[j] * A(i + 1, j + 1);  // Adjusted indices for 1-based A function
            y[i] = a;
        }
    }

    function Atv(x, y, N) {
        for (local i = 0; i < N; i++) {
            local a = 0;
            for (local j = 0; j < N; j++) a += x[j] * A(j + 1, i + 1);  // Adjusted indices for 1-based A function
            y[i] = a;
        }
    }

    function AtAv(x, y, t, N) {
        Av(x, t, N);
        Atv(t, y, N);
    }

    local N = 500;
    local u = array(N, 1);  // Initialize an array of size N with all elements set to 1
    local v = array(N, 0);  // Initialize an array of size N with all elements set to 0
    local t = array(N, 0);  // Initialize an array of size N with all elements set to 0

    for (local i = 0; i < 10; i++) {
        AtAv(u, v, t, N);
        AtAv(v, u, t, N);
    }

    local vBv = 0.0;
    local vv = 0.0;
    for (local i = 0; i < N; i++) {
        local ui = u[i];
        local vi = v[i];
        vBv += ui * vi;
        vv += vi * vi;
    }

    return sqrt(vBv / vv);
}
local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"spectral norm\", " + profile_it(10, test) + ", 10\n");
