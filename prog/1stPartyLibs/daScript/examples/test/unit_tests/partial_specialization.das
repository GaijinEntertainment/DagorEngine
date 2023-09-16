let
	allow_broken = false

struct Foo
	a : int

struct Bar : Foo
	b : int

struct Far : Bar
	c : int

/////////////////////////////////////////////////////////////////////////////////////////////////////
// 0. substitute distance is number of casts required for the function
//		function with less casts is picked

def test(a:Foo;b:Foo)
	return "foo foo"

def test(a:Foo;b:Bar)
	return "foo bar"

def test(a:Bar;b:Foo)
	return "bar foo"

def test_distance
	assert( test([[Foo]],[[Foo]])=="foo foo")	// 0 cast
	assert( test([[Foo]],[[Bar]])=="foo bar")	// 0 cast
	assert( test([[Bar]],[[Foo]])=="bar foo")	// 0 cast
	assert( test([[Far]],[[Bar]])=="foo bar")	// 1 cast
	static_if allow_broken
		test([[Bar]],[[Bar]])					// won't compile

/////////////////////////////////////////////////////////////////////////////////////////////////////
//  one generic is more specialized than the other, if ALL arguments are more specialized or the same

// 1. non auto is more specialized than auto

def gen_1(a:Foo;b)
	return "Foo *"

def gen_1(a;b)
	return "* *"

def test_gen_1
	assert( gen_1([[Foo]],[[Foo]])=="Foo *" )

// 2. if both non-auto, the one without cast is more specialized

def gen_2(a:Foo;b)
	return "Foo *"

def gen_2(a:Bar;b)
	return "Bar *"

def test_gen_2
	assert( gen_2([[Bar]],[[Foo]])=="Bar *" )

// 3. one with dim is more specialized, than one without
//      if both have dim, one with actual value is more specialized, than the other one

def gen_3 ( arr )
	return "*"

def gen_3 ( arr : auto[] )
	return "[*]"

def gen_3 ( arr : auto[3] )
	return "[3]"

def test_gen_3
	assert ( gen_3([[int 1;2;3;4]])=="[*]" )
	assert ( gen_3([[int 1;2;3]])=="[3]" )

// 4. the one with base type of auto\alias is less specialized
//      if both are auto\alias - we assume its the same level of specialization

def gen_4  ( arr )
	return "*"

def gen_4 ( arr : array<auto> )
	return "array<*>"

def test_gen_4
	let t : array<int>
	assert ( gen_4(t)=="array<*>" )

// if its an array or a pointer, we compare specialization of subtype

def gen_5  ( arr : array<auto?> )
	return "<*>"

def gen_5 ( arr : array<auto> )
	return "<*?>"

def test_gen_5
	let t : array<int?>
	assert ( gen_5(t)=="<*?>" )

// if its a table, we compare both subtypes

def gen_6 ( arr )
	return "*"

def gen_6 ( arr : table<auto;auto?> )
	return "<*:*?>"

def gen_6  ( arr : table<int;auto> )
	return "<int:*?>"

def gen_6 ( arr : table<int;auto?> )
	return "<int:*>"

def test_gen_6
	let t : table<int;int?>
	assert ( gen_6(t)=="<int:*?>" )
	let tf : table<float;int?>
	assert ( gen_6(tf)=="<*:*?>" )

// if its a tuple or a variant, we compare all subtypes

def gen_7 ( arr : tuple<auto;auto?;int> )
	return "<*:*?:int>"

def gen_7  ( arr : tuple<auto;auto;auto> )
	return "<*:*:*>"

def test_gen_7
	let t : tuple<float;int?;int>
	assert ( gen_7(t)=="<*:*?:int>" )

// if its a block, a function, or a lambda, we compare all subtypes and firstType (result)

def gen_8 ( arr : auto )
	return "*"

def gen_8 ( arr : function<(arg:auto;arg2:int):auto?> )
	return "(*:int):?"

def gen_8  ( arr : function<(arg:auto;arg2:int):auto> )
	return "(*:int):*"

def test_gen_8
	let t : function<(arg:float;arg2:int):void?>
	assert ( gen_8(t)=="(*:int):?" )

[export]
def test
	test_distance()
	test_gen_1()
	test_gen_2()
	test_gen_3()
	test_gen_4()
	test_gen_5()
	test_gen_6()
	test_gen_7()
	test_gen_8()
	return true
