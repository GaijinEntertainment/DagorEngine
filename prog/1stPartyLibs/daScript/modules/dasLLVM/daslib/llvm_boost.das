options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false

module llvm_boost shared

require llvm/bindings/llvm_func public
require llvm/bindings/llvm_struct public
require llvm/bindings/llvm_enum public

require daslib/safe_addr
require daslib/defer
require strings
require daslib/ast_boost

class PrimitiveTypes {
    private ctx : LLVMContextRef
    t_float : LLVMOpaqueType?
    t_double : LLVMOpaqueType?
    t_int1 : LLVMOpaqueType?
    t_int8 : LLVMOpaqueType?
    t_int16 : LLVMOpaqueType?
    t_int32 : LLVMOpaqueType?
    t_int64 : LLVMOpaqueType?
    t_int128 : LLVMOpaqueType?
    t_void : LLVMOpaqueType?

    def PrimitiveTypes(var ctx_ : LLVMContextRef) {
        ctx = ctx_
        t_float = LLVMFloatTypeInContext(ctx)
        t_double = LLVMDoubleTypeInContext(ctx)
        t_int1 = LLVMInt1TypeInContext(ctx)
        t_int8 = LLVMInt8TypeInContext(ctx)
        t_int16 = LLVMInt16TypeInContext(ctx)
        t_int32 = LLVMInt32TypeInContext(ctx)
        t_int64 = LLVMInt64TypeInContext(ctx)
        t_int128 = LLVMInt128TypeInContext(ctx)
        t_void = LLVMVoidTypeInContext(ctx)
    }

    def PointerType(typ : LLVMOpaqueType?) {
        return LLVMPointerType(typ, 0u)
    }

    def const StructType(var fields : LLVMOpaqueType ??; len : uint; packed : int = 0) {
        return LLVMStructTypeInContext(ctx, fields, len, packed)
    }

    def const LLVMFloat4Type {
        return LLVMVectorType(t_float, 4u)
    }

    def const LLVMFloat3Type {
        return LLVMVectorType(t_float, 3u)
    }

    def const LLVMFloat2Type {
        return LLVMVectorType(t_float, 2u)
    }

    def const LLVMInt4Type {
        return LLVMVectorType(t_int32, 4u)
    }

    def const LLVMInt3Type {
        return LLVMVectorType(t_int32, 3u)
    }

    def const LLVMInt2Type {
        return LLVMVectorType(t_int32, 2u)
    }

    def const LLVMRange64Type {
        return LLVMVectorType(t_int64, 2u)
    }

    def const get_type_string : LLVMOpaqueType ? {
        return LLVMPointerType(t_int8, 0u)
    }

    def LLVMIntPtrType : LLVMOpaqueType ? {
        static_if (typeinfo sizeof(type<void?>) == 8) {
            return t_int64
        } else {
            return t_int32
        }
    }

    def const LLVMVoidPtrType : LLVMOpaqueType ? {
        return LLVMPointerType(t_int8, 0u)
    }

    def const ConstI1(val : bool) {
        return LLVMConstInt(t_int1, uint64(val ? 1 : 0), 0);
    }

    def const ConstI8(val : int8) {
        return LLVMConstInt(t_int8, val |> uint64(), 0);
    }

    def const ConstI16(val : int16) {
        return LLVMConstInt(t_int16, val |> uint64, 0);
    }

    def const ConstI32(val : uint64) {
        return LLVMConstInt(t_int32, val, 0);
    }

    def const ConstI64(val : uint64) {
        return LLVMConstInt(t_int64, val, 0);
    }

    def const ConstPtr(val : uint64; tp : LLVMOpaqueType?) {
        return LLVMConstIntToPtr(LLVMConstInt(t_int64, val, 0), tp);
    }

    def const ConstReal(value : double) {
        return LLVMConstReal(t_float, value)
    }

    def const ConstDouble(value : double) {
        return LLVMConstReal(t_double, value)
    }



    private interop_fn_type : LLVMTypeRef

    def LLVMInteropFunctionType {
        if (interop_fn_type == null) {
            interop_fn_type = LLVMFunctionType(LLVMFloat4Type(),
                fixed_array<LLVMTypeRef>(
                    LLVMVoidPtrType(),
                    LLVMVoidPtrType(),
                    LLVMPointerType(LLVMFloat4Type(), 0u)
                ))
        }
        return interop_fn_type
    }


}

def public array_data_ptr(var arr : auto(T)[]) : T? {
    return unsafe(addr(arr[0]))
}

def public array_data_ptr(var arr : array<auto(T)>) : T? {
    if (arr |> empty()) {
        return null
    } else {
        return unsafe(addr(arr[0]))
    }
}

def StructType(types : PrimitiveTypes?; var fields : LLVMOpaqueType? []) {
    return types.StructType(array_data_ptr(fields), uint(length(fields)))
}

def StructType(types : PrimitiveTypes?; var fields : array<LLVMOpaqueType?>) {
    return types.StructType(array_data_ptr(fields), uint(length(fields)))
}


def LLVMBuildSRemInt32(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; lhs, rhs : LLVMOpaqueValue?; name : string) {
    // convert lhs % rhs to lhs - int(double(lhs)/double(rhs))*rhs
    let l = LLVMBuildSIToFP(builder, lhs, types.t_double, "")
    let r = LLVMBuildSIToFP(builder, rhs, types.t_double, "")
    let d = LLVMBuildFDiv(builder, l, r, "")
    let i = LLVMBuildFPToSI(builder, d, types.t_int32, "")
    let m = LLVMBuildMul(builder, i, rhs, "")
    return LLVMBuildSub(builder, lhs, m, name)
}


def LLVMBuildURemUInt32(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; lhs, rhs : LLVMOpaqueValue?; name : string) {
    // convert lhs % rhs to lhs - uint(double(lhs)/double(rhs))*rhs
    let l = LLVMBuildUIToFP(builder, lhs, types.t_double, "")
    let r = LLVMBuildUIToFP(builder, rhs, types.t_double, "")
    let d = LLVMBuildFDiv(builder, l, r, "")
    let i = LLVMBuildFPToUI(builder, d, types.t_int32, "")
    let m = LLVMBuildMul(builder, i, rhs, "")
    return LLVMBuildSub(builder, lhs, m, name)
}

def LLVMIsVector3(typ : LLVMOpaqueType?) : bool {
    return (LLVMGetTypeKind(typ) == LLVMTypeKind.LLVMVectorTypeKind) && (LLVMGetVectorSize(typ) == 3u)
}

def LLVMBuildLoadVector3(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; var typ : LLVMOpaqueType?; ptr : LLVMOpaqueValue?; name : string) {
    verify(LLVMGetVectorSize(typ) == 3u)
    let elemT = LLVMGetElementType(typ)
    var x = LLVMBuildLoad2(builder, elemT, ptr, "")
    var y = LLVMBuildLoad2(builder, elemT, LLVMBuildGEP2(builder, elemT, ptr, types.ConstI32(1ul), "", true), "")
    var z = LLVMBuildLoad2(builder, elemT, LLVMBuildGEP2(builder, elemT, ptr, types.ConstI32(2ul), "", true), "")
    var xyz = LLVMGetUndef(LLVMVectorType(elemT, 3u))
    xyz = LLVMBuildInsertElement(builder, xyz, x, types.ConstI32(0ul), "")
    xyz = LLVMBuildInsertElement(builder, xyz, y, types.ConstI32(1ul), "")
    xyz = LLVMBuildInsertElement(builder, xyz, z, types.ConstI32(2ul), "")
    return xyz
}

def LLVMBuildLoad2Aligned(builder : LLVMOpaqueBuilder?; typ : LLVMOpaqueType?; ptr : LLVMOpaqueValue?; alignment : uint; name : string) {
    let ld = LLVMBuildLoad2(builder, typ, ptr, name)
    LLVMSetAlignment(ld, alignment)
    return ld
}

def LLVMBuildStoreAligned(builder : LLVMOpaqueBuilder?; val, ptr : LLVMOpaqueValue?; alignment : uint) {
    let st = LLVMBuildStore(builder, val, ptr)
    LLVMSetAlignment(st, alignment)
    return st
}

def LLVMBuildStore2(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; typ : LLVMOpaqueType?; val : LLVMOpaqueValue?; ptr : LLVMOpaqueValue?) {
    let tptr = LLVMBuildPointerCast(builder, ptr, types.PointerType(typ), "")
    return LLVMBuildStore(builder, val, tptr)
}

def LLVMBuildStore2Aligned(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; typ : LLVMOpaqueType?; val : LLVMOpaqueValue?; ptr : LLVMOpaqueValue?; alignment : uint) {
    let tptr = LLVMBuildPointerCast(builder, ptr, types.PointerType(typ), "")
    let st = LLVMBuildStore(builder, val, tptr)
    LLVMSetAlignment(st, alignment)
    return st
}

def LLVMGetEnumAttributeKindForName(attr_name : string) {
    return LLVMGetEnumAttributeKindForName(attr_name, uint64(length(attr_name)))
}

def LLVMGetEnumAttribute(ctx : LLVMContextRef; attr_name : string) : LLVMOpaqueAttributeRef? {
    var attr_id = LLVMGetEnumAttributeKindForName(attr_name)
    if (attr_id == 0u) {
        panic("{attr_name} attribute not found")
    }
    var attr = LLVMCreateEnumAttribute(ctx, attr_id, 0ul)
    if (attr == null) {
        panic("{attr_name} attribute not found")
    }
    return attr
}

def LLVMAddAttributeToFunction(ffunc : LLVMOpaqueValue?; attr : LLVMOpaqueAttributeRef?) {
    LLVMAddAttributeAtIndex(ffunc, ~0u, attr)
}

def LLVMAddAttributesToFunction(ffunc : LLVMOpaqueValue?; attr : LLVMOpaqueAttributeRef? []) {
    for (a in attr) {
        LLVMAddAttributeAtIndex(ffunc, ~0u, a)
    }
}

def LLVMAddAttributeToFunctionResult(ffunc : LLVMOpaqueValue?; attr : LLVMOpaqueAttributeRef?) {
    LLVMAddAttributeAtIndex(ffunc, 0u, attr)
}

def LLVMAddAttributeToFunctionArgument(ffunc : LLVMOpaqueValue?; ai : uint; attr : LLVMOpaqueAttributeRef?) {
    LLVMAddAttributeAtIndex(ffunc, ai + 1u, attr)
}

def LLVMBuildShuffleVector(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; v1, v2 : LLVMOpaqueValue?; mask : array<int>; name : string) {
    var elem : array<LLVMOpaqueValue?>
    for (m in mask) {
        if (m != -1) {
            elem |> push(types.ConstI32(uint64(m)))
        } else {
            elem |> push(LLVMGetUndef(types.t_int32))
        }
    }
    return LLVMBuildShuffleVector(builder, v1, v2, LLVMConstVector(array_data_ptr(elem), uint(length(elem))), name)
}

def LLVMAddIncoming(phi : LLVMOpaqueValue?; var phi_vals : LLVMOpaqueValue? []; var phi_blocks : LLVMOpaqueBasicBlock? []) {
    static_if (typeinfo dim(phi_vals) != typeinfo dim(phi_blocks)) {
        concept_assert(false, "LLVMAddIncoming: phi_vals and phi_blocks must have the same length")
    }
    LLVMAddIncoming(phi, array_data_ptr(phi_vals), array_data_ptr(phi_blocks), uint(length(phi_vals)))
}

def LLVMAddIncoming(phi : LLVMOpaqueValue?; var phi_vals : array<LLVMOpaqueValue?>; var phi_blocks : array<LLVMOpaqueBasicBlock?>) {
    assert(length(phi_vals) == length(phi_blocks), "LLVMAddIncoming: phi_vals and phi_blocks must have the same length")
    LLVMAddIncoming(phi, array_data_ptr(phi_vals), array_data_ptr(phi_blocks), uint(length(phi_vals)))
}

def LLVMBuildGEP2(builder : LLVMOpaqueBuilder?; ty : LLVMOpaqueType?; ptr : LLVMOpaqueValue?; var index : LLVMOpaqueValue?; name : string; inbounds : bool = true) {
    var gep = LLVMBuildGEP2(builder, ty, ptr, unsafe(addr(index)), 1u, name)
    if (inbounds) {
        LLVMSetIsInBounds(gep, 1)
    }
    return gep
}

def LLVMFunctionType(ret_type : LLVMTypeRef; var param_types : LLVMTypeRef[]; isVariadic : bool = false) {
    return LLVMFunctionType(ret_type, array_data_ptr(param_types), uint(length(param_types)), isVariadic ? 1 : 0)
}

def LLVMFunctionType(ret_type : LLVMTypeRef; var param_types : array<LLVMTypeRef>; isVariadic : bool = false) {
    let nparam = length(param_types)
    if (nparam == 0) {
        return LLVMFunctionType(ret_type, null, 0u, isVariadic ? 1 : 0)
    } else {
        return LLVMFunctionType(ret_type, array_data_ptr(param_types), uint(nparam), isVariadic ? 1 : 0)
    }
}

def LLVMFunctionType(ret_type : LLVMTypeRef; isVariadic : bool = false) {
    return LLVMFunctionType(ret_type, null, 0u, isVariadic ? 1 : 0)
}

def LLVMVerifyModule(mod : LLVMOpaqueModule?; action : LLVMVerifierFailureAction; fail : bool = true) {
    var error : string
    if (LLVMVerifyModule(mod, action, safe_addr(error)) != 0) {
        if (fail) {
            let msg = clone_string(error)
            LLVMDisposeMessage(error)
            panic(msg)
        } else {
            LLVMDisposeMessage(error)
        }
        return false
    }
    LLVMDisposeMessage(error) // Here we have 1 byte message anyway
    return true
}

def LLVMVerifyFunction(fn : LLVMOpaqueValue?; action : LLVMVerifierFailureAction; fail : bool) {
    if (LLVMVerifyFunction(fn, action) != 0) {
        if (fail) {
            panic("Failed to verify")
        } else {
        }
        return false
    }
    return true
}

def LLVMCreateJITCompilerForModule(var engine : LLVMExecutionEngineRef&; mod : LLVMOpaqueModule?; optlevel : uint = 2u) {
    var error : string
    if (LLVMCreateJITCompilerForModule(unsafe(addr(engine)), mod, optlevel, safe_addr(error)) != 0) {
        let msg = clone_string(error)
        LLVMDisposeMessage(error)
        panic(msg)
    }
}

def LLVMCreateExecutionEngineForModule(var engine : LLVMExecutionEngineRef&; mod : LLVMOpaqueModule?) {
    var error : string
    if (LLVMCreateExecutionEngineForModule(unsafe(addr(engine)), mod, safe_addr(error)) != 0) {
        let msg = clone_string(error)
        LLVMDisposeMessage(error)
        panic(msg)
    }
}

def LLVMBuildAnyInt2_xy(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; int2type : LLVMOpaqueType?; x, y : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(int2type)
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    return LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), name)
}

def LLVMBuildInt2_xy(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMInt2Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    return LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), name)
}

def LLVMBuildInt3_xyz(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y, z : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMInt3Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    vres = LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), "")
    return LLVMBuildInsertElement(builder, vres, z, types.ConstI32(2ul), name)
}

def LLVMBuildInt4_xyzw(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y, z, w : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMInt4Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    vres = LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), "")
    vres = LLVMBuildInsertElement(builder, vres, z, types.ConstI32(2ul), "")
    return LLVMBuildInsertElement(builder, vres, w, types.ConstI32(3ul), name)
}

def LLVMBuildFloat2_xy(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMFloat2Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    return LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), name)
}

def LLVMBuildFloat3_xyz(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y, z : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMFloat3Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    vres = LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), "")
    return LLVMBuildInsertElement(builder, vres, z, types.ConstI32(2ul), name)
}

def LLVMBuildFloat4_xyzw(builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; x, y, z, w : LLVMOpaqueValue?; name : string) {
    var vres = LLVMGetUndef(types.LLVMFloat4Type())
    vres = LLVMBuildInsertElement(builder, vres, x, types.ConstI32(0ul), "")
    vres = LLVMBuildInsertElement(builder, vres, y, types.ConstI32(1ul), "")
    vres = LLVMBuildInsertElement(builder, vres, z, types.ConstI32(2ul), "")
    return LLVMBuildInsertElement(builder, vres, w, types.ConstI32(3ul), name)
}

def LLVMBuildCall(builder : LLVMOpaqueBuilder?; func : LLVMOpaqueValue?; var args : LLVMOpaqueValue? []; name : string) {
    return LLVMBuildCall(builder, func, array_data_ptr(args), uint(length(args)), name)
}

def LLVMBuildCall2(builder : LLVMOpaqueBuilder?; typ : LLVMOpaqueType?; func : LLVMOpaqueValue?; var args : LLVMOpaqueValue? []; name : string) {
    return LLVMBuildCall2(builder, typ, func, array_data_ptr(args), uint(length(args)), name)
}

def LLVMBuildCall2(builder : LLVMOpaqueBuilder?; typ : LLVMOpaqueType?; func : LLVMOpaqueValue?; name : string) {
    return LLVMBuildCall2(builder, typ, func, null, 0u, name)
}

def LLVMBuildCall2(builder : LLVMOpaqueBuilder?; typ : LLVMOpaqueType?; func : LLVMOpaqueValue?; var args : array<LLVMOpaqueValue?>; name : string) {
    let nparams = length(args)
    if (nparams != 0) {
        return LLVMBuildCall2(builder, typ, func, array_data_ptr(args), uint(nparams), name)
    } else {
        return LLVMBuildCall2(builder, typ, func, null, 0u, name)
    }
}

def LLVMLookupIntrinsicID(name : string) {
    return LLVMLookupIntrinsicID(name, uint64(length(name)))
}

def LLVMGetIntrinsicDeclaration(mod : LLVMOpaqueModule?; id : uint; var argTypes : array<LLVMOpaqueType?>) {
    return LLVMGetIntrinsicDeclaration(mod, id, array_data_ptr(argTypes), uint64(length(argTypes)))
}

def describe(typ : LLVMOpaqueType?) {
    return "null" if (typ == null)
    var st = LLVMPrintTypeToString(typ)
    var stc = clone_string(st)
    LLVMDisposeMessage(st)
    return stc
}

def describe(typ : LLVMOpaqueValue?) {
    return "null" if (typ == null)
    var st = LLVMPrintValueToString(typ)
    var stc = clone_string(st)
    LLVMDisposeMessage(st)
    return stc
}

def describe(blk : LLVMOpaqueBasicBlock?) {
    return "null" if (blk == null)
    var st := LLVMGetBasicBlockName(blk)
    return st
}

def isExprOp2_Func(expr : smart_ptr<ExprOp2>) {
    if (!expr.func.flags.builtIn) {
        return true
    }
    if (expr.left._type.isHandle || expr.right._type.isHandle) {
        return true
    }
    return false
}

def isExprOp1_Func(expr : smart_ptr<ExprOp1>) {
    if (!expr.func.flags.builtIn) {
        return true
    }
    if (expr.subexpr._type.isHandle) {
        return true
    }
    return false
}

def LLVMSetFunctionCallConv(ofunc : LLVMOpaqueValue?; conv : LLVMCallConv) {
    LLVMSetFunctionCallConv(ofunc, uint(conv))
}

def LLVMSetInstructionCallConv(ccall : LLVMOpaqueValue?; conv : LLVMCallConv) {
    LLVMSetInstructionCallConv(ccall, uint(conv))
}

def set_meta_dereferenceable(g_ctx : LLVMContextRef; var types : PrimitiveTypes?; instr : LLVMOpaqueValue?; bytes : int; nil : bool) {
    var values : LLVMOpaqueValue? [1]
    values[0] = LLVMConstInt(types.t_int64, uint64(bytes), 0)
    let msg = nil ? "dereferenceable_or_null" : "dereferenceable"
    LLVMSetMetadata(
        instr,
        LLVMGetMDKindIDInContext(g_ctx, msg, uint(length(msg))),
        LLVMMDNodeInContext(g_ctx, array_data_ptr(values), 1u))
}

def LLVMAddAttributeToFunctionArgumentRange(func : LLVMOpaqueValue?; rng : urange; attr : LLVMOpaqueAttributeRef?) {
    for (t in rng) {
        LLVMAddAttributeToFunctionArgument(func, t, attr)
    }
}
