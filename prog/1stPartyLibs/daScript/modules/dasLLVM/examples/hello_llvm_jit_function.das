options gen2
require daslib/safe_addr

require llvm/daslib/llvm_boost
require jit
require daslib/rtti
require llvm/daslib/llvm_targets

// If for some reason you want to set jit function manually you should add [no_jit] attribute.
// Otherwise it will be jitted by default jit-compiler.
[sideeffects, no_jit, unused_argument(a, b)]
def add_two_ints(a, b : int) : int {
    panic("not implemented")
    return 0
}

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
    unsafe {
        instrument_jit(reinterpret<void?>(pfun), @@add_two_ints, LineInfo(), this_context())
    }
    let res = add_two_ints(11, 12)
    print("result = {res}\n")
    LLVMDumpModule(mod)
    LLVMDisposeBuilder(builder)
    LLVMDisposeExecutionEngine(engine)
}
