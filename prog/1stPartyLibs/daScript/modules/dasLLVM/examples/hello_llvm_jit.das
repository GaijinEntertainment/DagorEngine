options gen2
require daslib/safe_addr
require daslib/math_bits

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_targets
require jit

[export]
def main {
    var g_prim_t = new PrimitiveTypes(LLVMGetGlobalContext())
    var mod = LLVMModuleCreateWithName("my_module")
    // NOTE: we are making this example compatible with upcomming JIT, i.e. function signature is that of a JIT function
    // Context * context, vec4f * args, void * CMRES
    var ret_type = LLVMFunctionType(g_prim_t.LLVMFloat4Type(), fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u), g_prim_t.LLVMVoidPtrType()))
    var sum = LLVMAddFunction(mod, "sum", ret_type)
    var entry = LLVMAppendBasicBlock(sum, "entry")
    var builder = LLVMCreateBuilder()
    LLVMPositionBuilderAtEnd(builder, entry)
    var args = LLVMGetParam(sum, 1u)
    var a1 = LLVMBuildGEP2(builder, g_prim_t.LLVMFloat4Type(), args, g_prim_t.ConstI32(0ul), "a1")
    var a2 = LLVMBuildGEP2(builder, g_prim_t.LLVMFloat4Type(), args, g_prim_t.ConstI32(1ul), "a2")
    var v1 = LLVMBuildLoad2(builder, g_prim_t.t_int32, a1, "v1")
    var v2 = LLVMBuildLoad2(builder, g_prim_t.t_int32, a2, "v2")
    var t1 = LLVMBuildAdd(builder, v1, v2, "t1")
    var tbit = LLVMBuildBitCast(builder, t1, g_prim_t.t_float, "tbit")
    var vres = LLVMGetUndef(g_prim_t.LLVMFloat4Type())
    var tb = LLVMBuildInsertElement(builder, vres, tbit, g_prim_t.ConstI32(0ul), "tb")
    LLVMBuildRet(builder, tb)
    LLVMVerifyModule(mod, LLVMVerifierFailureAction.LLVMAbortProcessAction)
    var engine : LLVMExecutionEngineRef
    LLVMLinkInMCJIT()
    LLVMInitializeAllTargetInfos()
    LLVMInitializeAllTargets()
    LLVMInitializeAllTargetMCs()
    LLVMInitializeAllAsmPrinters()
    LLVMCreateJITCompilerForModule(engine, mod)
    var pfun = LLVMGetFunctionAddress(engine, "sum")
    var values <- fixed_array<int4>(
        int4(11, 0, 0, 0),
        int4(12, 0, 0, 0)
    )
    var res : float4
    unsafe {
        res = invoke_code(reinterpret<void?>(pfun), safe_addr(values[0]), null)
    }
    print("result = {float_bits_to_int(res)}\n")
    LLVMDumpModule(mod)
    LLVMDisposeBuilder(builder)
    LLVMDisposeExecutionEngine(engine)
}
