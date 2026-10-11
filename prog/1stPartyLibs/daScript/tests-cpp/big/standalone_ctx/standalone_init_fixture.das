options gen2
options stack = 262144

require standalone_init_dep
require daslib/export_c

var g_stamp = 0

def next_stamp : int {
    g_stamp ++
    return g_stamp
}

def read_forward : int {
    return g_later
}

var g_first = next_stamp() + g_dep
var g_second = next_stamp()
var g_reads_forward = read_forward() + 100
var g_later = 7
var g_init_fn_stamp = 0
let shared g_shared_taps = fixed_array<int>(1, 2, 3)

[init]
def record_init {
    g_init_fn_stamp = next_stamp()
}

[export]
def get_first : int {
    return g_first
}

[export]
def get_second : int {
    return g_second
}

[export]
def get_init_fn_stamp : int {
    return g_init_fn_stamp
}

[export]
def get_dep_init_stamp : int {
    return g_dep_init_stamp
}

[export]
def get_reads_forward : int {
    return g_reads_forward
}

[export]
def get_later : int {
    return g_later
}

[export]
def get_shared_total : int {
    var total = 0
    for (t in g_shared_taps) {
        total += t
    }
    return total
}

struct Pair {
    a : int
    b : int
}

enum Mode {
    off
    on
}

[export]
def make_pair(x, y : int) : Pair {
    return Pair(a = x, b = y)
}

[export]
def pair_sum(p : Pair) : int {
    return p.a + p.b
}

[export]
def flip(m : Mode) : Mode {
    return m == Mode.on ? Mode.off : Mode.on
}

struct Outer {
    inner : Inner
    tag : int
}

struct Inner {
    weight : int
}

[export]
def outer_weight(o : Outer) : int {
    return o.inner.weight + o.tag
}

struct Node {
    value : int
    next : Node?
}

var private g_node : Node?

[export]
def head_value : int {
    return g_node?.value ?? 41
}

[export]
def apply_lambda(x : int) : int {
    let add3 = @(v : int) : int => v + 3
    return invoke(add3, invoke(add3, x))
}

def local_double(x : int) : int {
    return x + x
}

[export]
def call_through_pointer(x : int) : int {
    let fp = @@local_double
    return invoke(fp, x)
}

[export]
def sum_generator(n : int) : int {
    var gen <- generator<int> {
        for (i in range(n)) {
            yield i
        }
        return false
    }
    var s = 0
    for (v in gen) {
        s += v
    }
    return s
}

[export_c(name = "renamed_sum")]
def sum_under_another_name(a, b : int) : int {
    return a + b
}
