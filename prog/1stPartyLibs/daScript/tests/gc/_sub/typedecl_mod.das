options gen2
options indenting = 4
options strict_smart_pointers = true

module typedecl_mod shared public

def test_typedecl_type(tt : auto(TT)) {
    var x : typedecl(type<TT>)
    x = tt
    print("test: {x}\n")
}



