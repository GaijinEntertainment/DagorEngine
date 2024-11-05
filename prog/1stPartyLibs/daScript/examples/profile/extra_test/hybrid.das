// options log=true, print_var_access=true, print_ref=true

require testProfile

[sideeffects]
def fibR(n)
    if (n < 2)
        return n
	return fibR(n - 1) + fibR(n - 2)

[sideeffects, hybrid]
def fibRH(n)
    if (n < 2)
        return n
	return fibRH(n - 1) + fibRH(n - 2)

[export]
def main
	var f1 = 0
    profile(20,"fibonacci recursive") <|
        f1 = fibR(31)
	assert(f1==1346269)
	var f3 = 0
    profile(20,"fibonacci recursive, hybrid") <|
        f3 = fibRH(31)
	assert(f3==1346269)


