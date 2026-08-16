// each_kv compile-time rejections: non-copyable value type, void values (set form — use keys()),
// and dim-array values (no dedicated overload). One statement per reject; the void/dim arms
// cascade the standard "void iteration" pair (30192/30107) behind the concept_assert.
options gen2

expect 31400:3, 30192:2, 30107:2

[export]
def main() {
    var nonCopyable : table<int; array<int>>
    var n = 0
    for (kv in each_kv(nonCopyable)) {
        n++
    }
    var voidValues : table<int>
    for (kv in each_kv(voidValues)) {
        n++
    }
    var dimValues : table<int; int[3]>
    for (kv in each_kv(dimValues)) {
        n++
    }
}
