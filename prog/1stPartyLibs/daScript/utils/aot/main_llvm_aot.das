options gen2
// LLVM object AOT driver. Sibling of utils/aot/main.das: instead of emitting C++ AOT stubs, it
// emits one native .o per input through the LLVM JIT backend. Each object self-registers its
// functions into the AOT library at load (das_aot_register), so linkCppAot binds them as
// SimNode_Jit -- the same role the C++ AOT .cpp plays, with no interpreter penalty.
// Requires dasLLVM, so only a build that links that module can run this driver.
options stack = 1_048_576

require daslib/clargs
require daslib/rtti
require daslib/ast_boost

// side-effect require: bakes the jit_llvm simulate-macro into the module graph so simulate()
// emits the object when cop.jit_emit_object is set.
require llvm/daslib/llvm_jit_run   // nolint:STYLE030 -- side-effect require, see above


[CommandLineArgs]
struct Config {
    @clarg_doc = "Source .das to compile (repeatable); pairs by position with --aot-out"
    aot_in : array<string>

    @clarg_doc = "Object path for the matching --aot-in, without the .o suffix (repeatable)"
    aot_out : array<string>

    @clarg_doc = "Parse the sources as gen1 syntax"
    gen1 : bool

    @clarg_doc = "Parse gen2 constructor syntax inside gen1 sources"
    gen2_make : bool

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}


// Compile `input` jit-aware and emit `<output>.o`. The jit_llvm macro emits the object during
// simulate() (jit_emit_object gate); no explicit run_jit needed. Uses make_file_access("") so
// module resolution matches the C++-AOT path.
def aot_object_one(input : string, output : string, gen2 : bool, gen2_make : bool) : bool {
    var success = true
    var inscope access <- make_file_access("")
    access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.aot_module = true             // prepare finalizers/lambdas/generators for AOT-JIT codegen
            cop.jit_emit_object = true        // emit a .o + das_aot_register ctor, for static link
            cop.jit_dll_mode = false
            cop.threadlock_context = false
            cop.version_2_syntax = gen2
            cop.gen2_make_syntax = gen2_make
            cop.ignore_shared_modules = false
            // Extended RTTI: simulate-time macros (ast_typedecl via typeinfo(...)) generate no node
            // without it -> error[50503]. The interpreter and the C++-AOT path enable it too.
            cop.rtti = true
            // Object emission is a codegen step, not authoring, and lint already ran upstream of it.
            // no_global_variables is a lint-only style rule that the JIT path would otherwise apply
            // fatally to modules with legit globals (daslib/debugger.das) even though JIT handles them.
            cop.no_lint = true
            cop.tune_frozen = true            // AOT is a cross-box artifact (see utils/aot/main.das)
            // Off so no wrapper is spliced that turns a runtime extern argument into a compile-time
            // constant for the folder to execute -- fatal when the host stubs its C++ externs in the
            // AOT tool. Must match the host runtime's policies so both make the same inline/fold
            // decisions and the AOT hashes agree.
            cop.auto_inline_functions = false
            cop.jit_output_path := output     // the macro appends the .o suffix
            compile_file(input, access, unsafe(addr(mg)), cop) $(ok; var program : smart_ptr<Program>; issues) {
                if (!ok || program == null) {
                    to_log(LOG_ERROR, "aot-object: failed to compile `{input}`\n{issues}\n")
                    success = false
                    return
                }
                simulate(program) $(sok; var _pctx : smart_ptr<Context>; serrors) {
                    if (!sok) {
                        to_log(LOG_ERROR, "aot-object: failed to simulate `{input}`\n{serrors}\n")
                        success = false
                    }
                }
            }
        }
    }
    return success
}


[export]
def main() {
    var r <- parse_args(type<Config>, get_cli_arguments())
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "main_llvm_aot")
        return
    }
    let cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<Config>), "main_llvm_aot")
        return
    }
    if (length(cfg.aot_in) != length(cfg.aot_out)) {
        panic("--aot-in and --aot-out must be passed the same number of times")
    }
    var result = true
    for (i in range(length(cfg.aot_in))) {
        ast_gc_guard() {
            let ok = aot_object_one(cfg.aot_in[i], cfg.aot_out[i], !cfg.gen1, cfg.gen2_make)
            result = ok && result
        }
    }
    if (!result) {
        panic("LLVM-AOT object emission failed")
    }
}
