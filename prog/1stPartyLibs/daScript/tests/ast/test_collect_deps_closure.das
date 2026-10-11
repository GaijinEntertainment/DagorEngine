options gen2
options indenting = 4
options auto_inline_functions = false   // a spliced callee leaves no call edge for the probe to find

//! collect_dependencies returns the annotated function's whole closure - callees of callees,
//! the globals its body reads, and the functions those globals' initializers call - and
//! nothing outside it. The [probe_deps] patch fails the compile on any other answer, so the
//! test's job is to compile and run.

require dastest/testing_boost public
require _test_collect_deps_closure_helper

// runtime-only bodies: a constant one is folded at its call site before patch() runs, and the
// edge to it with it
def leaf() : int {
    return int(ref_time_ticks() & 0l) + 1
}

def helper_for_global() : int {
    return int(ref_time_ticks() & 0l) + 2
}

let g_dep = helper_for_global()

def mid() : int {
    return leaf() + g_dep
}

def unrelated() : int {
    return 42
}

[probe_deps(functions = "helper_for_global,leaf,mid,root", variables = "g_dep")]
def root() : int {
    return mid()
}

[probe_deps(functions = "leaf,twice", variables = "")]
def twice() : int {
    return leaf() + leaf()
}

[test]
def test_collect_deps_closure(t : T?) {
    t |> run("the probes accepted the closures at compile time; the functions still run") <| @(t : T?) {
        t |> equal(3, root())
        t |> equal(2, twice())
        t |> equal(42, unrelated())
    }
}
