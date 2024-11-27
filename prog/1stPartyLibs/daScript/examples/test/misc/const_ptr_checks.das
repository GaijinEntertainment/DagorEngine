options relaxed_pointer_const = false

struct Foo
	a : int

struct Bar
	a : Foo?

struct Boo
	a : Far?
	b : array<int>

variant Far
	a : Foo?

tuple Tup
	a : Foo?

[export]
def test_copy_and_clone
	var a : Bar?
	let b : Bar?
	a := b
	a = b
	var c = b
	debug(c)

def new_const_foo : Foo? const
	return new Foo()

struct Goo
	a : Foo? = new_const_foo()

var A : Foo? = new_const_foo()

def get_foo : Foo?
	let a = new Foo()
	return a

[export]
def test_make_variant
	var t = Far(a=new_const_foo())

[export]
def test_make_struct
	var t = Bar(a=new_const_foo())

[export]
def test_make_tuple
	var t = Tup(a=new_const_foo())

[export]
def test_complex_types
	var a : Bar
	let b : Bar
	a = b
	a := b

[export]
def test_clone_complex_types
	var a : Bar
	let b : Bar
	a := b

