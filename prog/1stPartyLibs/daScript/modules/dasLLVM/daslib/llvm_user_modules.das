options gen2
options indenting = 4
options _dasllama_internal = true
options no_global_variables = false

module llvm_user_modules shared private

require llvm/daslib/llvm_code_selftest
require llvm/daslib/llvm_tune_fat_word
require daslib/module_group
require [llvm_code_generator]

//! Wiring for `[llvm_code]` generator modules. The JIT calls
//! `register_user_llvm_code_generators` (lazily, in the context that reads the registry)
//! before the first generator lookup. A generator package joins the `llvm_code_generator`
//! group from its descriptor and defines `register_llvm_code_generators`. Registered
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
    llvm_tune_fat_word::register_fat_word()  // the fat dispatch mask as an LLVM-side word
    call_module_group("llvm_code_generator", "register_llvm_code_generators")
}
