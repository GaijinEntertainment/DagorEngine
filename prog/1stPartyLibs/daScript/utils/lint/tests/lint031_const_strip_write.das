options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT031: a const pointer or reference parameter the body writes through after stripping
// the const - by reinterpret, an intptr round trip, or a writable slot of a call. The declared
// type denies a write the function performs; the parameter is declared var instead. The
// finding anchors at the parameter, the line the fix rewrites.

expect 50503:24

require daslib/lint

struct Cell {
    a : int
    b : int
}

struct Outer {
    inner : Cell
    slot_addr : uint64
}

struct Body {
    pos : float3
}

struct Node {
    next : Node?
    val : int
}

// ===== callees the routes below hand a stripped pointer to =====

def fill_one(var p : int?) {
    unsafe {
        *p = 1
    }
}

def forget_one(var p : int?) : int? {
    p = null
    return p
}

def bump_one(var p : int?) {
    unsafe {
        *p += 1
    }
}

def inc_one(var p : int?) {
    unsafe {
        (*p)++
    }
}

def clone_cell(var p : Cell?; c : Cell) {
    unsafe {
        *p := c
    }
}

def move_ints(var p : array<int>?; var a : array<int>) {
    unsafe {
        *p <- a
    }
}

def fill_via_memcpy(var p : void?; v : int) {
    var s = v
    unsafe {
        memcpy(p, addr<void?>(s), 4)
    }
}

def fill_recursive(var p : int?; n : int) {
    if (n > 0) {
        fill_recursive(p, n - 1)
    } else {
        unsafe {
            *p = 1
        }
    }
}

// a callee that strips its own const slot is reported on its own line, never at its caller
def sneaky(q : int?; v : int) {                             // LINT031
    unsafe {
        *reinterpret<int?>(q) = v
    }
}

// ===== the shapes the rule reports =====

// a const void? out-parameter, cast to a typed pointer
def route_void(dst : void?; v : int) {                      // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        *p = v
    }
}

// a const typed pointer, cast to its own type, written by index
def route_index(dst : int?; v : int) {                      // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        p[0] = v
    }
}

// a const structure reference, address taken and cast back
def route_const_ref(c : Cell; v : int) {                    // LINT031
    unsafe {
        var p = addr<Cell?>(c)
        p.a = v
    }
}

// a write into a nested field of the pointee
def route_nested(o : Outer; v : int) {                      // LINT031
    unsafe {
        var p = addr<Outer?>(o)
        p.inner.a = v
    }
}

// a swizzle into a vector field of the pointee
def route_swizzle(b : Body; v : float) {                    // LINT031
    unsafe {
        var p = addr<Body?>(b)
        p.pos.x = v
    }
}

// a write through pointer arithmetic on the stripped pointer
def route_offset(dst : int?; v : int) {                     // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        *(p + 1) = v
    }
}

// a reference local bound to the stripped pointee
def route_ref_local(dst : int?; v : int) {                  // LINT031
    unsafe {
        var r & = *reinterpret<int?>(dst)
        r = v
    }
}

// the pointer laundered through an integer
def route_intptr(dst : void?; v : int) {                    // LINT031
    unsafe {
        var i = intptr(dst)
        var p = reinterpret<int?>(i)
        *p = v
    }
}

// a const void? handed to memcpy's destination slot
def route_memcpy(dst : void?; v : int) {                    // LINT031
    var src = v
    unsafe {
        memcpy(dst, addr<void?>(src), 4)
    }
}

// a const void? handed to memmove's destination slot
def route_memmove(dst : void?; v : int) {                   // LINT031
    var src = v
    unsafe {
        memmove(dst, addr<void?>(src), 4)
    }
}

// a const void? handed to a memset destination slot
def route_memset(dst : void?; v : uint8) {                  // LINT031
    unsafe {
        memset8(dst, v, 4)
    }
}

// a compound assignment, a step, a clone and a move through the stripped pointer
def route_op_assign(dst : int?; v : int) {                  // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        *p += v
    }
}

def route_inc_top(dst : int?) {                             // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        (*p)++
    }
}

def route_clone_top(dst : Cell?; c : Cell) {                // LINT031
    unsafe {
        var p = reinterpret<Cell?>(dst)
        *p := c
    }
}

def route_move_top(dst : array<int>?; var a : array<int>) { // LINT031
    unsafe {
        var p = reinterpret<array<int>?>(dst)
        *p <- a
    }
}

// two writes through one parameter report once
def route_twice(dst : int?; v : int) {                      // LINT031
    unsafe {
        var p = reinterpret<int?>(dst)
        *p = v
        *p = v + 1
    }
}

// the write happens in a das callee whose body writes through its var slot - by a store, a
// compound assignment, a step, a clone, a move, a nested memcpy, or under recursion
def route_via_callee(dst : int?) {                          // LINT031
    unsafe {
        fill_one(reinterpret<int?>(dst))
    }
}

def route_via_bump(dst : int?) {                            // LINT031
    unsafe {
        bump_one(reinterpret<int?>(dst))
    }
}

def route_via_inc(dst : int?) {                             // LINT031
    unsafe {
        inc_one(reinterpret<int?>(dst))
    }
}

def route_via_clone(dst : Cell?; c : Cell) {                // LINT031
    unsafe {
        clone_cell(reinterpret<Cell?>(dst), c)
    }
}

def route_via_move(dst : array<int>?; var a : array<int>) { // LINT031
    unsafe {
        move_ints(reinterpret<array<int>?>(dst), a)
    }
}

def route_via_memcpy_callee(dst : void?; v : int) {         // LINT031
    unsafe {
        fill_via_memcpy(reinterpret<void?>(dst), v)
    }
}

def route_via_recursive(dst : int?) {                       // LINT031
    unsafe {
        fill_recursive(reinterpret<int?>(dst), 2)
    }
}

// ===== the shapes the rule leaves alone =====

// Good: the parameter is declared var
def write_var(var dst : void?; v : int) {
    unsafe {
        var p = reinterpret<int?>(dst)
        *p = v
    }
}

// Good: the const is stripped to read only
def read_stripped(src : void?) : int {
    unsafe {
        let p = reinterpret<int?>(src)
        return *p
    }
}

// Good: the address is taken and read through - a handout, not a write
def read_via_addr(c : Cell) : int {
    unsafe {
        let p = addr(c)
        return p.a + p.b
    }
}

// Good: a local copy of the pointee is written, the parameter is not
def write_copy(c : Cell; v : int) : int {
    var local = c
    local.a = v
    return local.a
}

// Good: a local copy of the pointer is reassigned, nothing the parameter owns is written
def walk_copy(head : Cell?; hops : int) : Cell const? {
    var cur = head
    for (_ in range(hops)) {
        cur = null
    }
    return cur
}

// Good: an address the pointee stores is loaded and written through - not the parameter's memory
def write_stored_address(o : Outer; v : int) {
    unsafe {
        var p = reinterpret<int?>(o.slot_addr)
        *p = v
    }
}

// Good: a pointer the pointee stores is loaded and written through - not the parameter's memory
def write_next_val(n : Node; v : int) {
    unsafe {
        var p = addr<Node?>(n)
        p.next.val = v
    }
}

// Good: the callee's var slot only reassigns its own copy of the pointer
def route_via_reader(dst : int?) {
    unsafe {
        forget_one(reinterpret<int?>(dst))
    }
}

// Good: the caller of a self-stripping callee fills a const slot - the finding is the callee's
def route_via_sneaky(dst : int?; v : int) {
    unsafe {
        sneaky(reinterpret<int?>(dst), v)
    }
}

// Good: the source slot of memcpy is read
def copy_from(src : void?; n : int) : int {
    var buf : uint8[8]
    unsafe {
        memcpy(addr<void?>(buf[0]), src, n)
    }
    return int(buf[0])
}

// Good: an implicit parameter type waives the const contract
def read_into(bytes : void? implicit; v : int) {
    var s = v
    unsafe {
        memcpy(bytes, addr<void?>(s), 4)
    }
}

// Good: a by-value parameter's slot is the function's own copy
def scribble_value(v : int) : int {
    unsafe {
        var p = addr<int?>(v)
        *p = 1
        return *p
    }
}

[export]
def main() {
    var slot = 0
    var cell = Cell(a = 1, b = 2)
    var pair : int[2]
    var outer = Outer(inner = Cell(a = 0, b = 0), slot_addr = 0ul)
    var body = Body(pos = float3(0.0, 0.0, 0.0))
    var node = Node(next = null, val = 0)
    var ints : array<int>
    var ints2 : array<int>
    var arr_slot : array<int>
    unsafe {
        outer.slot_addr = intptr(addr(slot))
        node.next = addr(node)
        route_void(addr<void?>(slot), 1)
        route_index(addr(slot), 2)
        route_const_ref(cell, 3)
        route_nested(outer, 9)
        route_swizzle(body, 1.5)
        route_offset(addr(pair[0]), 10)
        route_ref_local(addr(slot), 12)
        route_intptr(addr<void?>(slot), 4)
        route_memcpy(addr<void?>(slot), 5)
        route_memmove(addr<void?>(slot), 5)
        route_memset(addr<void?>(slot), 5u8)
        route_op_assign(addr(slot), 6)
        route_inc_top(addr(slot))
        route_clone_top(addr(cell), cell)
        route_move_top(addr(arr_slot), ints)
        route_twice(addr(slot), 13)
        route_via_callee(addr(slot))
        route_via_bump(addr(slot))
        route_via_inc(addr(slot))
        route_via_clone(addr(cell), cell)
        route_via_move(addr(arr_slot), ints2)
        route_via_memcpy_callee(addr<void?>(slot), 14)
        route_via_recursive(addr(slot))
        route_via_reader(addr(slot))
        route_via_sneaky(addr(slot), 15)
        write_var(addr<void?>(slot), 7)
        write_stored_address(outer, 11)
        write_next_val(node, 16)
        read_into(addr<void?>(slot), 17)
        print("{read_stripped(addr<void?>(slot))} {read_via_addr(cell)} {write_copy(cell, 8)} {walk_copy(addr(cell), 1) == null} {copy_from(addr<void?>(slot), 4)} {scribble_value(3)}\n")
    }
}
