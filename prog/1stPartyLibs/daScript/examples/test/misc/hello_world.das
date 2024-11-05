require padd_function

def take_structure ( foo : $padd_structure(type<auto(TT)>,null); value : TT )
	print("general {typeinfo(typename foo)} with TT = {typeinfo(typename value)}\n")

def take_structure ( foo : $padd_structure(type<tuple<auto(TT);float>>,null); value : TT )
	print("tuple specialized {typeinfo(typename foo)} with TT = {typeinfo(typename value)}\n")

[export]
def main
	var foo : $padd_structure(type<int>,4)
	take_structure(foo, 13)
	var bar : $padd_structure(type<float>,8)
	take_structure(bar, 3.14)
	var dar : $padd_structure(type<tuple<int;float>>,4)
	take_structure(dar, 13)

options log_infer_passes

