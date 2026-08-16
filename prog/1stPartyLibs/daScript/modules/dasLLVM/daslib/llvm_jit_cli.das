options gen2
options indenting = 4

module llvm_jit_cli shared private

require daslib/clargs

//! Shared JIT command-line option schema.
//!
//! Declared here (rather than inside `llvm_macro`) so both the `jit_llvm`
//! simulate-macro and the `utils/jit` driver parse the exact same flags.

[CommandLineArgs]
struct public JitCliOptions {
    @clarg_name = "jit-opt-level"
    @clarg_doc = "JIT: LLVM optimization level (0..3); 0 disables all passes"
    opt_level : Option<int>

    @clarg_name = "jit-size-level"
    @clarg_doc = "JIT: LLVM size-optimization level (0..2)"
    size_level : Option<int>

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
    @clarg_doc = "JIT (-exe): emit a startup safe-check that compares every used handled-type's HOST-baked size + field offsets against the TARGET runtime annotation and aborts — reporting ALL divergences — on any mismatch. Diagnostic for the cross-compile C++ ABI layout class (wasm). Off by default."
    check_abi : Option<bool>

    @clarg_name = "jit-linker-string"
    @clarg_doc = "JIT: extra args inserted verbatim into the host linker cmd. POSIX/mingw drive a compiler driver (e.g. -fsanitize=undefined works); Windows-MSVC drives lld-link, so pass link.exe-style flags."
    linker_string : Option<string>

    @clarg_name = "jit-path-to-linker"
    @clarg_doc = "JIT: explicit linker binary (overrides default c++/clang/lld-link). Windows-MSVC builds a link.exe-flavored cmd (/DLL /OUT:) — pass a link.exe-compatible linker; elsewhere match the compiler daslang was built with to avoid sanitizer runtime mismatch."
    path_to_linker : Option<string>

    @clarg_name = "jit-register-all-modules"
    @clarg_doc = "JIT (-exe only): register all builtin native modules (math, fio, dasbind, ...) at exe startup, so a standalone compiler-driver exe can recompile arbitrary daslang at runtime"
    register_all_modules : Option<bool>
}
