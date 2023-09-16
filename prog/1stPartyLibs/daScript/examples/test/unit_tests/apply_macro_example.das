require _apply_macro

// options log = true

var
	in_the_cond = false
	in_the_func = false

def custom_cond
	in_the_cond = true
	return true

[add_cond(cond="custom_cond")]
def custom_func
	in_the_func = true

[export]
def test()
	custom_func()
	assert(in_the_cond)
	assert(in_the_func)
    return true

