options gen2

options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const
options no_global_variables = false

module ir_dsl shared public

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dsl
require llvm/daslib/llvm_jit_common

//! A small DSL for writing an LLVM function body: Ir values and their operators build at the
//! insertion point of the function ir_function is filling, so a body reads like the C++ it mirrors.

struct Ir {
    v : LLVMOpaqueValue?
}

struct private IrHelper {
    fn : LLVMOpaqueValue?
    b : LLVMBuilderRef
    ctx : LLVMOpaqueValue?
    at : LLVMOpaqueValue?
}

var private g_ir : IrHelper

def ir(v : LLVMOpaqueValue?) : Ir => Ir(v = v)
def ir(v : uint) : Ir => Ir(v = g_prim_t.ConstI32(uint64(v)))
def arg(i : int) : Ir => ir(LLVMGetParam(g_ir.fn, uint(i)))
def text(s : string) : Ir => ir(build_string_constant(s))
def operator +(a, b : Ir) : Ir => ir(LLVMBuildAdd(g_ir.b, a.v, b.v, ""))
def operator -(a, b : Ir) : Ir => ir(LLVMBuildSub(g_ir.b, a.v, b.v, ""))
def operator -(a : Ir) : Ir => ir(LLVMBuildNeg(g_ir.b, a.v, ""))
def operator &(a, b : Ir) : Ir => ir(LLVMBuildAnd(g_ir.b, a.v, b.v, ""))
def operator ==(a, b : Ir) : Ir => ir(LLVMBuildICmp(g_ir.b, LLVMIntPredicate.LLVMIntEQ, a.v, b.v, ""))
def operator !=(a, b : Ir) : Ir => ir(LLVMBuildICmp(g_ir.b, LLVMIntPredicate.LLVMIntNE, a.v, b.v, ""))
def below(a, b : Ir) : Ir => ir(LLVMBuildICmp(g_ir.b, LLVMIntPredicate.LLVMIntULT, a.v, b.v, ""))
def zext64(a : Ir) : Ir => ir(LLVMBuildZExt(g_ir.b, a.v, g_prim_t.t_int64, ""))
def is_null(p : Ir) : Ir => ir(LLVMBuildIsNull(g_ir.b, p.v, ""))
def select(c, a, b : Ir) : Ir => ir(LLVMBuildSelect(g_ir.b, c.v, a.v, b.v, ""))
def offset(p : Ir; var bytes : Ir) : Ir => ir(LLVMBuildGEP2(g_ir.b, g_prim_t.t_int8, p.v, bytes.v, "", false))
def offset(p : Ir; bytes : uint) : Ir => offset(p, ir(bytes))
//! the pointer-sized slot `i` slots away from `p`, stepped by the target's pointer size
def ptr_slot(p : Ir; i : int) : Ir => ir(LLVMBuildGEP2(g_ir.b, g_prim_t.LLVMVoidPtrType(), p.v, LLVMConstInt(g_prim_t.t_int32, uint64(int64(i)), 1), "", false))
def field(p : Ir; st : LLVMOpaqueType?; index : int) : Ir => ir(LLVMBuildStructGEP2(g_ir.b, st, p.v, uint(index), ""))
def load(p : Ir; t : LLVMOpaqueType?) : Ir => ir(LLVMBuildLoad2(g_ir.b, t, p.v, ""))
def load32(p : Ir) : Ir => load(p, g_prim_t.t_int32)
def load_ptr(p : Ir) : Ir => load(p, g_prim_t.LLVMVoidPtrType())
def ir_builder : LLVMBuilderRef => g_ir.b

def ret(v : Ir) {
    LLVMBuildRet(g_ir.b, v.v)
}

def store(p, v : Ir) {
    LLVMBuildStore(g_ir.b, v.v, p.v)
}

//! branches to a fresh block when `cond` holds; the builder continues in the returned fall-through
def private split_on(cond : Ir) : LLVMOpaqueBasicBlock? {
    let taken = LLVMAppendBasicBlockInContext(g_ctx, g_ir.fn, "")
    let rest = LLVMAppendBasicBlockInContext(g_ctx, g_ir.fn, "")
    LLVMBuildCondBr(g_ir.b, cond.v, taken, rest)
    LLVMPositionBuilderAtEnd(g_ir.b, taken)
    return rest
}

def throw_if(cond, message : Ir) {
    let rest = split_on(cond)
    LLVMBuildCall2(g_ir.b, g_fn_types[FN_JIT_EXCEPTION], LLVMGetNamedFunction(g_mod, FN_JIT_EXCEPTION), fixed_array(message.v, g_ir.ctx, g_ir.at), "")
    LLVMBuildUnreachable(g_ir.b)
    LLVMPositionBuilderAtEnd(g_ir.b, rest)
}

def return_if(cond : Ir) {
    let rest = split_on(cond)
    LLVMBuildRetVoid(g_ir.b)
    LLVMPositionBuilderAtEnd(g_ir.b, rest)
}

//! defines internal function `name` into g_mod and returns it; `ctx` and `at` are the parameter indices throw_if passes on, and a body that falls off the end returns void
def ir_function(name : string; typ : LLVMTypeRef; ctx, at : int; body : block<void>) : LLVMOpaqueValue? {
    g_ir.fn = LLVMAddFunctionWithType(g_mod, name, typ)
    LLVMSetLinkage(g_ir.fn, LLVMLinkage.LLVMInternalLinkage)
    g_ir.b = LLVMCreateBuilderInContext(g_ctx)
    LLVMPositionBuilderAtEnd(g_ir.b, LLVMAppendBasicBlockInContext(g_ctx, g_ir.fn, "entry"))
    g_ir.ctx = ctx >= 0 ? LLVMGetParam(g_ir.fn, uint(ctx)) : null
    g_ir.at = at >= 0 ? LLVMGetParam(g_ir.fn, uint(at)) : null
    invoke(body)
    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(g_ir.b)) == null) {
        LLVMBuildRetVoid(g_ir.b)
    }
    LLVMDisposeBuilder(g_ir.b)
    let fn = g_ir.fn
    g_ir = IrHelper()
    return fn
}
