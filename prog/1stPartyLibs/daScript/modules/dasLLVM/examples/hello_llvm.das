options gen2
require daslib/safe_addr
require llvm/daslib/llvm_boost
require llvm/daslib/llvm_targets

[export]
def main {
    var ctx = LLVMGetGlobalContext()
    var types = new PrimitiveTypes(ctx)
    var mod = LLVMModuleCreateWithNameInContext("my_module", ctx)
    var ret_type = LLVMFunctionType(types.t_int32, fixed_array<LLVMTypeRef>(types.t_int32))
    var sum = LLVMAddFunction(mod, "add", ret_type)
    var entry = LLVMAppendBasicBlock(sum, "entry")
    var builder = LLVMCreateBuilderInContext(ctx)
    LLVMPositionBuilderAtEnd(builder, entry)
    var tmp = LLVMBuildAdd(builder, LLVMGetParam(sum, 0u), LLVMConstInt(types.t_int32, 10 |> uint64(), 0), "tmp")
    LLVMBuildRet(builder, tmp)
    LLVMVerifyModule(mod, LLVMVerifierFailureAction.LLVMAbortProcessAction)
    var engine : LLVMExecutionEngineRef
    var _error : string
    LLVMInitializeAllTargets()
    LLVMInitializeAllTargetMCs()
    LLVMCreateExecutionEngineForModule(engine, mod)
    let x = 10ul
    var arg1 = LLVMCreateGenericValueOfInt(types.t_int32, x, 0)
    var args <- fixed_array<LLVMGenericValueRef>(
        arg1,
    )
    var llvm_res = LLVMRunFunction(engine, sum, 1u, safe_addr(args[0]))
    let res = (llvm_res
            |> LLVMGenericValueToInt(0)
            |> int())
    assert(res == 20)
    print("result = {res}\n")

    // Free everything
    LLVMDisposeGenericValue(arg1)
    LLVMDisposeGenericValue(llvm_res)
    LLVMDumpModule(mod)
    LLVMDisposeBuilder(builder)
    LLVMDisposeExecutionEngine(engine)
}
