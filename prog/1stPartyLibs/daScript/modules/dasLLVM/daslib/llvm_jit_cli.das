options gen2
options indenting = 4

module llvm_jit_cli shared private

require daslib/clargs

//! Shared JIT command-line option schema.
//!
//! Declared here (rather than inside `llvm_macro`) so both the `jit_llvm`
//! simulate-macro and the repo's `jit` driver parse the exact same flags.

[CommandLineArgs]
struct public JitCliOptions {
    @clarg_name = "jit-opt-level"
    @clarg_doc = "JIT: LLVM optimization level (0..3); 0 disables all passes"
    opt_level : Option<int>

    @clarg_name = "jit-size-level"
    @clarg_doc = "JIT: LLVM size-optimization level (0..2)"
    size_level : Option<int>

    @clarg_name = "jit-split-modules"
    @clarg_doc = "JIT (dll mode, and a host-target -exe on request - an exe's default stays one unit): partition codegen per das-module - one .o per module, linked into the ONE cached DLL (an exe links them once, no cache, the entry in a partition of its own). -1 = split + auto threads (JobQue count; the default); 1 = split with serial emit; N>1 = split + N optimize/emit threads; 0 = monolithic emission, one context with full cross-module inlining, which split partitions lose (thread count never changes the artifact - split DLLs cache under one distinct key). Script-level pin for the DLL path: options jit_split_modules = N; an exe splits only when its build command asks"
    split_modules : Option<int>

    @clarg_name = "jit-obj-cache"
    @clarg_doc = "JIT (dll mode, with --jit-split-modules): reuse cached per-module partition objects, keyed by a chained module-prefix hash - a changed das-module re-emits itself and every module after it in program order; unchanged prefix modules link their cached .o. Require order is therefore the cache layout: place hot-edit modules as late in the require chain as their dependencies allow. 1 = on (default under split; off under a pinned jit_output_path, where no GC runs); 0 = always re-emit with positional object names. Script-level pin: options jit_obj_cache = false"
    obj_cache : Option<int>

    @clarg_name = "jit-lto"
    @clarg_doc = "JIT (-exe with --jit-split-modules, host target only): the partitions emit bitcode and the link runs LTO, so the split build keeps the one-unit form's cross-module inlining; the target cpu and features ride each function, so the LTO backend emits what the target machine would. The link is regular LTO (one merged module, its optimization serial) at level 3, the codegen level every exe pins: lld-link takes /opt:lldlto=3, a POSIX build links through clang++ with -flto -O3 (a GNU driver handles no bitcode, and a clang older than the LLVM daslang bundles reads none of its bitcode; --jit-path-to-linker names another clang). A build flag only, no script-level pin: an exe's form is its build command's"
    lto : Option<bool>

    @clarg_short = "g"
    @clarg_name = "jit-debug"
    @clarg_doc = "JIT: emit debug info / symbols"
    debug_info : Option<bool>

    @clarg_name = "jit-stack"
    @clarg_doc = "JIT: retain a logical daslang stack frame for every generated function and block"
    stack_frames : Option<bool>

    @clarg_short = "d"
    @clarg_name = "jit-dump"
    @clarg_doc = "JIT: dump generated LLVM IR"
    dump_ir : Option<bool>

    @clarg_name = "jit-target"
    @clarg_doc = "JIT: cross-compile target triple (use with -exe; e.g. wasm32-unknown-emscripten)"
    target : Option<string>

    @clarg_name = "jit-compile-only"
    @clarg_doc = "JIT: build + verify + optimize the module (print it with --jit-dump), then stop — no artifact written, no DLL loaded, no jitted function installed; the program runs interpreted. The cross-emission smoke rail (e.g. --jit-target=x86_64-unknown-linux-gnu with DAS_JIT_X64_FORCE_FEATURES)."
    compile_only : Option<bool>

    @clarg_name = "jit-dump-to"
    @clarg_doc = "JIT: write the --jit-dump IR to this file instead of the log"
    dump_ir_path : Option<string>

    @clarg_name = "jit-runtime-lib"
    @clarg_doc = "JIT: explicit path to libDaScript_runtime.a (wasm cross-compile); overrides auto-locate under web/output/lib/"
    runtime_lib : Option<string>

    @clarg_name = "jit-emit-object"
    @clarg_doc = "JIT (-exe wasm cross-compile): emit the linkable object only and skip the emcc link, so the caller (e.g. daspkg release wasm) runs emcc with module archives + flags. Object is written to -output verbatim."
    emit_object : Option<bool>

    @clarg_name = "jit-threads"
    @clarg_doc = "JIT (-exe wasm cross-compile): emit a -pthread-compatible object (+atomics,+bulk-memory) for linking against the threaded runtime archives (web/output64mt). The emcc link must also pass -pthread."
    threads : Option<bool>

    @clarg_name = "jit-check-abi"
    @clarg_doc = "JIT (-exe): emit a startup check that compares every used handled-type's HOST-baked size + field offsets against the TARGET runtime annotation and warns on stderr — reporting ALL divergences — on any mismatch; a clean run prints nothing and the program runs either way. Diagnostic for the cross-compile C++ ABI layout class (wasm). Off by default."
    check_abi : Option<bool>

    @clarg_name = "jit-sanitize"
    @clarg_doc = "JIT: instrument the emitted IR - address runs LLVM's asan pass, undefined runs LLVM's bounds check on every access to an object of known size, which reports through the ubsan runtime; address,undefined combines them and none turns both off. With no list, a daslang built under a sanitizer instruments with its own. The code calls the sanitizer runtime, so the host daslang must be built with it (-DDAS_USE_SANITIZER); an -exe or -lib links with -fsanitize= for the host's sanitizers and the requested ones, through clang++ unless --jit-path-to-linker names another driver. Script-level pin: options jit_sanitize = \"address\""
    sanitize : Option<string>

    @clarg_name = "jit-linker-string"
    @clarg_doc = "JIT: extra args inserted verbatim into the host linker cmd. POSIX/mingw drive a compiler driver (e.g. -fsanitize=undefined works); Windows-MSVC drives lld-link, so pass link.exe-style flags."
    linker_string : Option<string>

    @clarg_name = "jit-path-to-linker"
    @clarg_doc = "JIT: explicit linker binary (overrides default c++/clang/lld-link). Windows-MSVC builds a link.exe-flavored cmd (/DLL /OUT:) — pass a link.exe-compatible linker; elsewhere match the compiler daslang was built with to avoid sanitizer runtime mismatch."
    path_to_linker : Option<string>

    @clarg_name = "jit-lib"
    @clarg_doc = "JIT: emit a C-ABI native library plus its C header instead of running the program. Script-level pin: options jit_lib = true"
    lib : Option<bool>

    @clarg_name = "jit-lib-export-marked"
    @clarg_doc = "JIT (-lib): export every function the program already marks [export], not only the [export_c] ones. Selection only - unlike -lib-export-all it marks nothing itself, so a non-representable signature is still a hard error. Script-level pin: options jit_lib_export_marked = true"
    lib_export_marked : Option<bool>

    @clarg_name = "jit-lib-static"
    @clarg_doc = "JIT (-lib): produce a static archive (.a / .lib) instead of a shared library; the host then links libDaScript_runtime itself (the generated header records what). Under this flag --jit-path-to-linker names the ARCHIVER (llvm-ar / ar / lib), not a linker"
    lib_static : Option<bool>

    @clarg_name = "jit-lib-bindings"
    @clarg_doc = "JIT (-lib): also write daslang `[extern]` bindings for the emitted library to this path, so a daslang host requires them instead of hand-writing the declarations or parsing the C header"
    lib_bindings : Option<string>

    @clarg_name = "jit-register-all-modules"
    @clarg_doc = "JIT (-exe only): register all builtin native modules (math, fio, dasbind, ...) at exe startup, so a standalone compiler-driver exe can recompile arbitrary daslang at runtime"
    register_all_modules : Option<bool>
}
