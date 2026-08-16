options gen2
options indenting = 4

module llvm_code_selftest shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_code
require llvm/daslib/llvm_jit_common
require daslib/ast_boost

//! Worked example of an `[llvm_code]` generator, and the self-test vehicle for the rail
//! (tests/llvm_code/): emits `a + b` for an `(int, int) : int` stub, declines any other
//! signature so the reference body compiles instead.

def private is_int_pair_to_int(fn : Function const?) : bool {
    if ((length(fn.arguments) != 2)
        || (fn.arguments[0]._type.baseType != Type.tInt)
        || (fn.arguments[1]._type.baseType != Type.tInt)) return false
    return fn.result.baseType == Type.tInt
}

def private add_i32_gen(var gc : LlvmCodeCtx) : bool {
    if (!is_int_pair_to_int(gc.fn)) return false // decline: the reference body compiles as usual
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    let sum = LLVMBuildAdd(gc.jit.builder, LLVMGetParam(gc.impl, 0u), LLVMGetParam(gc.impl, 1u), "llvm_code.sum")
    LLVMBuildRet(gc.jit.builder, sum)
    return true
}

// perm-parameterized worked example (the [tune] framework's self-test vehicle): emits
// `a + b + k` from the annotation arg k, so distinct perms are observable at runtime;
// decline=true declines by request — the perm-level rejection rail.
def private add_plus_k_gen(var gc : LlvmCodeCtx) : bool {
    if (!is_int_pair_to_int(gc.fn)) return false
    let dv = find_arg(gc.ann.arguments, "decline")
    if (dv is tBool && dv as tBool) return false
    var k = 0
    let kv = find_arg(gc.ann.arguments, "k")
    if (kv is tInt) { k = kv as tInt }
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    var sum = LLVMBuildAdd(gc.jit.builder, LLVMGetParam(gc.impl, 0u), LLVMGetParam(gc.impl, 1u), "llvm_code.sum")
    sum = LLVMBuildAdd(gc.jit.builder, sum, gc.jit.types->ConstI32(uint64(k)), "llvm_code.sumk")
    LLVMBuildRet(gc.jit.builder, sum)
    return true
}

// companion worked example (the [tune_companion] self-test vehicle): emits `return k` for a
// `() : int` stub — the same perm arg the main generator gets, observable on the sibling.
// Declines in lockstep with add_plus_k_gen (the same decline=true rail), which is the
// two-function-stamp coherence contract.
def private k_value_gen(var gc : LlvmCodeCtx) : bool {
    if (!empty(gc.fn.arguments) || gc.fn.result.baseType != Type.tInt) return false
    let dv = find_arg(gc.ann.arguments, "decline")
    if (dv is tBool && dv as tBool) return false
    var k = 0
    let kv = find_arg(gc.ann.arguments, "k")
    if (kv is tInt) { k = kv as tInt }
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    LLVMBuildRet(gc.jit.builder, gc.jit.types->ConstI32(uint64(k)))
    return true
}

// bakes the resolved host_features truth (g_target_host_features) into a `() : int` stub as
// a constant 1/0 — the artifact-side witness of which feature rail built it. The reference
// body should return -1 so all three outcomes (host / generic / not-generated) are distinct.
def private host_features_gen(var gc : LlvmCodeCtx) : bool {
    if (!empty(gc.fn.arguments) || gc.fn.result.baseType != Type.tInt) return false
    var entry = LLVMAppendBasicBlockInContext(gc.jit.ctx, gc.impl, "entry")
    LLVMPositionBuilderAtEnd(gc.jit.builder, entry)
    LLVMBuildRet(gc.jit.builder, gc.jit.types->ConstI32(g_target_host_features ? 1ul : 0ul))
    return true
}

// Called from llvm_user_modules::register_user_llvm_code_generators — the JIT invokes that
// in the context that reads the registry, so the direct call lands in the right table copy.
[macro_function]
def public register_selftest {
    register_llvm_code_generator("llvm_code_selftest::add_i32", @@add_i32_gen)
    register_llvm_code_generator("llvm_code_selftest::add_plus_k", @@add_plus_k_gen)
    register_llvm_code_generator("llvm_code_selftest::k_value", @@k_value_gen)
    register_llvm_code_generator("llvm_code_selftest::host_features", @@host_features_gen)
}
