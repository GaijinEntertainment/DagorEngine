options gen2
options auto_inline_functions = false
// lint-skip-file: the const-stripped call below is the finding test_lint031_memo_and_anchor.das measures

// the first of two programs sharing one callee name: this fill_one writes through its slot
def fill_one(var p : int?) {
    unsafe {
        *p = 1
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
