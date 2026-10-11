options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _lint = "LINT030"   // the rule ships default-off; the fixture arms it
// LINT030: a call whose argument list reads a variable that a nested call in another
// argument writes by reference. The order arguments are evaluated in is not defined and
// differs between tiers, so the read sees the value before or after the write by the tier's
// choice.

expect 50503:12

require daslib/lint

def bump(var x : int&) : int {
    x++
    return x
}

def fill(var s : string&) : int {
    s = "filled"
    return 0
}

def grow(var xs : array<int>) : int {
    xs |> push(1)
    return length(xs)
}

def pair(a, b : int) : int {
    return a * 10 + b
}

def report(rc : int; text : string) : bool {
    return rc == 0 && !empty(text)
}

def take_ref(s : string&; rc : int) : int {
    return rc + length(s)
}

def bump_copy(var x : int) : int {
    x++
    return x
}

def call_block(blk : block<(q : int) : int>) : int {
    return invoke(blk, 1)
}

class Consts {
    def make(x : int) : int {
        return x * 2
    }
}

struct Draft {
    offs : array<int>
    n : int
    consts : Consts?
}

def read_through_method(var d : Draft; x : int) : int {
    return d.consts->make(x)
}

def reads_pointer_field(var d : Draft) : int {   // nolint:LINT014 - the var it never writes is the shape under test
    let c = d.consts
    return c == null ? 0 : 1
}

def repoint(var d : Draft; var c : Consts?) : int {
    d.consts = c
    return 0
}

struct Holder {
    p : Draft?
}

def plan(var d : Draft) : int {
    d.n++
    return d.n
}

def swap_draft(var h : Holder; var other : Draft) : int {
    h.p = unsafe(addr(other))
    return 0
}

def data_ptr(var xs : array<int>) : int? {
    return unsafe(addr(xs[0]))
}

def poke(var x : int&) : int {
    var p = unsafe(addr(x))
    unsafe {
        *p = 5
    }
    return 0
}

def count_and_read(var d : Draft; var hits : int&) : int {   // nolint:LINT014 - the var it never writes is the shape under test
    hits++
    return d.n
}

def reads_through(var d : Draft) : int {
    var hits = 0
    return count_and_read(d, hits)
}

def bumps_through(var d : Draft) : int {
    return bump(d.n)
}

def bump_ints_only(var v : auto(T)&) : int {
    static_if (typeinfo is_int(v)) {
        v++
    }
    return 0
}

// === Positives - a sibling argument reads what the nested call writes ===

def bad_read_after(var x : int) : int {
    return pair(bump(x), x)   // LINT030
}

def bad_read_before(var x : int) : int {
    return pair(x, bump(x))   // LINT030
}

def bad_out_parameter() : bool {
    var out : string
    return report(fill(out), out)   // LINT030
}

def bad_read_in_expression(var x : int) : int {
    return pair(bump(x), x + 1)   // LINT030
}

def bad_array(var xs : array<int>) : int {
    return pair(grow(xs), length(xs))   // LINT030
}

// the writing call sits inside an expression, not at the argument's root
def bad_writer_in_expression(var x : int) : int {
    return pair(1 + bump(x), x)   // LINT030
}

// a block handed to a nested call runs while that argument is evaluated - its reads are the
// argument's, unlike a block the outer call itself takes (below), which runs after every argument
def bad_block_invoked_by_sibling(var x : int) : int {
    return pair(bump(x), call_block($(q : int) : int { return x + q }))   // LINT030
}

// a field reached through a pointer the nested call can repoint is a value read, not an address
def bad_field_through_pointer(var h : Holder; var other : Draft) {
    h.p.offs |> push(swap_draft(h, other))   // LINT030
}

// an address the callee keeps and writes through is a write, wherever the caller reads
def bad_write_through_kept_address(var x : int) : int {
    return pair(poke(x), x)   // LINT030
}

// a write two calls down still counts: bumps_through hands d.n to bump, which writes it
def bad_write_through_a_nested_callee(var d : Draft) : int {
    return pair(bumps_through(d), d.n)   // LINT030
}

// assigning a pointer field writes the variable, the pointee's fate aside
def bad_pointer_field_assigned(var d : Draft; var c : Consts?) : int {
    return pair(repoint(d, c), d.n)   // LINT030
}

// === Negatives ===

// the write is a statement of its own, the read comes after it
def good_sequenced(var x : int) : int {
    let bumped = bump(x)
    return pair(bumped, x)
}

// every argument only reads
def good_read_only(x : int) : int {
    return pair(pair(x, x), x)
}

// the nested call writes one variable, the sibling reads another
def good_other_variable(var x : int; y : int) : int {
    return pair(bump(x), y)
}

// the outer call takes the written variable by reference - the callee sees its final state
// whatever the order
def good_outer_takes_by_ref() : int {
    var out : string
    return take_ref(out, fill(out))
}

// the nested call writes its own copy - a by-value parameter reaches nothing of the caller's
def good_by_value_parameter(var x : int) : int {
    return pair(bump_copy(x), x)
}

// a lambda argument runs later, not while the arguments are evaluated
def good_lambda_reads_later(var x : int) : int {
    let read = @(y : int) : int { return y }
    return pair(bump(x), invoke(read, 0))
}

// a plain field of the written variable handed to the outer call by reference is an address
// no write moves - the callee sees the field's final state whatever the order
def good_field_of_ref_target(var d : Draft) {
    d.offs |> push(plan(d))
}

// a callee that takes `var` only to hand out a pointer writes nothing during the call
def good_pointer_handout(var xs : array<int>) : int {
    return pair(data_ptr(xs) != null ? 1 : 0, length(xs))
}

// a callee that passes the variable on to a function writing some OTHER argument leaves it
// alone: the compiler marks every writable argument of such a call, the scan asks the callee
def good_read_beside_another_write(var d : Draft) : int {
    return pair(reads_through(d), d.n)
}

// a method called through a pointer the variable holds reaches the pointee, never the variable
def good_method_through_pointer_field(var d : Draft) : int {
    return pair(read_through_method(d, 1), d.n)
}

// copying a pointer out of the variable is a read, whatever the copy is later used for
def good_pointer_field_read(var d : Draft) : int {
    return pair(reads_pointer_field(d), d.n)
}

// a generic's instances are read one by one: the float instance of bump_ints_only writes nothing ...
def good_generic_instance_that_reads(var f : float) : int {
    return pair(bump_ints_only(f), int(f))
}

// ... and the int instance, scanned after it, still counts as the write it is
def bad_generic_instance_that_writes(var x : int) : int {
    return pair(bump_ints_only(x), x)   // LINT030
}

// a marked site is the author's call
def good_marked(var x : int) : int {
    return pair(bump(x), x)   // nolint:LINT030
}

// a call inside a block argument's body runs later too - its reads stay inside the block
def good_block_call_reads_later(var x : int) : int {
    var seen : int
    pair(bump(x), 0) |> with_value() $(v : int) {
        seen = pair(x, v)
    }
    return seen
}

def with_value(v : int; blk : block<(v : int) : void>) : int {
    invoke(blk, v)
    return v
}
