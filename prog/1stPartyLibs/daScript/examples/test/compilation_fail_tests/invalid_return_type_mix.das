expect 30102, 30102

struct Foo
    a:int

def func
    return $(a:int):int { return a+1; } // 30102: not a valid function return type
                                        // 30102: this type can't be returned at all block<(int):int>

def test:bool
    func()
    return true

