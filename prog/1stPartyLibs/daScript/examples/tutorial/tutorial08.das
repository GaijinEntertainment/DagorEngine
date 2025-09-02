options gen2
// require tutorial08module

typedef myType = typedecl(1 + 2)

def gentd(var a) {
    return typeinfo sizeof(type<typedecl(a)>)
}

[export]
def test() {
    print("sizeof myType = {typeinfo sizeof(type<myType>)}\n")
    return true
}

options log
options log_generics




