expect 30102, 32101, 32102

def test_mix(arg:bool)
    if arg
        return 13
    else
        return 13.0         // 30102: incompatible return type, expecting int vs float

def test_int:int
    return                  // 32101: expecting return value

def test_void:void
    return 13               // 32102: not expecting return value


