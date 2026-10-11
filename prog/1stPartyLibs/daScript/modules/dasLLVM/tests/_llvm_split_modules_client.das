options gen2

// child for llvm_split_modules.das: a 3-partition program (main -> mod_a -> mod_b) whose one
// output line the parent checks against the interp-derived constants
require _llvm_split_modules_mod_a
require _llvm_split_modules_mod_b

def private mixup(x : int) : int => helper_a(x) + helper_b(x) + slow_b(x)

[export]
def main {
    var acc = 0
    for (i in range(10)) {
        acc += mixup(i)
    }
    let p = make_pair(6)
    var t : table<int; int>
    t |> insert(3, 33)
    print("SPLIT_RESULT {acc} {p.lo} {p.hi} {table_b(t, 3)} {table_b(t, 9)}\n")
}
