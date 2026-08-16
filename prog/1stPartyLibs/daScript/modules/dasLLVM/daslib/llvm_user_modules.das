options gen2
options indenting = 4

module llvm_user_modules shared private

require llvm/daslib/llvm_code_selftest
require ?dasllama/dasllama_gemm_gen dasllama/dasllama_gemm_register  // nolint:STYLE030 — used inside the static_if below; on builds without the package both the require and the branch drop (path guard: pure-das package, nothing C++ to guard on)

//! User-extensible wiring for `[llvm_code]` generator modules. The JIT calls
//! `register_user_llvm_code_generators` (lazily, in the context that reads the registry)
//! before the first generator lookup. To plug in a generator module: add a
//! `require ?<impl-path> <registration>` line above — a PATH guard loads exactly when the
//! contributor's package is mounted (the guard's own file resolves), so this
//! module never hard-depends on the contributor. Pair each line with a `static_if
//! (typeinfo builtin_module_exists(<impl>))` around the registration call below; registered
//! generators are name-keyed and inert for programs whose `[llvm_code]` annotations never
//! name them.

var private g_registered = false

[macro_function]
def public register_user_llvm_code_generators {
    if (g_registered) {
        return
    }
    g_registered = true
    llvm_code_selftest::register_selftest()  // worked example + self-test generator (add_i32, add_plus_k)
    static_if (typeinfo builtin_module_exists(dasllama_gemm_gen)) {
        dasllama_gemm_register::register_dasllama_gemm_code_generators()
    }
}
