struct Foo
	a : int = 10
	s : string = "bar"

def test_c(): Foo const#
	unsafe
		return [[Foo# a=4]]

[export]
def test()
	let foo = test_c()
	let cop := foo
	assert(foo.a==4)
    assert(cop.a==4)
    return true
