options gen2
options no_aot
options stack = 1_048_576

require daslib/aot_standalone
require daslib/aot_cpp
require daslib/ast_boost
require daslib/strings_boost
require daslib/fio
require math
require daslib/rtti
require strings
require dastest/testing_boost public

def private standalone_cop(var cop : CodeOfPolicies) {
    cop.threadlock_context = false
    cop.aot = false
    cop.aot_module = true
    cop.aot_lib = false
    cop.fail_on_lack_of_aot_export = true
    cop.ignore_shared_modules = false
    cop.version_2_syntax = true
    cop.tune_frozen = true
}

def private generate_standalone_files(t : T?; fixture, out_dir : string; cross_platform : bool = false; stack_policy : uint = 0u) : tuple<header : string; source : string> {
    let input = path_join(get_das_root(), "tests/aot/{fixture}.das")
    using() $(var cop : CodeOfPolicies) {
        standalone_cop(cop)
        if (stack_policy != 0u) {
            cop.stack = stack_policy
        }
        ast_gc_guard() {
            standalone_aot(input, out_dir, cross_platform, false, cop)
        }
    }
    var files : tuple<header : string; source : string>
    let generated = path_join(out_dir, "{fixture}.das.cpp")
    fopen(generated, "rb") $(fr) {
        if (fr != null) {
            files.source = fread(fr)
        }
    }
    fopen(path_join(out_dir, "{fixture}.das.h"), "rb") $(fr) {
        if (fr != null) {
            files.header = fread(fr)
        }
    }
    if (empty(files.source)) {
        t |> failure("standalone_aot produced no {generated}")
    }
    remove(generated)
    remove(path_join(out_dir, "{fixture}.das.h"))
    return files
}

def private generate_standalone_source(t : T?; fixture, out_dir : string) : string {
    return generate_standalone_files(t, fixture, out_dir).source
}

//! the emitted C++ name whose prefix is `name_prefix`, read off its prototype
def private emitted_name(source, name_prefix : string) : string {
    let at = find(source, " {name_prefix}")
    return "" if (at < 0)
    let sp = find(source, " ", at + 1)
    return sp < 0 ? "" : slice(source, at + 1, sp)
}

//! the definition of the emitted function whose C++ name starts with `name_prefix`: the header
//! line that does not end in `;` through the closing brace in column 0
def private emitted_body(source, name_prefix : string) : string {
    var body = ""
    peek_data(source) $(d) {
        var at = find(d, " {name_prefix}")
        while (at >= 0) {
            let eol = find(d, "\n", at)
            break if (eol < 0)
            if (int(d[eol - 1]) != ';') {
                let close = find(d, "\n}\n", eol)
                body = close < 0 ? "" : slice(d, at, close + 2)
                break
            }
            at = find(d, " {name_prefix}", eol)
        }
    }
    return body
}

//! the regular (per-module) AOT unit of a fixture under the same policies as the standalone one
def private generate_aot_source(t : T?; fixture : string) : string {
    let input = path_join(get_das_root(), "tests/aot/{fixture}.das")
    var source = ""
    using() $(var cop : CodeOfPolicies) {
        standalone_cop(cop)
        ast_gc_guard() {
            source = aot(input, false, false, cop)
        }
    }
    t |> success(!empty(source), "aot produced no source for {fixture}")
    return source
}

def private expect_emit_failure(t : T?; fixture, out_dir : string) {
    let input = path_join(get_das_root(), "tests/aot/{fixture}.das")
    var ok = true
    using() $(var cop : CodeOfPolicies) {
        standalone_cop(cop)
        ast_gc_guard() {
            ok = standalone_aot(input, out_dir, false, false, cop)
        }
    }
    t |> success(!ok, "standalone_aot reports failure")
    let generated = path_join(out_dir, "{fixture}.das.cpp")
    t |> success(!stat(generated).is_valid, "no output is written on failure")
}

def private brace_balance(text : string) : int {
    var balance = 0
    peek_data(text) $(bytes) {
        for (byte in bytes) {
            let ch = int(byte)
            if (ch == '{') {
                balance ++
            } elif (ch == '}') {
                balance --
            }
        }
    }
    return balance
}

//! every `{ 0x<hash>, ` row of the generated AOT factory table
def private factory_hashes(source : string) : table<string> {
    var out : table<string>
    for (line in split(source, "\n")) {
        let s = strip(line)
        continue if (!starts_with(s, "\{ 0x"))
        let comma = find(s, ",")
        continue if (comma < 0)
        out |> insert(strip(slice(s, 1, comma)))
    }
    return <- out
}

//! the AOT hash of every FunctionInfo row - the 2nd `0x` argument; the strings ahead of it may
//! carry commas, so it is read positionally
def private row_aot_hashes(source : string) : array<string> {
    var out : array<string>
    for (line in split(source, "\n")) {
        let at = find(line, "FunctionInfo(")
        continue if (at < 0)
        let tail = slice(line, at)
        let first = find(tail, ", 0x")
        continue if (first < 0)
        let rest = slice(tail, first + 2)
        let second = find(rest, ", 0x")
        continue if (second < 0)
        let hash_at = slice(rest, second + 2)
        let comma = find(hash_at, ",")
        continue if (comma < 0)
        out |> push(slice(hash_at, 0, comma))
    }
    return <- out
}

[test]
def test_standalone_emit(t : T?) {   // nolint:STYLE038 - flat list of emit subcases
    var tmp_err : string
    let out_dir = create_temp_directory("standalone_emit", tmp_err)
    if (!empty(tmp_err)) {
        t |> failure("cannot create temp directory: {tmp_err}")
        return
    }

    t |> run("a context with no init functions still closes its constructor") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_novars_fixture", out_dir)
        t |> success(find(source, "Standalone::Standalone() : Context(16384/*stack*/) \{") >= 0, "constructor was emitted with the default stack size")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("the context stack and heap hints honor the script's options") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_options_fixture", out_dir)
        t |> success(find(source, ": Context(262144/*stack*/)") >= 0, "the ctor sizes the stack from options stack")
        t |> success(find(source, "policies.heap_size_hint = 123456;") >= 0, "heap_size_hint comes from the script's options")
        t |> success(find(source, "policies.string_heap_size_hint = 234567;") >= 0, "string_heap_size_hint comes from the script's options")
        t |> success(find(source, "unused_interpreted_helper") < 0, "an unused [no_aot] function neither blocks emission nor emits")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("an explicit stack option below the floor is honored exactly") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_small_stack_fixture", out_dir)
        t |> success(find(source, ": Context(4096/*stack*/)") >= 0, "options stack = 4096 emits 4096 - embedded targets budget the stack")
    }

    t |> run("an explicit stack of zero keeps the floor - Context(0) has no stack at all") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_zero_stack_fixture", out_dir)
        t |> success(find(source, ": Context(16384/*stack*/)") >= 0, "options stack = 0 falls back to the floored default")
    }

    t |> run("a sub-floor policies stack without an explicit option is raised to the floor") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_novars_fixture", out_dir, stack_policy = 8192u)
        t |> success(find(files.source, ": Context(16384/*stack*/)") >= 0, "policies.stack = 8192 still gets the 16K floor")
    }

    t |> run("with no stack option the policies' stack size decides") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_novars_fixture", out_dir, stack_policy = 32768u)
        t |> success(find(files.source, ": Context(32768/*stack*/)") >= 0, "the ctor sizes the stack from CodeOfPolicies")
    }

    t |> run("forward declarations spell types the way the definitions do") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_tuple_fixture", out_dir)
        t |> success(find(source, "TTuple<") >= 0, "the tuple definition was emitted")
        t |> success(find(source, "AutoTuple<") < 0, "cross-platform spelling leaked into a native emit")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("the constructor runs the init script and then the [init] functions") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_init_fixture", out_dir)
        let ctor_pos = find(source, "Standalone::Standalone()")
        t |> success(ctor_pos >= 0, "constructor was emitted")
        let init_script_pos = find(source, "__init_script(&context, true);")
        t |> success(init_script_pos > ctor_pos, "the constructor calls __init_script")
        let zero_pos = find(source, "memset(context.globals, 0, context.getGlobalSize());")
        t |> success(zero_pos > ctor_pos && zero_pos < init_script_pos, "globals are zeroed before the init script runs")
        let tail = slice(source, init_script_pos)
        t |> success(find(tail, "record_init") >= 0, "the [init] function is called after __init_script")
        t |> success(find(source, "runInitScript") < 0, "the constructor does not call runInitScript")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("required modules' globals emit in dependency order before the entry module's") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_cross_module_fixture", out_dir)
        let a_pos = find(files.source, "/*g_dep_a*/")
        let b_pos = find(files.source, "/*g_dep_b*/")
        let main_pos = find(files.source, "/*g_main*/")
        t |> success(a_pos >= 0 && b_pos >= 0 && main_pos >= 0, "all three globals were emitted")
        t |> success(a_pos < b_pos && b_pos < main_pos, "dependency-first order: dep_a, dep_b, entry module")
        t |> success(find(files.header, "static_assert(sizeof(DepSpan)") >= 0, "a required module's struct definition is in the header")
        t |> success(find(files.header, "struct _standalone_dep_a::DepSpan") >= 0, "the method spells the required module's struct by its public name")
    }

    t |> run("a required module's used functions emit into the one translation unit, and link their externs' modules") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_foreign_call_fixture", out_dir)
        t |> success(find(source, "// @_standalone_dep_call::dep_probe ") >= 0, "the foreign function has a row in the function table")
        t |> success(find(source, "// @_standalone_dep_call::DepProbe`method ") >= 0, "the foreign class method has a row: it dispatches through its Func slot")
        t |> success(find(source, "__cwd_rename_at") >= 0 || find(source, "builtin_getcwd") >= 0, "the foreign function's body is emitted, not just declared")
        t |> success(find(source, "\"fio_core\", &::register_Module_FIO") >= 0, "an extern reached only from the foreign function still registers its module")
        // the ctor sizes context.functions by one count and fills one info row per function; with
        // foreign rows in play the two are computed separately and must agree
        let alloc_marker = "context.code->allocate( "
        let alloc_at = find(source, alloc_marker)
        t |> success(alloc_at >= 0, "the ctor allocates the function table")
        if (alloc_at >= 0) {
            let after = slice(source, alloc_at + length(alloc_marker), length(source))
            let declared = to_int(slice(after, 0, find(after, "/*")))
            var rows = 0
            var at = find(source, ", FunctionInfo(\"")
            while (at >= 0) {
                rows++
                at = find(source, ", FunctionInfo(\"", at + 1)
            }
            t |> equal(rows, declared, "the table is sized by exactly the rows it fills, foreign rows included")
            t |> success(rows >= 4, "the foreign function, its class's method, ctor and finalizer are rows: {rows}")
        }
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("a call into a required module is a direct C++ call; [hybrid], [jit] and Func slots still resolve by name") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_foreign_call_fixture", out_dir)
        let entry = emitted_body(source, "call_foreign_")
        t |> success(!empty(entry), "the entry function body is found")
        let probe_name = emitted_name(source, "dep_probe_")
        t |> success(!empty(probe_name) && find(entry, "{probe_name}(__context__,") >= 0, "the entry body calls the foreign function by its C++ name: {probe_name}")
        t |> success(find(entry, "fnByMangledName(/*@_standalone_dep_call::dep_probe ") < 0, "no Context lookup for a function this unit defines")
        t |> success(find(entry, "fnByMangledName(/*@_standalone_dep_call::dep_hybrid ") >= 0, "a [hybrid] foreign function keeps the lookup - its body is patched at run time")
        t |> success(find(entry, "fnByMangledName(/*@_standalone_dep_call::dep_jit ") >= 0, "a [jit] foreign function keeps the lookup")
        for (shape in ["dep_pair_", "dep_text_", "dep_fill_"]) {
            let name = emitted_name(source, shape)
            t |> success(!empty(name) && find(entry, "{name}(__context__,") >= 0, "a foreign call returning a struct, taking a string, or filling an array by reference is direct: {shape}{name}")
        }
        t |> success(find(source, "fnByMangledName(/*@_standalone_dep_call::DepProbe`method ") >= 0, "a class method slot resolves through fnByMangledName - it needs a SimFunction")
        t |> success(find(source, "fnByMangledName(/*@_standalone_dep_call::DepProbe'__finalize ") >= 0, "the finalizer slot resolves through fnByMangledName")
    }

    t |> run("a generic two modules instantiate has one body, and a foreign body calls it directly") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_shared_generic_fixture", out_dir)
        let instance = emitted_name(source, "_Func_standalone_gen_modTicktwiceTick")
        t |> success(!empty(instance), "the generic instance is emitted")
        var headers = 0
        var at = find(source, " {instance} ( Context")
        while (at >= 0) {
            headers++
            at = find(source, " {instance} ( Context", at + 1)
        }
        t |> equal(headers, 2, "the instance is declared once and defined once, whichever module's copy won")
        let dep_body = emitted_body(source, "dep_use_")
        t |> success(find(dep_body, "{instance}(__context__,") >= 0, "the required module's body calls the instance by its C++ name, not through a lookup")
        t |> success(find(dep_body, "fnByMangledName") < 0, "no Context lookup inside the required module's body")
    }

    t |> run("a regular AOT unit keeps the Context lookup for a call into another module") @(t : T?) {
        let source = generate_aot_source(t, "_standalone_foreign_call_fixture")
        t |> success(find(source, "fnByMangledName(/*@_standalone_dep_call::dep_probe ") >= 0, "the foreign body lives in another unit, so the call resolves by name")
    }

    t |> run("struct and enum signatures put the type definitions in the header") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_struct_sig_fixture", out_dir)
        t |> success(find(files.header, "static_assert(sizeof(Pair)") >= 0, "the struct definition is in the header")
        t |> success(find(files.header, "enum class Mode") >= 0, "the enum definition is in the header")
        t |> success(find(files.header, "-> struct ctx__standalone_struct_sig_fixture::Pair") >= 0, "the method spells the struct by its public name")
        t |> success(find(files.header, "DAS_COMMENT(enum) ctx__standalone_struct_sig_fixture::Mode") >= 0, "the method spells the enum by its public name")
        t |> success(find(files.header, "pair_sum") >= 0, "the struct-taking method is declared")
        t |> success(find(files.header, "namespace  \{") < 0, "no 'namespace  \{' (empty module name) in the header")
        t |> success(find(files.header, "#pragma once") >= 0, "the header guards against double inclusion")
        t |> success(find(files.header, "bin_serializer.h") >= 0, "required-module includes are in the header")
        t |> success(find(files.source, "class Standalone") >= 0 && find(files.source, "static_assert(sizeof(Pair)") >= 0,
            "the .cpp carries the header's text instead of including it, so one copy defines the types")
        t |> success(find(files.source, "#pragma once") < 0, "no #pragma once in the .cpp - it is a main file")
        t |> success(find(files.source, "#include \"_standalone_struct_sig_fixture.das.h\"") < 0,
            "the .cpp does not include the header it inlined")
        t |> equal(brace_balance(files.header), 0, "unbalanced braces in the generated header")
        t |> equal(brace_balance(files.source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("cross-platform debug info spells entry-module types by their public name") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_struct_sig_fixture", out_dir, true)
        t |> success(find(files.source, "offsetof(ctx__standalone_struct_sig_fixture::Pair,") >= 0, "offsetof names the qualified struct")
        t |> success(find(files.source, "TypeSize<ctx__standalone_struct_sig_fixture::Pair>") >= 0, "TypeSize names the qualified struct")
        t |> success(find(files.source, "DAS_COMMENT(enum) ctx__standalone_struct_sig_fixture::Mode>") >= 0, "TypeSize names the qualified enum")
        t |> success(find(files.source, "offsetof(Pair,") < 0, "no unqualified offsetof survives")
        t |> success(find(files.source, "TypeSize<Pair>") < 0, "no unqualified struct TypeSize survives")
        t |> equal(brace_balance(files.source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("a compile-time macro stays out of the standalone context") @(t : T?) {
        // AOT mode used to force-export every generated struct ctor, macro classes
        // included; that rooted the whole macro into the simulated context and the
        // [no_aot] check below then reported its visit() as an unemittable function.
        let source = generate_standalone_source(t, "_standalone_macro_fixture", out_dir)
        t |> success(find(source, "5050") >= 0, "the macro's compile-time sum is baked into the source")
        t |> success(find(source, "struct BakeSum") < 0, "the macro class is not emitted (a comment naming it is fine)")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("a used [no_aot] function is a collected error, not a runtime crash") @(t : T?) {
        expect_emit_failure(t, "_standalone_no_aot_fixture", out_dir)
    }

    t |> run("every function-table row names a body the factory table registers") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_cross_module_fixture", out_dir)
        let factory <- factory_hashes(source)
        let rows <- row_aot_hashes(source)
        t |> success(!empty(factory) && !empty(rows), "the emission has a factory table and rows ({length(factory)} / {length(rows)})")
        let unbound <- [for (h in rows); h; where !(factory |> key_exists(h))]
        t |> equal(length(unbound), 0, "rows whose AOT hash no factory entry registers: {join(unbound, " ")}")
    }

    t |> run("a required module's [init] is called from the constructor too") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_foreign_init_fixture", out_dir)
        let init_script_pos = find(source, "__init_script(&context, true);")
        t |> success(init_script_pos >= 0, "the constructor calls __init_script")
        let tail = slice(source, init_script_pos)
        t |> success(find(tail, "dep_record_init") >= 0, "the required module's [init] is called after __init_script")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("before_emit receives the compiled program") @(t : T?) {
        let input = path_join(get_das_root(), "tests/aot/_standalone_novars_fixture.das")
        var saw_program = false
        using() $(var cop : CodeOfPolicies) {
            standalone_cop(cop)
            ast_gc_guard() {
                standalone_aot(input, out_dir, false, false, cop) $(var program : ProgramPtr) {
                    saw_program = program != null
                }
            }
        }
        t |> success(saw_program, "the before_emit block ran with a non-null program")
        remove(path_join(out_dir, "_standalone_novars_fixture.das.cpp"))
        remove(path_join(out_dir, "_standalone_novars_fixture.das.h"))
    }

    t |> run("a context reaching no C++ module beyond builtin registers nothing") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_novars_fixture", out_dir)
        t |> success(find(files.header, "standalone_modules.h") < 0, "no registry header for the registry-free tier")
        t |> success(find(files.header, "class Standalone : public Context \{") >= 0, "the context derives from Context alone")
        t |> success(find(files.source, "DECLARE_MODULE") < 0, "no module is declared")
    }

    t |> run("an extern call links and registers its module, builtin modules first") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_fio_fixture", out_dir)
        let source = files.source
        t |> success(find(source, " // require fio_core\n") >= 0, "fio_core is linked")
        t |> success(find(source, "fio_core - compile time only") < 0, "fio_core is not pruned")
        t |> success(find(source, "aot_builtin_fio.h") >= 0, "the fio AOT header is included")
        t |> success(find(files.header, "standalone_modules.h") >= 0, "the header includes the registry")
        t |> success(find(files.header, "class Standalone : public StandaloneModuleScope, public Context \{") >= 0, "the module scope is the first base, ahead of Context")
        t |> success(find(source, "DECLARE_MODULE(Module_FIO);") >= 0, "Module_FIO is declared")
        let builtin_row = find(source, "\{ \"$\", &::register_Module_BuiltIn, 0 \}")
        let strings_row = find(source, "&::register_Module_Strings")
        let fio_row = find(source, "&::register_Module_FIO")
        t |> success(builtin_row >= 0, "the builtin module is a row at rank 0")
        t |> success(strings_row > builtin_row && fio_row > strings_row, "strings registers before fio_core, whose constructor requires it by name")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("a handled type alone links its module") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_handle_fixture", out_dir)
        t |> success(find(source, " // require fio_core\n") >= 0, "fio_core is linked for the FILE handle")
        t |> success(find(source, "DECLARE_MODULE(Module_FIO);") >= 0, "Module_FIO is declared")
    }

    t |> run("a structure nothing reaches still keeps its field's module") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_orphan_struct_fixture", out_dir)
        t |> success(find(source, " // require fio_core\n") >= 0, "an unreferenced struct's handled field still links fio_core")
    }

    t |> run("a member pointer names a namespace the file declares") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_emit_paths_fixture", out_dir)
        // the qualifier the member pointer uses, whatever the entry module ends up being called
        let at = find(source, "das_safe_navigation_ptr<")
        t |> success(at >= 0, "the safe navigation emitted its member-pointer form")
        if (at >= 0) {
            let tail = slice(source, at)
            let amp = find(tail, ",&")
            let end = find(tail, "::Node::next>")
            t |> success(amp >= 0 && end > amp, "the member pointer is spelled in the expected shape")
            if (amp >= 0 && end > amp) {
                let ns = slice(tail, amp + 2, end)
                t |> success(!empty(ns), "the member pointer is qualified")
                t |> success(find(source, "namespace {ns} \{") >= 0,
                    "`{ns}` is a namespace the file declares - the das module name is not one")
            }
        }
    }

    t |> run("an emitted literal's trigraph is escaped") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_emit_paths_fixture", out_dir)
        t |> success(find(source, "\"what\\?\\?>\"") >= 0, "a literal's ?? is escaped, so no C compiler reads it as a trigraph")
        t |> success(find(source, "\"what??>\"") < 0, "the raw sequence does not survive")
        t |> equal(brace_balance(source), 0, "unbalanced braces in the generated C++")
    }

    t |> run("options gc and persistent_heap reach the context, so heap_collect is allowed there") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_gc_fixture", out_dir)
        t |> success(find(source, "policies.persistent_heap = true;") >= 0, "the persistent heap policy is on")
        t |> success(find(source, "context.gcEnabled = true;") >= 0, "the collector's own flag is on")
        t |> success(find(source, "VarInfo __gvar_info_") >= 0, "each global gets a debug info the collector walks")
        t |> success(find(source, "].debugInfo = &__gvar_info_") >= 0, "and the context's global table points at it")
        let plain = generate_standalone_source(t, "_standalone_gc_free_fixture", out_dir)
        t |> success(find(plain, "gcEnabled") < 0, "a program without options gc never names the flag - the nano context has none")
        t |> success(find(plain, "].debugInfo = &__gvar_info_") >= 0, "but its globals still carry their debug info, as under the interpreter")
    }

    t |> run("a module only dead code reaches stays compile-time-only") @(t : T?) {
        let files = generate_standalone_files(t, "_standalone_dead_fio_fixture", out_dir)
        t |> success(find(files.source, "fio_core - compile time only") >= 0, "fio_core is pruned: only an unused function names FILE? and calls its extern")
        t |> success(find(files.source, "DECLARE_MODULE") < 0, "no module is declared")
        t |> success(find(files.header, "StandaloneModuleScope") < 0, "the context has no module scope")
    }

    t |> run("the function table has one row per emitted function, numbered from zero") @(t : T?) {
        let source = generate_standalone_source(t, "_standalone_fio_fixture", out_dir)
        let total_pos = find(source, "context.totalFunctions = ")
        t |> success(total_pos >= 0, "totalFunctions is emitted")
        let digits_from = total_pos + length("context.totalFunctions = ")
        let total = to_int(slice(source, digits_from, find(source, "/*totalFunctions*/;", digits_from)))
        let rows = length(split(source, ", FunctionInfo(")) - 1
        t |> equal(rows, total, "one row per slot")
        t |> success(find(source, "\{0, FunctionInfo(") >= 0, "rows are numbered from zero")
        t |> success(find(source, "\{{total}, FunctionInfo(") < 0, "no row past the table")
    }

    t |> run("registration lists the builtin modules first with non-decreasing ranks") @(t : T?) {
        let input = path_join(get_das_root(), "tests/aot/_standalone_fio_fixture.das")
        var names : array<string>
        var ranks : array<int>
        using() $(var cop : CodeOfPolicies) {
            standalone_cop(cop)
            ast_gc_guard() {
                standalone_aot(input, out_dir, false, false, cop) $(var program : ProgramPtr) {
                    for (entry in standaloneModuleRegistration(program)) {
                        names |> push(string(entry.mod.name))
                        ranks |> push(entry.rank)
                    }
                }
            }
        }
        t |> success(length(names) >= 2, "the fixture registers more than the builtin module")
        t |> success(!empty(names) && names[0] == "$" && ranks[0] == 0, "the builtin module is first, at rank 0")
        t |> success((names |> find_index("strings")) < (names |> find_index("fio_core")), "strings precedes fio_core")
        for (i in range(1, length(ranks))) {
            t |> success(ranks[i] >= ranks[i - 1], "ranks never decrease")
        }
        remove(path_join(out_dir, "_standalone_fio_fixture.das.cpp"))
        remove(path_join(out_dir, "_standalone_fio_fixture.das.h"))
    }

    t |> run("a default C++ module only a compile-time macro loaded is not registered") @(t : T?) {
        let input = path_join(get_das_root(), "tests/aot/_standalone_macro_fixture.das")
        var names : array<string>
        using() $(var cop : CodeOfPolicies) {
            standalone_cop(cop)
            ast_gc_guard() {
                standalone_aot(input, out_dir, false, false, cop) $(var program : ProgramPtr) {
                    for (entry in standaloneModuleRegistration(program)) {
                        names |> push(string(entry.mod.name))
                    }
                }
            }
        }
        t |> success((names |> find_index("strings")) >= 0, "the fixture reaches strings, so the registration walk ran")
        t |> success((names |> find_index("rtti_core")) < 0, "rtti_core, loaded for the macro module's daslib/ast, is not registered")
        t |> success((names |> find_index("ast_core")) < 0, "ast_core, loaded for the macro module's daslib/ast, is not registered")
        remove(path_join(out_dir, "_standalone_macro_fixture.das.cpp"))
        remove(path_join(out_dir, "_standalone_macro_fixture.das.h"))
    }

    t |> run("DEFAULT_MODULE_ORDER mirrors register_builtin_modules_impl") @(t : T?) {
        var registrar : array<string>
        fopen(path_join(get_das_root(), "src/builtin/modules.cpp"), "rb") $(fr) {
            if (fr != null) {
                for (line in split(fread(fr), "\n")) {
                    let at = find(line, "NEED_MODULE(")
                    if (at >= 0) {
                        registrar |> push(slice(line, at + length("NEED_MODULE("), find(line, ")")))
                    }
                }
            }
        }
        t |> success(!empty(registrar), "src/builtin/modules.cpp lists the builtin registrations")
        t |> equal(length(registrar), length(DEFAULT_MODULE_ORDER), "the same number of modules on both sides")
        for (i in range(min(length(registrar), length(DEFAULT_MODULE_ORDER)))) {
            t |> equal(registrar[i], DEFAULT_MODULE_ORDER[i], "the same module at position {i}")
        }
    }

    rmdir(out_dir)
}
