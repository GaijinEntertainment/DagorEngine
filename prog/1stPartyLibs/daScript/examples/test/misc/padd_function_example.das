require padd_function

[export]
def main
	var foo : $padd_structure(type<int>,13+1)
	print("foo = {foo}\n")

options log
