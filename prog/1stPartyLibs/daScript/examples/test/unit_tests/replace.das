require strings

[export]
def test()
	assert(replace("hello, worlld","ll","goo") == "hegooo, worgood")
	assert(replace("","a","bb")=="")
	assert(replace("hello, worlld","ll","")=="heo, word")
	assert(replace("hello, world","","x")=="hello, world")
    return true
