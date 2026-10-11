options gen2

// child fixture of llvm_jit_debug_info.das: one function covering every DWARF shape the rail
// describes - scalars, a struct argument, an enum, a string, a vector local, a nested block,
// a ref local, a while latch, a fixed array, a container, a tuple, a self-referencing struct
// reached through a pointer, and a block that captures a local by reference

struct Point {
    x : float
    y : float
}

struct Node {
    value : int
    next : Node?
}

enum Mode {
    fast
    slow
}

var g_calls = 0

def private invoke_block(blk : block<(a : int) : int>) {
    return invoke(blk, 3)
}

[export]
def hot(n : int; scale : float; p : Point; m : Mode; tag : string) : int {
    g_calls++
    var acc = 0
    var f4 = float4(1.0, 2.0, 3.0, 4.0)
    f4 = f4 * scale
    for (i in range(n)) {
        acc += i * i
        if (i == n / 2) {
            let inner = acc + int(scale * p.x)
            acc += inner % 7
        }
    }
    var acc_ref & = acc
    acc_ref++
    var spin = 0
    while (spin < 3) {
        spin++
    }
    var dim3 : int[3]
    dim3[1] = spin
    var xs : array<int>
    xs |> push(spin)
    let pair = (spin, scale)
    var head = Node(value = spin, next = null)
    let node = unsafe(addr(head))
    let blocked = invoke_block() $(a : int) : int {
        acc_ref += a
        return a * 2
    }
    return (acc + g_calls + dim3[1] + length(xs) + pair._0 + node.value + blocked + int(f4.w)
        + (m == Mode.slow ? length(tag) : 0))
}

[export]
def main {
    let p = Point(x = 1.5, y = 2.5)
    print("DEBUG_INFO_ROOT {hot(100, 2.0, p, Mode.slow, "run")}\n")
}
