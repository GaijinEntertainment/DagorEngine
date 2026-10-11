options gen2

require _llvm_split_modules_mod_a
require _llvm_split_modules_mod_b

def private mixup(x : int) : int => helper_a(x) + helper_b(x) + slow_b(x)

[export]
def split_values(var acc, lo, hi, hit, miss : int?) {
    var a = 0
    for (i in range(10)) {
        a += mixup(i)
    }
    let p = make_pair(6)
    var t : table<int; int>
    t |> insert(3, 33)
    *acc = a
    *lo = p.lo
    *hi = p.hi
    *hit = table_b(t, 3)
    *miss = table_b(t, 9)
}

[export]
def main {
}
