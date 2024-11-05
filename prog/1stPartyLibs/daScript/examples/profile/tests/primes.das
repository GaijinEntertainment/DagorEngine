// options log=true, print_var_access=true, print_ref=true

require testProfile

include ../config.das

def isprime(n)
    for i in range(2, n)
        if (n % i == 0)
            return false
    return true

[sideeffects]
def primes(n)
    var count = 0
    for i in range(2, n+1)
        if isprime(i)
            ++count
    return count


[export,no_jit,no_aot]
def main
	var f1 = 0
    profile(20,"primes loop") <|
        f1 = primes(14000)
	assert(f1==1652)
    if is_cpp_time()
        cpp_label()
        var f3 = 0
        profile(20,"primes loop") <|
            f3 = testProfile::testPrimes(14000)
        assert(f3==1652)
        run_files([[TestFile
            luauFile = "primes.luau",
            luaFile = "primes.lua",
            luaJitFile = "primes.lua",
            nutFile = "primes.nut",
            nutModifiedFile = "primes.nut",
            asFile = "primes.as",
            jsFile = "primes.js",
            monoFile = "primes.cs",
            dotNetFile = "primes.cs",
        ]])
