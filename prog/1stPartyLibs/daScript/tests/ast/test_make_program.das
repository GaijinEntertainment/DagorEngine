options gen2
options multiple_contexts
options no_aot

require dastest/testing_boost public
require daslib/ast_boost
require daslib/templates_boost
require daslib/debugger
require strings

def private add_twice(mod : Module?) {
    var fn = qmacro_function("twice") $(x : int) : int {
        return x * 2
    }
    mod |> add_function(fn)
}

def private add_run(mod : Module?) {
    var fn = qmacro_function("run") $(var out : int?) {
        var a : array<int>
        a |> push(3)
        *out = twice(length(a)) + int(sqrt(16.0))
    }
    fn.flags.exports = true
    mod |> add_function(fn)
}

def private add_calls_twice_with_string(mod : Module?) {
    var fn = qmacro_function("bad") $() {
        twice("three")
    }
    mod |> add_function(fn)
}

def private has_error(program : ProgramPtr; text : string) : bool {
    for (err in program.errors) {
        return true if (find(string(err.what), text) >= 0)
    }
    return false
}

[test]
def test_built_program_runs(t : T?) {
    t |> run("a built module, math and builtin.das all reach the main module, and the program runs") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            make_program(cop) $ {
                let helpers = make_module("mp_helpers") $(var mod) {
                    add_twice(mod)
                }
                t |> success(helpers != null, "the helpers module compiles")
                var inscope program <- make_main_module() $(var mmain) {
                    mmain |> add_module_require(helpers, false)
                    mmain |> add_module_require(get_module("math"), false)
                    add_run(mmain)
                }
                t |> success(!program.failed, "the main module compiles")
                return if (program.failed)
                simulate(program) $(sok; context; serrors) {
                    t |> success(sok, "the program simulates {serrors}")
                    return if (!sok)
                    var out = 0
                    unsafe {
                        invoke_in_context(context, "run", addr(out))
                    }
                    t |> equal(6, out)
                }
            }
        }
    }
}

[test]
def test_built_program_errors(t : T?) {
    t |> run("a type error in the main module comes back on the program") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let helpers = make_module("mp_helpers") $(var mod) {
                    add_twice(mod)
                }
                var inscope program <- make_main_module() $(var mmain) {
                    mmain |> add_module_require(helpers, false)
                    add_calls_twice_with_string(mmain)
                }
                t |> success(program.failed, "the program failed")
                t |> success(has_error(program, "twice"), "the error names the call")
            }
        }
    }
    t |> run("a failed module answers null, and the main module answers its program without running its block") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let broken = make_module("mp_broken") $(var mod) {
                    add_calls_twice_with_string(mod)
                }
                t |> success(broken == null, "the broken module answers null")
                var ran = false
                var inscope program <- make_main_module() $(var mmain) {
                    ran = true
                }
                t |> success(!ran, "the main module's block did not run")
                t |> success(program.failed, "the program carries the module's failure")
                t |> success(has_error(program, "twice"), "the module's own error is on it")
            }
        }
    }
    t |> run("a module name already in the group is refused") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let first = make_module("mp_twin") $(var mod) {
                    add_twice(mod)
                }
                let second = make_module("mp_twin") $(var mod) {
                    add_twice(mod)
                }
                t |> success(first != null && second == null, "the second module of that name is refused")
                var inscope program <- make_main_module() $(var mmain) {
                    pass
                }
                t |> success(has_error(program, "twin"), "the refusal names the module")
            }
        }
    }
}

[test]
def test_build_calls_outside_make_program(t : T?) {
    var outside = false
    try {
        make_module("mp_outside") $(var _mod) {
            pass
        }
    } recover {
        outside = true
    }
    t |> success(outside, "make_module outside make_program throws")
    var outside_main = false
    try {
        var inscope program <- make_main_module() $(var _mmain) {
            pass
        }
    } recover {
        outside_main = true
    }
    t |> success(outside_main, "make_main_module outside make_program throws")
    var nameless = false
    using() $(var cop : CodeOfPolicies) {
        make_program(cop) $ {
            try {
                make_module("") $(var _mod) {
                    pass
                }
            } recover {
                nameless = true
            }
        }
    }
    t |> success(nameless, "a nameless module throws")
}

[test]
def test_build_panic_leaves_make_program(t : T?) {
    var caught = false
    try {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                make_module("mp_panics") $(var _mod) {
                    panic("builder failed")
                }
            }
        }
    } recover {
        caught = true
    }
    t |> success(caught, "the panic reaches the caller")
    var after = false
    try {
        make_module("mp_after") $(var _mod) {
            pass
        }
    } recover {
        after = true
    }
    t |> success(after, "no make_program scope is left behind")
    using() $(var cop : CodeOfPolicies) {
        make_program(cop) $ {
            let again = make_module("mp_panics") $(var mod) {
                add_twice(mod)
            }
            t |> success(again != null, "the same module name builds in a fresh make_program")
        }
    }
}

[test]
def test_build_requires_refused(t : T?) {
    t |> run("add_module_require refuses a null module and a module requiring itself") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                var inscope program <- make_main_module() $(var mmain) {
                    t |> success(!(mmain |> add_module_require(get_module("no_such_module"), false)), "a null module is refused")
                    t |> success(!(mmain |> add_module_require(mmain, true)), "a module can not require itself")
                }
                t |> success(!program.failed, "the program still compiles")
            }
        }
    }
    t |> run("two modules that require each other are refused") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let first = make_module("mp_first") $(var mod) {
                    add_twice(mod)
                }
                let second = make_module("mp_second") $(var mod) {
                    first |> add_module_require(mod, true)
                    mod |> add_module_require(first, true)
                }
                t |> success(second == null, "the second module fails")
                var inscope program <- make_main_module() $(var _mmain) {
                    pass
                }
                t |> success(has_error(program, "require each other"), "the error names the cycle")
            }
        }
    }
    t |> run("a module from another make_program is refused, and the outer scope comes back") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let outer = make_module("mp_outer_mod") $(var mod) {
                    add_twice(mod)
                }
                make_program(cop) $ {
                    let inner = make_module("mp_inner_mod") $(var mod) {
                        mod |> add_module_require(outer, false)
                    }
                    t |> success(inner == null, "the inner module fails")
                    var inscope program <- make_main_module() $(var _mmain) {
                        pass
                    }
                    t |> success(has_error(program, "not in this program's module group"), "the error names the group")
                }
                let after = make_module("mp_after_inner") $(var mod) {
                    add_twice(mod)
                }
                t |> success(after != null && outer.name == "mp_outer_mod", "the outer scope builds on after the inner one")
            }
        }
    }
}

[test]
def test_build_module_names(t : T?) {
    for (name in fixed_array("math", "a::b", "1abc", "__")) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                let built = make_module(name) $(var mod) {
                    add_twice(mod)
                }
                t |> success(built == null, "'{name}' is refused")
                var inscope program <- make_main_module() $(var _mmain) {
                    pass
                }
                t |> success(has_error(program, "can't build module"), "the refusal of '{name}' is on the program")
            }
        }
    }
}

[test]
def test_build_scope_rules(t : T?) {
    t |> run("a function built outside the module's block is refused") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                var outside = qmacro_function("stray") $(x : int) : int {
                    return x
                }
                let strays = make_module("mp_strays") $(var mod) {
                    mod |> add_function(outside)
                }
                t |> success(strays == null, "the module fails")
                var inscope program <- make_main_module() $(var _mmain) {
                    pass
                }
                t |> success(has_error(program, "was built outside the block"), "the error names the stray function")
            }
        }
    }
    t |> run("after a module fails, the next make_module answers null without running its block") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                make_module("mp_broken") $(var mod) {
                    add_calls_twice_with_string(mod)
                }
                var ran = false
                let after = make_module("mp_after_failure") $(var _mod) {
                    ran = true
                }
                t |> success(after == null && !ran, "the later module is skipped")
            }
        }
    }
    t |> run("a module built inside another module's block leaves the outer compile bound") @(t : T?) {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                var restored = false
                let outer = make_module("mp_env_outer") $(var mod) {
                    let inner = make_module("mp_env_inner") $(var imod) {
                        add_twice(imod)
                    }
                    restored = inner != null && compiling_module() == mod
                    add_twice(mod)
                }
                t |> success(outer != null && restored, "compiling_module is the outer module again")
            }
        }
    }
}

[test]
def test_build_export_all(t : T?) {
    for (export_all in fixed_array(false, true)) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = export_all
            make_program(cop) $ {
                var inscope program <- make_main_module() $(var mmain) {
                    add_twice(mmain)
                }
                t |> success(!program.failed, "the main module compiles")
                var kept = false
                for_each_function(get_this_module(program), "twice") $(_fn) {
                    kept = true
                }
                t |> success(kept == export_all, "export_all={export_all} decides whether the unused twice stays")
            }
        }
    }
}

[test]
def test_build_program_outlives_scope(t : T?) {
    var inscope kept : ProgramPtr
    var caught = false
    try {
        using() $(var cop : CodeOfPolicies) {
            make_program(cop) $ {
                var inscope built <- make_main_module() $(var mmain) {
                    add_twice(mmain)
                }
                move(kept) <| built
            }
        }
    } recover {
        caught = true
    }
    t |> success(caught, "make_program panics when a program it built outlives it")
}
