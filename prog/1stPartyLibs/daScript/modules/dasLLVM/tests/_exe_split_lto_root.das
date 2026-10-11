options gen2
require daslib/strings_boost
require math

// the split exe's fixture: work spread over three das modules (this one, strings_boost, math), so the
// partitions call across each other and the entry's registration names functions in every one

def mix(values : array<int>) : int {
    var acc = 0
    for (v in values) {
        acc = acc * 31 + v
    }
    return acc
}

def twice(v : int) : int => v * 2

//! a global table literal: the entry's init_globals goes through the string-key table accessor
var SEEDS <- { "split" => 5 }

//! the same accessor from this module's partition, so its slot is collected before the entry declares it
def count_distinct(words : array<string>) : int {
    let seen <- {for (w in words); w => length(w)}
    return length(seen)
}

//! vector-constant literals at and past the constant-data floor, one below it, and a mixed one
def vectors : int {
    let pairs = [float2(1.0, 2.0), float2(3.0, 4.0), float2(5.0, 6.0), float2(7.0, 8.0)]
    let quads = [int4(1, 2, 3, 4), int4(5, 6, 7, 8), int4(9, 10, 11, 12), int4(13, 14, 15, 16), int4(17, 18, 19, 20)]
    let three = [10, 20, 30]
    let mixed = [1, 2, 3, length(pairs)]
    var acc = 0
    for (p in pairs) {
        acc += int(p.x + p.y)
    }
    for (q in quads) {
        acc += q.x + q.w
    }
    for (v in three) {
        acc += v
    }
    for (m in mixed) {
        acc += m
    }
    return acc
}

//! a function address taken inside a body: its slot is defined in this partition and filled by the exe's startup
def apply(f : function<(v : int) : int>; v : int) : int => invoke(f, v)

[export]
def main {
    let words <- "split lto exe".split(" ")
    let lengths <- [for (w in words); length(w)]
    print("WORDS {length(words)}\n")
    print("MIX {mix(lengths)}\n")
    print("SQRT {int(sqrt(144.0))}\n")
    print("ADDR {apply(@@twice, 21)}\n")
    print("TABLE {(SEEDS?["split"] ?? 0) + count_distinct(words)}\n")
    print("VEC {vectors()}\n")
}
