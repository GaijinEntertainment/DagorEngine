// delete super.self error cases — covers every validation branch.
options gen2
expect 30144:2, 30808:6, 30827, 30838:6, 30920:3

class Base {
}

// 1. no base class
class NoParent {
    def operator delete {
        delete super.self                           // 31002: no base class
    }
}

// 2. not inside a finalizer: regular class method
class DerivedA : Base {
    def bad_method() {
        delete super.self                           // 31002: not in operator delete
    }
}

// 3. not inside a finalizer: free function
def free_not_finalizer {
    delete super.self                               // 31002: not in operator delete
}

// 4. finalize with more than 1 argument
struct StructX {
    v : int
}

struct StructY : StructX {
    w : int
}

def finalize(var self : StructY, extra : int) {     // 2 args
    delete super.self                               // 31002: not a finalizer
}

// 5. first arg not named 'self'
struct StructZ : StructX {
    z : int
}

def operator delete(var x : StructZ) {              // arg name 'x'
    delete super.self                               // 31002: first arg must be 'self'
}

// 6. first arg isn't a structure type
def finalize(var self : int) {                      // int, not struct
    delete super.self                               // 31002: must be a structure type
}

[export]
def main {
}
