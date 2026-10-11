options gen2
options rtti = true
options strict_smart_pointers = false
options no_aot

require dastest/testing_boost public
require daslib/ast
require daslib/rtti
require strings

let FIXTURE = "{get_das_root()}/tests/ast/_fixture_jit_selection.das"

//! the fixture's function of that name, from the program's own module
def private fixture_function(prog : smart_ptr<Program>; name : string) : FunctionPtr {
    var found : FunctionPtr
    for_each_function(get_this_module(prog), name) $(var fn) {
        found = fn
    }
    return found
}

//! a function every program in the process shares: a builtin module's
def private shared_function() : FunctionPtr {
    var found : FunctionPtr
    for_each_function(find_module("strings"), "to_upper") $(var fn) {
        found = fn
    }
    return found
}

def private compile_fixture(t : T?; blk : block<(prog : smart_ptr<Program>) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true           // the fixture's functions survive the unused-symbol sweep as themselves
            cop.no_optimizations = true
            compile_file(FIXTURE, access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                if (!ok) {
                    t |> failure("the fixture does not compile: {issues}")
                    return
                }
                invoke(blk, program)
            }
        }
    }
}

[test]
def test_jit_selection(t : T?) {
    t |> run("the selection is the program's table - set, read, cleared - and the [jit] annotation is not it") @(t : T?) {
        compile_fixture(t) $(prog) {
            let jitted = fixture_function(prog, "jitted")
            let plain = fixture_function(prog, "plain")
            t |> success(jitted != null && plain != null, "the fixture's functions are found")
            return if (jitted == null || plain == null)
            t |> success(jitted.moreFlags.requestJit && !plain.moreFlags.requestJit, "[jit] is the annotation's bit")
            t |> success(!is_jit_selected(prog, jitted) && !is_jit_selected(prog, plain), "a fresh program selects nothing")
            set_jit_selected(prog, jitted, true)
            t |> success(is_jit_selected(prog, jitted) && !is_jit_selected(prog, plain), "set marks the one function")
            set_jit_selected(prog, plain, true)
            clear_jit_selection(prog)
            t |> success(!is_jit_selected(prog, jitted) && !is_jit_selected(prog, plain), "clear empties the table")
            t |> success(jitted.moreFlags.requestJit && !plain.moreFlags.requestJit, "the annotation is untouched by the stamp")
        }
    }
    t |> run("a shared module's function is selected per program, not per function") @(t : T?) {
        let fn = shared_function()
        t |> success(fn != null, "strings::to_upper is in the process")
        return if (fn == null)
        compile_fixture(t) $(a) {
            compile_fixture(t) $(b) {
                set_jit_selected(a, fn, true)
                t |> success(is_jit_selected(a, fn), "the program that selected it says so")
                t |> success(!is_jit_selected(b, fn), "the other program does not")
                set_jit_selected(a, fn, false)
                t |> success(!is_jit_selected(a, fn), "unset")
            }
        }
    }
    t |> run("a null program is a holder nobody filled and throws") @(t : T?) {
        let fn = shared_function()
        var none : Program?
        var why = ""
        try {
            let answered = is_jit_selected(none, fn)
            why = "answered {answered}"
        } recover {
            why = this_context().last_exception
        }
        t |> success(find(why, "is_jit_selected: null program") >= 0, "the null program is the error: {why}")
    }
}
