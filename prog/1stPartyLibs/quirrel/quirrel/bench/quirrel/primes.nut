//-file:plus-string
function isprime(n) {
    local i;
    for (i = 2; i < n; ++i)
        if (n % i == 0)
            return false;
    return true;
}



function primes(n) {
    local count = 0;
    for (local i = 2; i <= n; ++i)
        if (isprime(i))
            ++count;
    return count;
}


local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

print("\"primes loop\", " + profile_it(20, function() {primes(14000)}) + ", 20\n");
