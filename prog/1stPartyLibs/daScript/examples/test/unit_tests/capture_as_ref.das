class Foo
	i : int
	k : array<int>
	def Foo
		i = 5
	def kaboom : lambda<int>
		var counter <- @ <| ()
			return i ++             // NOTE: lambda captures 'self' here. good thing 'self' is captured as REF by default
		return <- counter

[export]
def test
	var x = new Foo()
	let boom <- x->kaboom()
	assert(x.i==5)
	return true
