// test_modules.das — Module system integration tests
//
// Tests the daScript module resolution system by compiling and running
// various module configurations. Each test creates a FileAccess (optionally
// with a .das_project file), compiles a main script, simulates it, and
// invokes the exported "test" function in the child context.
//
// The module source files live in _modules/ (dastest skips _ prefixed dirs).
//
// Test scenarios:
//   1. test_module_main — basic module resolution via project.das_project
//      (requires foo via .das_module, bar.bar via dotted path)
//   2. test_module_main_inc — same project but with #include support
//      (main_inc.das uses #include "sample.inc")
//   3. test_module_main_default — no project file, plain FileAccess
//      (main_default.das uses relative requires that work without a project)
//   4. test_module_alias — module aliasing via third tuple field
//      (requires "sub/event as sub_event", testing alias resolution)
//   5. test_module_cdp — custom "profile" routing in project resolver
//      (profile module always resolved from project root, not relative)
//   6. test_module_unsafe — module_allowed_unsafe permission
//      (allows `implicit` parameters in named modules)
//   7. test_module_failed — module_allowed blocking a require
//      (strings_boost is blocked, compilation must fail)
//   8. test_module_conflict — module name collision detection
//      (two packages provide module "foo", compilation must fail)

options gen2
options multiple_contexts
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/rtti
require daslib/debugger
require dastest/testing_boost public


// Helper: compile a module test scenario and invoke its "test" function.
// Returns true if the compilation, simulation, and invocation all succeed.
// If use_project is true, loads the project.das_project from the given path.
// If use_project is false, uses a plain FileAccess (no project file).
def run_module_test(base_path, main_file : string; use_project : bool) : bool {
    var result = false
    let project_path = use_project ? "{base_path}/project.das_project" : ""
    let file_path = "{base_path}/{main_file}"
    var inscope access <- make_file_access(project_path)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                if (!ok) {
                    return
                }
                simulate(program) $(sok; context; serrors) {
                    if (!sok) {
                        return
                    }
                    unsafe {
                        invoke_in_context(context, "test")
                    }
                    result = true
                }
            }
        }
    }
    return result
}


// Helper: try to compile a module test scenario expecting FAILURE.
// Returns true if compilation fails (ok == false), meaning the module
// system correctly rejected the script.
def run_module_test_expect_failure(base_path, main_file : string) : bool {
    var result = false
    let project_path = "{base_path}/project.das_project"
    let file_path = "{base_path}/{main_file}"
    var inscope access <- make_file_access(project_path)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                if (!ok) {
                    // Compilation failed as expected
                    result = true
                }
            }
        }
    }
    return result
}


[test]
def test_module_main(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules"
    t |> run("basic module resolution via project.das_project") @(t : T?) {
        t |> success(run_module_test(base, "main.das", true))
    }
}

[test]
def test_module_main_inc(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules"
    t |> run("module resolution with #include support") @(t : T?) {
        t |> success(run_module_test(base, "main_inc.das", true))
    }
}

[test]
def test_module_main_default(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules"
    t |> run("module resolution without project file") @(t : T?) {
        t |> success(run_module_test(base, "main_default.das", false))
    }
}

[test]
def test_module_alias(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules/alias"
    t |> run("module aliasing via third tuple field") @(t : T?) {
        t |> success(run_module_test(base, "main.das", true))
    }
}

[test]
def test_module_cdp(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules/cdp"
    t |> run("custom profile routing in project resolver") @(t : T?) {
        t |> success(run_module_test(base, "main.das", true))
    }
}

[test]
def test_module_unsafe(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules/unsafe"
    t |> run("module_allowed_unsafe permission for implicit params") @(t : T?) {
        t |> success(run_module_test(base, "main.das", true))
    }
}

[test]
def test_module_failed(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules/failed"
    t |> run("module_allowed blocks strings_boost — compilation must fail") @(t : T?) {
        t |> success(run_module_test_expect_failure(base, "main.das"))
    }
}

[test]
def test_module_conflict(t : T?) {
    let base = "{get_das_root()}/tests/module_tests/_modules/conflict"
    t |> run("same module name from different packages — compilation must fail") @(t : T?) {
        t |> success(run_module_test_expect_failure(base, "main.das"))
    }
}
