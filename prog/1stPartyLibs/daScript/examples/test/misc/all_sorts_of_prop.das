def get(cb : block<(a : string#) : void>)
	unsafe
		cb |> invoke(reinterpret<string#> "foo")

struct Foo
	resz : string

def operator . res := ( var f : Foo; st : string# )
	f.resz := st

[export]
def main()
	if true
		var res : string
		get() <| $ (name)
			res := name
		debug(res)
	if true
		var f : Foo
		get() <| $ (name)
			f.resz := name
		debug(f)
	if true
		var f : Foo
		get() <| $ (name)
			f.res := name
		debug(f)

class sealed UIFrame
    def const operator . sizezz() : float2
		print("get value")
        return float2(1,2)
    def operator . sizezz :=(value : float2)
        print("set {value}\n")

[export]
def apply_canvas_anchoring(var frame : UIFrame?; parentSize : float2 )
	frame.sizezz := parentSize

options log
