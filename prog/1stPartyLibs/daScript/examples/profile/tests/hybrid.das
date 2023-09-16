// options log=true, print_var_access=true, print_ref=true

require testProfile

[sideeffects]
def fibR(n)
    if (n < 2)
        return n
	return fibR(n - 1) + fibR(n - 2)

[sideeffects,no_aot]
def fibRNA(n)
    if (n < 2)
        return n
	return fibRNA(n - 1) + fibRNA(n - 2)

[sideeffects, hybrid]
def fibRH(n)
    if (n < 2)
        return n
	return fibRH(n - 1) + fibRH(n - 2)

[export]
def test()
	var f1 = 0
    profile(20,"fibbonacci recursive") <|
        f1 = fibR(31)
	assert(f1==1346269)
	var f2 = 0
    profile(20,"fibbonacci recursive, no aot") <|
        f2 = fibRNA(31)
	assert(f2==1346269)
	var f3 = 0
    profile(20,"fibbonacci recursive, hybrid") <|
        f3 = fibRH(31)
	assert(f3==1346269)
    return true


