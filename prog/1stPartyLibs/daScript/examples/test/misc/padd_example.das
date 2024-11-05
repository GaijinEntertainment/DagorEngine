require padd

def take_foo ( foo : $padd_dim(type<auto(TT)>,14) ) : TT
	debug(foo)
	return [[TT]]

[export]
def main
	var foo : $padd_dim(type<float>,14)
    print("foo = {foo}\n")
	take_foo(foo)
