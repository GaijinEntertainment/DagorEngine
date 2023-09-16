// options log=true, print_var_access=true, print_ref=true

require testProfile

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

[sideeffects]
def primesI(n)
    var count = 0
    for i in range(2, n+1)
		count ++
		for j in range(2,i)
			if i % j == 0
				count --
				break
    return count

[export]
def test()
	var f1 = 0
    profile(20,"primes loop") <|
        f1 = primes(14000)
	assert(f1==1652)
	var f2 = 0
    profile(20,"primes loop, inline") <|
        f2 = primesI(14000)
	assert(f2==1652)
	var f3 = 0
    profile(20,"primes loop C++") <|
        f3 = testProfile::testPrimes(14000)
	assert(f3==1652)
    return true

