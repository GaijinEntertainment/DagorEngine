require daslib/json_boost

struct Bar
	x, y : int

struct Foo
	i : int
	s : string
	q : double[3]
	b : Bar

[export]
def main
	let foo = [[Foo i=1, s="hello", q=[[double 1lf;2lf;3lf]], b=[[Bar x=13,y=14]] ]]

	var js_foo = JV(foo)
	print("{write_json(js_foo)}\n")

	var bar <- from_JV(js_foo, type<Foo>)
	print("bar = {bar}\n")
