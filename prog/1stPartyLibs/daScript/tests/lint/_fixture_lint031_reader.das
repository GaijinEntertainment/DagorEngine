options gen2
options auto_inline_functions = false

// the second program: the same fill_one name and signature, a body that only reads
def fill_one(var p : int?) {
    unsafe {
        p = *p == 0 ? p : null
    }
}

def route(dst : int?) {
    unsafe {
        fill_one(reinterpret<int?>(dst))
    }
}

[export]
def main() {
    var slot = 0
    unsafe {
        route(addr(slot))
    }
    print("{slot}\n")
}
