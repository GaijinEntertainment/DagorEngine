// options log=true, print_var_access=true, print_ref=true

require testProfile

[sideeffects]
def fibR(n)
    if (n < 2)
        return n
    return fibR(n - 1) + fibR(n - 2)

[sideeffects]
def fibI(n)
    var last = 1
    var cur = 0
    for i in range(n)
        let tmp = cur
        cur += last
        last = tmp
    return cur

[export]
def test()
	var f1 = 0
    profile(20,"fibbonacci loop") <|
        f1 = fibI(6511134)
	assert(f1==1781508648)
	var f3 = 0
    profile(20,"fibbonacci recursive") <|
        f3 = fibR(31)
	assert(f3==1346269)
	var f4 = 0
    profile(20,"fibbonacci loop c++") <|
        f4 = testProfile::testFibI(6511134)
	assert(f4==1781508648)
	var f5 = 0
    profile(20,"fibbonacci recursive c++") <|
        f5 = testProfile::testFibR(31)
	assert(f5==1346269)
    return true

