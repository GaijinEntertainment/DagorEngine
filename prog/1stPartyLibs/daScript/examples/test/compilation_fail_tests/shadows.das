expect 30305:4

def invokeIt ( a:int; xxx )
	print("xxx = {xxx}\n")
	invoke(xxx,a)

def testA ( a : int )
	invokeIt(a) <| $ ( a : int )
		print("a = {a}\n")

def testB ( b : int )
	invokeIt(b) <| $ ( a : int )
		invokeIt(a) <| $ ( a : int )
			print("a = {a}\n")

def testC ( a : int )
	invokeIt(a) <| $ ( b : int )
		var b = a
		print("b = {b}\n")

def testD ( a : int )
	var a = 5
	print("a = {a}\n")

[export]
def test()
	testA(13)
	testB(13)
	testC(13)
	testD(13)
	return true
