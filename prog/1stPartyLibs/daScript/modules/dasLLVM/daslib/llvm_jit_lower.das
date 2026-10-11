options gen2
options indenting = 4

module llvm_jit_lower shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_jit_intrin
require daslib/ast_boost
require daslib/lpipe

def private jit_append_bb(ctx : JitCtx; name : string) {
    let ffunc = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx.builder))
    return LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, name)
}

def private jit_select(ctx : JitCtx; cond : LLVMOpaqueValue?; typ : LLVMOpaqueType?;
                       if_true, if_false : block<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    if (LLVMIsConstant(cond) != 0) {
        let val = LLVMConstIntGetZExtValue(cond)
        return val != 0ul ? invoke(if_true) : invoke(if_false)
    }
    var check_true = jit_append_bb(ctx, "check_true")
    var check_false = jit_append_bb(ctx, "check_false")
    var check_end = jit_append_bb(ctx, "check_end")
    var phi_true = jit_append_bb(ctx, "phi_true")
    var phi_false = jit_append_bb(ctx, "phi_false")
    LLVMBuildCondBr(ctx.builder, cond, check_true, check_false)
    LLVMPositionBuilderAtEnd(ctx.builder, check_true)
    var pt = invoke(if_true)
    LLVMBuildBr(ctx.builder, phi_true)
    LLVMPositionBuilderAtEnd(ctx.builder, phi_true)
    LLVMBuildBr(ctx.builder, check_end)
    LLVMPositionBuilderAtEnd(ctx.builder, check_false)
    var pf = invoke(if_false)
    LLVMBuildBr(ctx.builder, phi_false)
    LLVMPositionBuilderAtEnd(ctx.builder, phi_false)
    LLVMBuildBr(ctx.builder, check_end)
    LLVMPositionBuilderAtEnd(ctx.builder, check_end)
    var phi = LLVMBuildPhi(ctx.builder, typ, "select")
    LLVMAddIncoming(phi, fixed_array(pt, pf), fixed_array(phi_true, phi_false))
    return phi
}

def private lower_array_push_back(ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>;
                                  slow : block<() : LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    let arr_ptr = arguments[0]
    let arrTy = type_to_llvm_type(expr.arguments[0]._type)
    var hdr = LLVMBuildLoad2(ctx.builder, arrTy, arr_ptr, "push.hdr")
    var size = LLVMBuildExtractValue(ctx.builder, hdr, uint(JIT_ARRAY.SIZE), "push.size")
    var cap = LLVMBuildExtractValue(ctx.builder, hdr, uint(JIT_ARRAY.CAPACITY), "push.cap")
    var lock = LLVMBuildExtractValue(ctx.builder, hdr, uint(JIT_ARRAY.LOCK), "push.lock")
    var unlocked = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, lock,
        LLVMConstInt(LLVMTypeOf(lock), 0ul, 0), "push.unlocked")
    var fits = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntULT, size, cap, "push.fits")
    var fast = LLVMBuildAnd(ctx.builder, unlocked, fits, "push.fast")
    var res = jit_select(ctx, fast, ctx.types.t_int64) {
        var sizePtr = LLVMBuildStructGEP2(ctx.builder, arrTy, arr_ptr, uint(JIT_ARRAY.SIZE), "push.sizep")
        var next = LLVMBuildAdd(ctx.builder, size, LLVMConstInt(LLVMTypeOf(size), 1ul, 0), "push.next")
        LLVMBuildStore(ctx.builder, next, sizePtr)
        return size
    }
    lpipe() {
        return invoke(slow)
    }
    return res
}

def private gate_array_push_back(expr : ExprCallFunc?) : bool {
    if (length(expr.arguments) < 2) return false
    assume argT = expr.arguments[0]._type
    return argT != null && argT.isGoodArrayType
}

let private g_lower_lookup <- {
    "$::__builtin_array_push_back" => @@lower_array_push_back
}

let private g_lower_gate <- {
    "$::__builtin_array_push_back" => @@gate_array_push_back
}

def private lowering_name(expr : ExprCallFunc?) : string {
    if (expr.func == null) return ""
    return "{expr.func._module.name}::{expr.func.name}"
}

def public has_interop_lowering(expr : ExprCallFunc?) : bool {
    var result = false
    g_lower_gate |> get(lowering_name(expr)) $(pgate) {
        result = pgate |> invoke(expr)
    }
    return result
}

def public lookup_interop_lowering(var g_ctx : LLVMContextRef; var g_builder : LLVMOpaqueBuilder?;
                                   var types : PrimitiveTypes?; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>;
                                   slow : block<() : LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var result : LLVMOpaqueValue?
    g_lower_lookup |> get(lowering_name(expr)) $(pfun) {
        result = pfun |> invoke(JitCtx(ctx = g_ctx, types = types, builder = g_builder), expr, arguments) {
            return invoke(slow)
        }
    }
    return result
}
