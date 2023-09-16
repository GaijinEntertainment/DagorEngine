struct Foo
    a,b:int

var
    ds : das_string?

[sideeffects, unused_argument(obj,name)]
def get_ecs_string(obj,name) : das_string?
    return ds

def get_string(obj; name, defaultValue : string)
    let str = get_ecs_string(obj, name)
    return str != null ? string(deref(str)) : defaultValue

[export]
def test
    let abilityObj : Foo
    var name = get_string(abilityObj, "name", "")
    name = get_string(abilityObj, "name", "")
    return true

