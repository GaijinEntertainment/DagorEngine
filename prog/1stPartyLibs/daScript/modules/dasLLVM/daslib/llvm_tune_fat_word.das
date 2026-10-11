options gen2
options indenting = 4

module llvm_tune_fat_word shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_code
require daslib/ast_boost

//! The fat dispatch words as LLVM-side globals. `[llvm_code(name = "llvm_tune_fat_word::load",
//! global = "<name>")]` on a `() : int` stub emits a load of the module global `<name>`; the
//! `store` twin on an `(int) : void` stub emits the store. The global is one word of process
//! memory per artifact, so every context of the process reads what the exe's `[init]` wrote -
//! the pooled forks whose init script never ran included, where a das global stays zero. The
//! stubs' das bodies serve the interpreter and AOT; their `[llvm_code]` spelling is
//! `llvm_code_shell`'s.

def private word_global(var gc : LlvmCodeCtx) : LLVMOpaqueValue? {
    let gv = find_arg(gc.ann.arguments, "global")
    return null if (!(gv is tString) || empty(gv as tString))
    let name = gv as tString
    let mod = LLVMGetGlobalParent(gc.impl)
    var g = LLVMGetNamedGlobal(mod, name)
    if (g == null) {
        g = LLVMAddGlobal(mod, gc.jit.types.t_int32, name)
        LLVMSetInitializer(g, gc.jit.types->ConstI32(0ul))
        LLVMSetLinkage(g, LLVMLinkage.LLVMInternalLinkage)
        LLVMSetAlignment(g, 4u)
    }
    return g
}

def private load_gen(var gc : LlvmCodeCtx) : bool {
    if (!empty(gc.fn.arguments) || gc.fn.result.baseType != Type.tInt) return false
    let g = word_global(gc)
    return false if (g == null)
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    LLVMBuildRet(gc.jit.builder, LLVMBuildLoad2(gc.jit.builder, gc.jit.types.t_int32, g, "fat.word"))
    return true
}

def private store_gen(var gc : LlvmCodeCtx) : bool {
    if (length(gc.fn.arguments) != 1 || gc.fn.arguments[0]._type.baseType != Type.tInt
        || gc.fn.result.baseType != Type.tVoid) return false
    let g = word_global(gc)
    return false if (g == null)
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    LLVMBuildStore(gc.jit.builder, LLVMGetParam(gc.impl, 0u), g)
    LLVMBuildRetVoid(gc.jit.builder)
    return true
}

// Called from llvm_user_modules::register_user_llvm_code_generators, in the context that reads the registry.
[macro_function]
def public register_fat_word {
    register_llvm_code_generator("llvm_tune_fat_word::load", @@load_gen)
    register_llvm_code_generator("llvm_tune_fat_word::store", @@store_gen)
}
