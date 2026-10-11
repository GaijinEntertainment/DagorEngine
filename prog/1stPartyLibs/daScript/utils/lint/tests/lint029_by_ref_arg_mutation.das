options gen2
options auto_inline_functions = false
options _lint = "LINT029"   // the rule ships default-off; the fixture arms it

expect 50503:5

require daslib/lint

struct private Box {
    x : int
}


def bad_push(var xs : array<int>) {
    xs |> push(1)
}

def bad_second_state(tag : string; var b : Box) {
    b.x = length(tag)
}

def bad_receiver(var b : Box; delta : int) {
    b.x += delta
}

def bad_ref_scalar(var n : int&) {
    n = 5
}

def bad_pass_mutable(var xs : array<int>) {
    bad_push(xs)
}


def good_read_only(xs : array<int>) : int {
    return length(xs)
}

def good_returned(var xs : array<int>) : array<int> {
    return <- xs
}

def good_pure_grow(xs : array<int>) : array<int> {
    var r := xs
    r |> push(1)
    return <- r
}

def good_nolint(var xs : array<int>) {   // nolint:LINT029
    xs |> push(2)
}

struct private PtrPair {
    a : int?
    b : int?
}

def good_ptr_fields(var pp : PtrPair) {
    unsafe {
        *pp.a = 1
        *pp.b = 2
    }
}

def good_lambda_param() : int {
    let l <- @(var v : array<int>) {
        v |> push(1)
    }
    var xs : array<int>
    l |> invoke(xs)
    return length(xs)
}

def apply_each(var xs : array<int>; blk : block<(var v : int&) : void>) {   // nolint:LINT029
    for (x in xs) {
        blk |> invoke(x)
    }
}

def good_block_param() : int {
    var xs <- [1, 2, 3]
    xs |> apply_each() $(var v : int&) {
        v += 1
    }
    return xs[0]
}

[export]
def main() {
    var xs : array<int>
    bad_push(xs)
    var b = Box()
    bad_second_state("t", b)
    bad_receiver(b, 1)
    var n = 1
    bad_ref_scalar(n)
    bad_pass_mutable(xs)
    print("{good_read_only(xs)}\n")
    var moved <- good_returned(xs)
    var grown <- good_pure_grow(moved)
    good_nolint(grown)
    var pa = 0
    var pb = 0
    var pp = PtrPair(a = unsafe(addr(pa)), b = unsafe(addr(pb)))
    good_ptr_fields(pp)
    print("{pa + pb}\n")
    print("{length(grown)} {good_lambda_param()} {good_block_param()}\n")
}
