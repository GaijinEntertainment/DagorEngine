struct Foo
	dummy : int

var
	g_counter = 0

def operator delete ( f : Foo )
	g_counter ++


def test_delete
	var pA = new Foo
	var pB = new Foo
	var C = [[Foo]]
	var lam <- @ <| [[<-pB,:=pC]]
		assert(pA.dummy==0)
		assert(pB.dummy==0)
		assert(C.dummy==0)
	assert ( pB==null )
	invoke(lam)
	delete lam
	assert(g_counter==1)	// only C was safe to delete
	return true


[export]
def test
	var a1 <- [{int 1;2}]
	var a2 <- [{int 1;2}]
	var a3 <- [{int 1;2}]
	unsafe
		var lam <- @ <| [[&a1,<-a2,:=a3]]	// due to &a1
			push(a1,1)
			push(a2,1)
			push(a3,1)
		invoke(lam)
	assert(length(a1)==3)
	assert(length(a2)==0)
	assert(length(a3)==2)
    test_delete()
	return true
