options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const
options unsafe_table_lookup = false

module llvm_jit_intrin shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_common
require daslib/ast_boost
require strings

struct JitCtx {
    ctx : LLVMContextRef
    types : PrimitiveTypes -const? -const
    builder : LLVMOpaqueBuilder -const? -const
}

let g_intrin_lookup <- {
// $, aka builtin
    // misc
        "$::length" => @@intrinsic_builtin_length,
    // pointer math
        "$::i_das_ptr_set_add" => @@intrinsic_das_ptr_set_add,
        "$::i_das_ptr_add" => @@intrinsic_das_ptr_add,
        "$::i_das_ptr_set_sub" => @@intrinsic_das_ptr_set_sub,
        "$::i_das_ptr_sub" => @@intrinsic_das_ptr_sub,
    // memset
        "$::memset8" => @@intrinsic_memset8,
        "$::memset16" => @@intrinsic_memset16,
        "$::memset32" => @@intrinsic_memset32,
        "$::memset64" => @@intrinsic_memset64,
        "$::memset128" => @@intrinsic_memset128,
    // type c-tors
    // vector type c-tors
        "$::range" => @@intrinsic_builtin_range,
        "$::urange" => @@intrinsic_builtin_range,
        "$::range64" => @@intrinsic_builtin_range,
        "$::urange64" => @@intrinsic_builtin_range,
        "$::interval" => @@intrinsic_builtin_range,
        "$::float2" => @@intrinsic_builtin_float_vec,
        "$::float3" => @@intrinsic_builtin_float_vec,
        "$::float4" => @@intrinsic_builtin_float_vec,
        "$::int2" => @@intrinsic_builtin_int_vec,
        "$::int3" => @@intrinsic_builtin_int_vec,
        "$::int4" => @@intrinsic_builtin_int_vec,
        "$::uint2" => @@intrinsic_builtin_int_vec,
        "$::uint3" => @@intrinsic_builtin_int_vec,
        "$::uint4" => @@intrinsic_builtin_int_vec,
    // basic type c-tor
            "$::float" => @@intrinsic_builtin_float,
            "$::double" => @@intrinsic_builtin_float,
            "$::int8" => @@intrinsic_builtin_int,
            "$::uint8" => @@intrinsic_builtin_int,
            "$::int16" => @@intrinsic_builtin_int,
            "$::uint16" => @@intrinsic_builtin_int,
            "$::int" => @@intrinsic_builtin_int,
            "$::uint" => @@intrinsic_builtin_int,
            "$::int64" => @@intrinsic_builtin_int,
            "$::uint64" => @@intrinsic_builtin_int,
            "$::bitfield" => @@intrinsic_builtin_int,
            "$::bitfield8" => @@intrinsic_builtin_int,
            "$::bitfield16" => @@intrinsic_builtin_int,
            "$::bitfield64" => @@intrinsic_builtin_int,
    // bits
        "$::clz" => @@intrinsic_bit_nzp_op1,
        "$::ctz" => @@intrinsic_bit_nzp_op1,
        "$::popcnt" => @@intrinsic_bit_op1,
    // mul
        "$::mul128" => @@intrinsic_mul_128,
    // variant
        "$::set_variant_index" => @@intrinsic_builtin_set_variant_index,
        "$::variant_index" => @@intrinsic_builtin_variant_index,
// math - scatter gather
    "math::gather" => @@intrinsic_math_gather,
    "math::store_neq_mask" => @@intrinsic_math_store_neq_mask,
    "math::gather_store_neq_mask" => @@intrinsic_math_gather_store_mask,
    "math::gather_store_stride" => @@intrinsic_math_gather_store_stride,
    "math::u8x4_gather_store" => @@intrinsic_math_u8x4_gather_store,
// math
    "math::sign" => @@intrinsic_math_sign,
    "math::saturate" => @@intrinsic_math_saturate,
    "math::abs" => @@intrinsic_math_abs,
    "math::mad" => @@intrinsic_math_mad_op3,
    "math::lerp" => @@intrinsic_math_lerp_op3,
    "math::rcp" => @@intrinsic_math_rcp,
    "math::rcp_est" => @@intrinsic_math_rcp,
    "math::exp" => @@intrinsic_math_float_op1,
    "math::log" => @@intrinsic_math_float_op1,
    "math::exp2" => @@intrinsic_math_float_op1,
    "math::log2" => @@intrinsic_math_float_op1,
    "math::sqrt" => @@intrinsic_math_float_op1,
    "math::rsqrt" => @@intrinsic_math_rsqrt,
    "math::rsqrt_est" => @@intrinsic_math_rsqrt,
    "math::tan" => @@intrinsic_math_tan,
    "math::sin" => @@intrinsic_math_float_op1,
    "math::cos" => @@intrinsic_math_float_op1,
    "math::pow" => @@intrinsic_op2,
    "math::min" => @@intrinsic_math_minmax,
    "math::max" => @@intrinsic_math_minmax,
    "math::clamp" => @@intrinsic_math_clamp,
    "math::round" => @@intrinsic_math_float_op1,
    "math::floor" => @@intrinsic_math_float_op1,
    "math::ceil" => @@intrinsic_math_float_op1,
    "math::floori" => @@intrinsic_math_float_op1_to_int,
    "math::ceili" => @@intrinsic_math_float_op1_to_int,
    "math::roundi" => @@intrinsic_math_roundi,
    "math::trunci" => @@intrinsic_math_trunci,
    "math::length_sq" => @@intrinsic_math_length_sq,
    "math::dot" => @@intrinsic_math_dot,
    "math::cross" => @@intrinsic_math_cross,
    "math::inv_distance_sq" => @@intrinsic_math_inv_distance_sq,
    "math::inv_length" => @@intrinsic_math_inv_length,
    "math::fast_normalize" => @@intrinsic_math_fast_normalize,
    "math::normalize" => @@intrinsic_math_normalize
}

def public has_intrinsic(expr : smart_ptr<ExprCallFunc>) {
    var result = false
    assume argType = expr.arguments[0]._type
    let call_name = "{expr.func._module.name}::{expr.func.name}"
    if (call_name == "$::length" && !argType.isGoodArrayType) {
        return false
    }
    g_intrin_lookup |> get(call_name) <| $(pfun) {
        result = true
    }
    return result
}

def public lookup_intinsic(g_ctx : LLVMContextRef; g_builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var result :  LLVMOpaqueValue?
    let call_name = "{expr.func._module.name}::{expr.func.name}"
    g_intrin_lookup |> get(call_name) <| $(pfun) {
        result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
    }
    return result
}

// intrinsics, $ aka builtin

def intrinsic_builtin_variant_index(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, arguments[0], "$::variant_index")
}

def intrinsic_builtin_set_variant_index(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return LLVMBuildStore2(ctx.builder, ctx.types, ctx.types.t_int32, arguments[1], arguments[0])
}

def intrinsic_builtin_length(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.isGoodArrayType) {
        var arr = LLVMBuildLoad2(ctx.builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "arr")
        return LLVMBuildExtractValue(ctx.builder, arr, uint(JIT_ARRAY.SIZE), "array.size")
    } else {
        return null
    }
}

def intrinsic_builtin_int(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    let intType = base_type_to_llvm_type(expr._type.baseType)
    if (length(arguments) == 0) {
        return LLVMConstInt(intType, 0ul, 0)
    }
    assume argType = expr.arguments[0]._type
    if (argType.baseType == expr._type.baseType) {
        return arguments[0] // ctor of the same type is the same value, i.e int(a:int) is a
    } elif (argType.baseType == Type.tFloat || argType.baseType == Type.tDouble) {
        if (expr._type.isSignedInteger) {
            return LLVMBuildFPToSI(ctx.builder, arguments[0], intType, string(expr.name))
        } else {
            return LLVMBuildFPToUI(ctx.builder, arguments[0], intType, string(expr.name))
        }
    } elif (argType.isInteger || argType.isEnum) {
        if (argType.sizeOf >= expr._type.sizeOf) {
            return LLVMBuildTruncOrBitCast(ctx.builder, arguments[0], intType, string(expr.name))
        } elif (argType.isSignedInteger || argType.isEnum) {
            return LLVMBuildSExtOrBitCast(ctx.builder, arguments[0], intType, string(expr.name))
        } else {
            return LLVMBuildZExtOrBitCast(ctx.builder, arguments[0], intType, string(expr.name))
        }
    } else {
        return null
    }
}

def any2int(var ctx : JitCtx; val : LLVMOpaqueValue?; argType : Type; _outType : LLVMOpaqueType?; inbytes, outbytes : int; outSigned : bool) : LLVMOpaqueValue? {
    var outType = _outType != null ? _outType : ctx.types.t_int32
    if (argType == Type.tFloat || argType == Type.tDouble) {
        if (outSigned) {
            return LLVMBuildFPToSI(ctx.builder, val, outType, "")
        } else {
            return LLVMBuildFPToUI(ctx.builder, val, outType, "")
        }
    } else {
        return val if (inbytes == outbytes)
        let baseSInt = argType == Type.tInt8 || argType == Type.tInt16 || argType == Type.tInt || argType == Type.tInt64
        let baseUInt = argType == Type.tUInt8 || argType == Type.tUInt16 || argType == Type.tUInt || argType == Type.tUInt64
        if (baseSInt || baseUInt) {
            if (inbytes >= outbytes) {
                return LLVMBuildTruncOrBitCast(ctx.builder, val, outType, "")
            } elif (baseSInt) {
                return LLVMBuildSExtOrBitCast(ctx.builder, val, outType, "")
            } else {
                return LLVMBuildZExtOrBitCast(ctx.builder, val, outType, "")
            }
        } else {
            panic("unsupported any2int base type {argType}")
            return null
        }
    }
}

def intrinsic_builtin_int_vec(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume resType = expr._type
    assume argType = expr.arguments[0]._type
    if (length(arguments) == 0) {
        return build_broadcast_vector(ctx.builder, resType, ctx.types.ConstI32(0ul))
    }
    let outSigned = resType.vectorBaseType == Type.tInt
    // this works around the bug of fp2ui not working correctly for vectors on some values
    var isFloatToUintVec = argType.isVectorType && (argType.vectorBaseType == Type.tFloat) && !outSigned
    if (length(arguments) == 1 && !isFloatToUintVec) {
        if (argType.isVectorType) {
            return any2int(ctx, arguments[0], argType.vectorBaseType, type_to_llvm_type(expr._type), 4, 4, outSigned)
        } else {
            var val = any2int(ctx, arguments[0], argType.baseType, null, argType.sizeOf, 4, outSigned)
            return build_broadcast_vector(ctx.builder, resType, val)
        }
    }
    var vres = LLVMGetUndef(LLVMVectorType(ctx.types.t_int32, uint(expr._type.vectorDim)))
    var elemIndex = 0
    for (earg, arg in expr.arguments, arguments) {
        if (earg._type.isVectorType) {
            for (ei in range(earg._type.vectorDim)) {
                var eval = LLVMBuildExtractElement(ctx.builder, arg, ctx.types.ConstI32(uint64(ei)), "")
                var val = any2int(ctx, eval, earg._type.vectorBaseType, null, 4, 4, outSigned)
                vres = LLVMBuildInsertElement(ctx.builder, vres, val, ctx.types.ConstI32(uint64(elemIndex++)), "")
            }
        } else {
            var val = any2int(ctx, arg, earg._type.baseType, null, earg._type.sizeOf, 4, outSigned)
            vres = LLVMBuildInsertElement(ctx.builder, vres, val, ctx.types.ConstI32(uint64(elemIndex++)), "")
        }
    }
    if (elemIndex != resType.vectorDim) {
        failed_E(expr, "vector ctor {expr.name}({describe(expr._type)}) has {elemIndex} elements, but {resType.vectorDim} are expected")
    }
    return vres
}

def any2float(var ctx : JitCtx; val : LLVMOpaqueValue?; argType : Type; _outType : LLVMOpaqueType? = null) {
    let outType = _outType != null ? _outType : ctx.types.t_float
    return val                                              if (argType == Type.tFloat)
    return LLVMBuildFPTrunc(ctx.builder, val, outType, "") if (argType == Type.tDouble)
    return LLVMBuildSIToFP(ctx.builder, val, outType, "") if (argType == Type.tInt8 || argType == Type.tInt16 || argType == Type.tInt || argType == Type.tInt64)
    return LLVMBuildUIToFP(ctx.builder, val, outType, "") if (argType == Type.tUInt8 || argType == Type.tUInt16 || argType == Type.tUInt || argType == Type.tUInt64)
    panic("unsupported any2float base type {argType}")
    return null
}

def intrinsic_builtin_float_vec(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume resType = expr._type
    if (length(arguments) == 0) {
        return build_broadcast_vector(ctx.builder, resType, LLVMConstReal(ctx.types.t_float, 0.0lf))
    }
    if (length(arguments) == 1) {
        if (expr.arguments[0]._type.isVectorType) {
            let outType = type_to_llvm_type(expr._type)
            return any2float(ctx, arguments[0], expr.arguments[0]._type.vectorBaseType, outType)
        } else {
            var val = any2float(ctx, arguments[0], expr.arguments[0]._type.baseType)
            return build_broadcast_vector(ctx.builder, resType, val)
        }
    }
    var vres = LLVMGetUndef(LLVMVectorType(ctx.types.t_float, uint(expr._type.vectorDim)))
    var elemIndex = 0
    for (earg, arg in expr.arguments, arguments) {
        if (earg._type.isVectorType) {
            for (ei in range(earg._type.vectorDim)) {
                var eval = LLVMBuildExtractElement(ctx.builder, arg, ctx.types.ConstI32(uint64(ei)), "")
                var val = any2float(ctx, eval, earg._type.vectorBaseType)
                vres = LLVMBuildInsertElement(ctx.builder, vres, val, ctx.types.ConstI32(uint64(elemIndex++)), "")
            }
        } else {
            var val = any2float(ctx, arg, earg._type.baseType)
            vres = LLVMBuildInsertElement(ctx.builder, vres, val, ctx.types.ConstI32(uint64(elemIndex++)), "")
        }
    }
    if (elemIndex != resType.vectorDim) {
        failed_E(expr, "vector ctor {expr.name}({describe(expr._type)}) has {elemIndex} elements, but {resType.vectorDim} are expected")
    }
    return vres
}

def intrinsic_builtin_float(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    let floatType = base_type_to_llvm_type(expr._type.baseType)
    if (length(arguments) == 0) {
        return LLVMConstReal(floatType, 0.0lf)
    }
    assume argType = expr.arguments[0]._type
    if (argType.baseType == expr._type.baseType) {
        return arguments[0]
    } elif (argType.isSignedInteger) {
        return LLVMBuildSIToFP(ctx.builder, arguments[0], floatType, string(expr.name))
    } elif (argType.isUnsignedInteger) {
        return LLVMBuildUIToFP(ctx.builder, arguments[0], floatType, string(expr.name))
    } elif (argType.isFloatOrDouble) {
        if (argType.sizeOf > expr._type.sizeOf) {
            return LLVMBuildFPTrunc(ctx.builder, arguments[0], floatType, string(expr.name))
        } else {
            return LLVMBuildFPExt(ctx.builder, arguments[0], floatType, string(expr.name))
        }
    } else {
        return null
    }
}

def intrinsic_builtin_range(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var x, y : LLVMOpaqueValue?
    assume argType = expr.arguments[0]._type
    assume exprBaseType = expr._type.baseType
    let outSigned = exprBaseType == Type.tRange || exprBaseType == Type.tRange64
    let outBytes = (exprBaseType == Type.tRange64 || exprBaseType == Type.tURange64) ? 8 : 4
    let intType = (exprBaseType == Type.tRange64 || exprBaseType == Type.tURange64) ? ctx.types.t_int64 : ctx.types.t_int32
    if (expr.arguments |> length == 1) {
        if (argType.isVectorType) {
            return any2int(ctx, arguments[0], argType.vectorBaseType, type_to_llvm_type(expr._type), 4, outBytes, outSigned)
        } else {
            x = LLVMConstInt(intType, 0ul, 0)
            y = any2int(ctx, arguments[0], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
        }
    } else {
        x = any2int(ctx, arguments[0], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
        y = any2int(ctx, arguments[1], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
    }
    return LLVMBuildAnyInt2_xy(ctx.builder, ctx.types, type_to_llvm_type(expr._type), x, y, string(expr.name))
}

let private bit_op1_name <- {
    "clz" => "ctlz",
    "ctz" => "cttz",
    "popcnt" => "ctpop"
}

def build_op_name(op_name : string; opType : TypeDeclPtr) : string {
    if (opType.isInteger || (opType.isVectorType && (opType.vectorBaseType == Type.tInt || opType.vectorBaseType == Type.tUInt))) {
        if (opType.baseType == Type.tInt64 || opType.baseType == Type.tUInt64) {
            return "llvm.{op_name}.i64"
        } else {
            return "llvm.{op_name}.i32"
        }
    } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        if (opType.baseType == Type.tDouble) {
            return "llvm.{op_name}.f64"
        } else {
            return "llvm.{op_name}.f32"
        }
    } else {
        failed("{op_name}({describe(opType)}) is not supported (yet?)")
    }
    return ""
}

def intrinsic_bit_nzp_op1(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) <| $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type for {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0], LLVMConstInt(ctx.types.t_int1, 0ul, 0)]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], ctx.types.t_int1])
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def intrinsic_bit_op1(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) <| $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0]]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], argTypes)
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

let private op2_name <- {
    "min" => "minimum",
    "max" => "maximum",
    "pow" => "pow"
}

def intrinsic_op2(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    op2_name |> get(string(expr.name)) <| $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0], arguments[1]]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0]])
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def build_math_minmax(g_builder : LLVMOpaqueBuilder?; exprName : string; expr : smart_ptr<ExprCallFunc>; arg0, arg1 : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    if (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        var pred = exprName == "min" ? LLVMRealPredicate.LLVMRealOLT : LLVMRealPredicate.LLVMRealOGT
        var cond = LLVMBuildFCmp(g_builder, pred, arg0, arg1, "")
        return LLVMBuildSelect(g_builder, cond, arg0, arg1, "{exprName}")
    } elif (opType.isInteger || (opType.isVectorType && (opType.vectorBaseType == Type.tInt || opType.vectorBaseType == Type.tUInt))) {
        op_name = exprName == "min" ? "min" : "max"
        if (opType.isSignedIntegerOrIntVec) {
            op_name = "s{op_name}"
        } else {
            op_name = "u{op_name}"
        }
    } else {
        failed_E(expr, "{exprName} {describe(expr._type)} is not supported (yet?)")
        return null
    }
    let sqrt_name = build_op_name(op_name, opType)
    var args <- [ arg0, arg1]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0]])
    return LLVMBuildCall2(g_builder, typ, decl, args, string(exprName))
}

def intrinsic_math_minmax(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return build_math_minmax(ctx.builder, string(expr.name), expr, arguments[0], arguments[1])
}

def intrinsic_math_clamp(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var mini = build_math_minmax(ctx.builder, "max", expr, arguments[0], arguments[1])
    return build_math_minmax(ctx.builder, "min", expr, mini, arguments[2])
}

// intrinsics, math

def intrinsic_math_rcp(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        var one = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 1.0lf))
        return LLVMBuildFDiv(ctx.builder, one, arguments[0], "rcp")
    } elif (argType.baseType == Type.tDouble) {
        var one = LLVMConstReal(ctx.types.t_double, 1.0lf)
        return LLVMBuildFDiv(ctx.builder, one, arguments[0], "rcp")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_rsqrt(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        var one = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 1.0lf))
        var sq = intrinsic_math_any_float_op1("sqrt", ctx.builder, expr, arguments)
        return LLVMBuildFDiv(ctx.builder, one, sq, "rsqrt")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_any_float_op1(expr_name : string; g_builder : LLVMOpaqueBuilder?; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var sqrt_name = "llvm.{expr_name}.f32"
    assume opType = expr.arguments[0]._type
    if (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        if (opType.baseType == Type.tDouble) {
            sqrt_name = "llvm.{expr_name}.f64"
        }
    } else {
        failed_E(expr, "{expr_name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0]]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, expr_name)
}

def intrinsic_math_float_op1(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return intrinsic_math_any_float_op1(string(expr.name), ctx.builder, expr, arguments)
}

def intrinsic_math_float_op1_to_int(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var res = intrinsic_math_any_float_op1(string(expr.name) |> slice(0, -1), ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_tan(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var sin_a = intrinsic_math_any_float_op1("sin", ctx.builder, expr, arguments)
    var cos_a = intrinsic_math_any_float_op1("cos", ctx.builder, expr, arguments)
    return LLVMBuildFDiv(ctx.builder, sin_a, cos_a, "tan")
}

def intrinsic_math_lerp_op3(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var a = arguments[0]
    var b = arguments[1]
    var t = arguments[2]
    if (expr.arguments[0]._type.isVectorType && !expr.arguments[2]._type.isVectorType) {
        t = expand_scalar(ctx.builder, t, expr.arguments[0]._type)
    }
    var b_sub_a = LLVMBuildFSub(ctx.builder, b, a, "")
    var b_a_mul_t = LLVMBuildFMul(ctx.builder, b_sub_a, t, "")
    return LLVMBuildFAdd(ctx.builder, b_a_mul_t, a, "lerp")
}

def intrinsic_math_mad_op3(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var a = arguments[0]
    var b = arguments[1]
    if (expr.arguments[0]._type.isVectorType && !expr.arguments[1]._type.isVectorType) {
        b = expand_scalar(ctx.builder, b, expr.arguments[0]._type)
    }
    var c = arguments[2]
    let bt = expr._type.baseType
    if (bt == Type.tFloat || bt == Type.tDouble || bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4) {
        var a_mul_b = LLVMBuildFMul(ctx.builder, a, b, "")
        return LLVMBuildFAdd(ctx.builder, a_mul_b, c, "mad")
    } else {
        var a_mul_b = LLVMBuildMul(ctx.builder, a, b, "")
        return LLVMBuildAdd(ctx.builder, a_mul_b, c, "mad")
    }
}

def intrinsic_math_abs(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var int_name = ""
    if (opType.isInteger || (opType.isVectorType && (opType.vectorBaseType == Type.tInt || opType.vectorBaseType == Type.tUInt))) {
        if (opType.baseType == Type.tInt64 || opType.baseType == Type.tUInt64) {
            int_name = "llvm.abs.i64"
        } else {
            int_name = "llvm.abs.i32"
        }
        return intrinsic_math_abs_int(int_name, ctx, expr, arguments)
    } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        if (opType.baseType == Type.tDouble) {
            int_name = "llvm.fabs.f64"
        } else {
            int_name = "llvm.fabs.f32"
        }
        return intrinsic_math_abs_float(int_name, ctx.builder, expr, arguments)
    } else {
        failed("abs({describe(opType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_abs_float(int_name : string; g_builder : LLVMOpaqueBuilder?; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var args <- [ arguments[0]]
    var argTypes <- [ type_to_llvm_abi_type(opType)]
    var id = LLVMLookupIntrinsicID(int_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {int_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))
}

def intrinsic_math_abs_int(abs_name : string; ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var args <- [ arguments[0], LLVMConstInt(ctx.types.t_int1, 0ul, 0)]
    var argTypes <- [ type_to_llvm_abi_type(opType)]
    var id = LLVMLookupIntrinsicID(abs_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {abs_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], ctx.types.t_int1]) // type : (a, LLVMInt1Type) -> a
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def intrinsic_math_saturate(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    if (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        var v_0 = LLVMConstReal(ctx.types.t_float, double(0.))
        var v_1 = LLVMConstReal(ctx.types.t_float, double(1.))
        if (!opType.isFloatOrDouble) {
            v_0 = expand_scalar(ctx.builder, v_0, opType)
            v_1 = expand_scalar(ctx.builder, v_1, opType)
        }
        var cond_max = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOGT, arguments[0], v_0, "")
        var val_max  = LLVMBuildSelect(ctx.builder, cond_max, arguments[0], v_0, "")
        var cond_min = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOLT, val_max, v_1, "")
        return LLVMBuildSelect(ctx.builder, cond_min, val_max, v_1, "saturate")
    } else {
        failed_E(expr, "missing intrinsic {expr.name}({describe(opType)})")
        return null
    }
}

def build_fadd(var ctx : JitCtx; opType : TypeDeclPtr; v2 : LLVMOpaqueValue?; name : string) : LLVMOpaqueValue? {
    let fadd_name = "llvm.vector.reduce.fadd.v{opType.vectorDim}f32"
    let neg_0 = LLVMConstReal(ctx.types.t_float, -double(0.))
    var args_fadd <- [ neg_0, v2]
    var argTypes_fadd <- [ type_to_llvm_abi_type(opType)]
    var id = LLVMLookupIntrinsicID(fadd_name)
    var decl_fadd = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes_fadd)
    if (decl_fadd == null) {
        failed("missing intrinsic {fadd_name}")
        return null
    }
    var typ = LLVMFunctionType(ctx.types.t_float, [ ctx.types.t_float, type_to_llvm_abi_type(opType)])
    return LLVMBuildCall2(ctx.builder, typ, decl_fadd, args_fadd, name)
}

def build_fsqrt(var ctx : JitCtx; v2 : LLVMOpaqueValue?; name : string) : LLVMOpaqueValue? {
    let fsqrt_name = "llvm.sqrt.f32"
    var args_fsqrt <- [ v2]
    var argTypes_fsqrt <- [ ctx.types.t_float]
    var id = LLVMLookupIntrinsicID(fsqrt_name)
    var decl_fsqrt = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes_fsqrt)
    if (decl_fsqrt == null) {
        failed("missing intrinsic {fsqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(ctx.types.t_float, argTypes_fsqrt)
    return LLVMBuildCall2(ctx.builder, typ, decl_fsqrt, args_fsqrt, name)
}

def build_frcp(var ctx : JitCtx; v2 : LLVMOpaqueValue?; name : string) : LLVMOpaqueValue? {
    var one = LLVMConstReal(ctx.types.t_float, 1.0lf)
    var fdiv = LLVMBuildFDiv(ctx.builder, one, v2, "")
    return fdiv
}

def intrinsic_math_length_sq(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    return build_fadd(ctx, opType, v2, "length_sq")
}

def intrinsic_math_dot(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[1], "")
    return build_fadd(ctx, opType, v2, "dot")
}

def intrinsic_math_fast_normalize(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    let len = build_fsqrt(ctx, len_sq, "")
    let ilen = build_frcp(ctx, len, "")
    let vilen = build_broadcast_vector(ctx.builder, opType, ilen)
    return LLVMBuildFMul(ctx.builder, arguments[0], vilen, "fast_normalize")
}


def intrinsic_math_normalize(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    let len = build_fsqrt(ctx, len_sq, "")

    let zero_val = LLVMConstNull(ctx.types.t_float)
    let is_zero = ctx.builder |> LLVMBuildFCmp(LLVMRealPredicate.LLVMRealOEQ, len_sq, zero_val, "isVecZero")

    let ffunc       = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx.builder))
    let if_zero     = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "if.zero")
    let if_not_zero = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "if.notzero")
    let merge_bb    = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "merge")
    LLVMBuildCondBr(ctx.builder, is_zero, if_zero, if_not_zero)

    LLVMPositionBuilderAtEnd(ctx.builder, if_zero)
    let zero_vec = build_broadcast_vector(ctx.builder, opType, zero_val)
    LLVMBuildBr(ctx.builder, merge_bb)

    LLVMPositionBuilderAtEnd(ctx.builder, if_not_zero)
    let ilen     = build_frcp(ctx, len, "")
    let vilen    = build_broadcast_vector(ctx.builder, opType, ilen)
    let norm_vec = LLVMBuildFMul(ctx.builder, arguments[0], vilen, "normalize")
    LLVMBuildBr(ctx.builder, merge_bb)

    LLVMPositionBuilderAtEnd(ctx.builder, merge_bb)
    let phi = LLVMBuildPhi(ctx.builder, type_to_llvm_type(opType), "result")
    LLVMAddIncoming(phi, fixed_array(norm_vec, zero_vec), fixed_array(if_not_zero, if_zero))

    return phi
}

def intrinsic_math_cross(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    //! a.yzx * b.zxy - a.zxy * b.yzx
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType && opType.vectorDim == 3)
    let a = arguments[0]
    let b = arguments[1]
    var a_yzx = LLVMBuildShuffleVector(ctx.builder, ctx.types, a, a, [ 1, 2, 0, -1], "a_yzx")
    var b_zxy = LLVMBuildShuffleVector(ctx.builder, ctx.types, b, b, [ 2, 0, 1, -1], "b_zxy")
    var a_yzx_b_zxy = LLVMBuildFMul(ctx.builder, a_yzx, b_zxy, "a_yzx_b_zxy")
    var a_zxy = LLVMBuildShuffleVector(ctx.builder, ctx.types, a, a, [ 2, 0, 1, -1], "a_zxy")
    var b_yzx = LLVMBuildShuffleVector(ctx.builder, ctx.types, b, b, [ 1, 2, 0, -1], "b_yzx")
    var a_zxy_b_yzx = LLVMBuildFMul(ctx.builder, a_zxy, b_yzx, "a_zxy_b_yzx")
    var cross = LLVMBuildFSub(ctx.builder, a_yzx_b_zxy, a_zxy_b_yzx, "cross")
    return LLVMBuildShuffleVector(ctx.builder, ctx.types, cross, cross, [ 0, 1, 2], "cross")
}

def intrinsic_math_inv_distance_sq(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v = LLVMBuildFSub(ctx.builder, arguments[0], arguments[1], "")
    let v2 = LLVMBuildFMul(ctx.builder, v, v, "")
    let distance_sq = build_fadd(ctx, opType, v2, "")
    return LLVMBuildFDiv(ctx.builder, LLVMConstReal(ctx.types.t_float, 1.0lf), distance_sq, "inv_distance_sq")
}

def intrinsic_math_inv_length(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    let len = build_fsqrt(ctx, len_sq, "")
    return build_frcp(ctx, len, "inv_length")
}

def intrinsic_das_ptr_set_add(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var arg1 = arguments[1]
    var arg2 = arguments[2]
    if (expr.arguments[1]._type.baseType != Type.tInt) {
        arg2 = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], type_to_llvm_type(expr.arguments[1]._type), "")
    }
    var value_stride = LLVMBuildMul(ctx.builder, arg1, arg2, "")
    value_stride = LLVMBuildSExt(ctx.builder, value_stride, ctx.types.t_int64, "")
    var ptr_value = LLVMBuildLoad2(ctx.builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "")
    var ptr_value_i8 = LLVMBuildPointerCast(ctx.builder, ptr_value, LLVMPointerType(ctx.types.t_int8, 0u), "")
    var ptr_value_stride = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr_value_i8, value_stride, "")
    var vptr_value_stride = LLVMBuildPointerCast(ctx.builder, ptr_value_stride, type_to_llvm_type(expr.arguments[0]._type), "")
    return LLVMBuildStore(ctx.builder, vptr_value_stride, arguments[0])
}

def intrinsic_das_ptr_set_sub(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var args := arguments
    args[1] = LLVMBuildNeg(ctx.builder, args[1], "")
    return intrinsic_das_ptr_set_add(ctx, expr, args)
}

def intrinsic_das_ptr_add(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var arg1 = arguments[1]
    var arg2 = arguments[2]
    if (expr.arguments[1]._type.baseType != Type.tInt) {
        arg2 = LLVMBuildZExtOrBitCast(ctx.builder, arg2, type_to_llvm_type(expr.arguments[1]._type), "")
    }
    var value_stride = LLVMBuildMul(ctx.builder, arg1, arg2, "")
    value_stride = LLVMBuildSExt(ctx.builder, value_stride, ctx.types.t_int64, "")
    var ptr_value = arguments[0]
    if (expr.arguments[0]._type.flags.ref) {
        ptr_value = LLVMBuildLoad2(ctx.builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "")
    }
    var ptr_value_i8 = LLVMBuildPointerCast(ctx.builder, ptr_value, LLVMPointerType(ctx.types.t_int8, 0u), "")
    var ptr_value_stride = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr_value_i8, value_stride, "")
    var vptr_value_stride = LLVMBuildPointerCast(ctx.builder, ptr_value_stride, type_to_llvm_type(expr.arguments[0]._type), "")
    return vptr_value_stride
}

def intrinsic_das_ptr_sub(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var args := arguments
    args[1] = LLVMBuildNeg(ctx.builder, args[1], "")
    return intrinsic_das_ptr_add(ctx, expr, args)
}

def intrinsic_memset8(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    return LLVMBuildMemSet(ctx.builder, arguments[0], arguments[1], arguments[2], 1u)
}

def build_cmove_nz(var ctx : JitCtx; val : LLVMOpaqueValue?; blk : block) {
    let cb = LLVMGetInsertBlock(ctx.builder)
    var write = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "write")
    var skip = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "skip")
    var cond = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntNE, val, LLVMConstInt(ctx.types.t_int64, 0ul, 0), "")
    LLVMBuildCondBr(ctx.builder, cond, write, skip)
    LLVMPositionBuilderAtEnd(ctx.builder, write)
    invoke(blk)
    LLVMBuildBr(ctx.builder, skip)
    LLVMPositionBuilderAtEnd(ctx.builder, skip)
}

def build_write_n_values(var ctx : JitCtx; iptr, value, ival, isize : LLVMOpaqueValue?; count : int) {
    var ptr = LLVMBuildLoad2(ctx.builder, LLVMPointerType(ctx.types.LLVMInt4Type(), 0u), iptr, "")
    for (i in range(count)) {
        LLVMSetAlignment(LLVMBuildStore(ctx.builder, value, ptr), 4u)
        ptr = LLVMBuildGEP2(ctx.builder, ctx.types.LLVMInt4Type(), ptr, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    }
    LLVMBuildStore(ctx.builder, LLVMBuildSub(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, uint64(count), 0), ""), isize)
    LLVMBuildStore(ctx.builder, ptr, iptr)
}

def build_memset_128(var ctx : JitCtx; _ptr, value, _size : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    var isize = LLVMBuildAlloca(ctx.builder, ctx.types.t_int64, "isize")
    LLVMSetAlignment(isize, 8u)
    LLVMBuildStore(ctx.builder, _size, isize)
    var iptr = LLVMBuildAlloca(ctx.builder, LLVMPointerType(ctx.types.LLVMInt4Type(), 0u), "iptr")
    LLVMSetAlignment(iptr, 8u)
    LLVMBuildStore(ctx.builder, _ptr, iptr)
// if ( size & 1 ) write 1 value
    var ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var size_and_1 = LLVMBuildAnd(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    build_cmove_nz(ctx, size_and_1) <| $() {
        build_write_n_values(ctx, iptr, value, ival, isize, 1)
    }
// if ( size & 3 ) write 2 values
    ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var size_and_3 = LLVMBuildAnd(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, 3ul, 0), "")
    build_cmove_nz(ctx, size_and_3) <| $() {
        build_write_n_values(ctx, iptr, value, ival, isize, 2)
    }
// if ( size & 7 ) write 4 values
    ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var size_and_7 = LLVMBuildAnd(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, 7ul, 0), "")
    build_cmove_nz(ctx, size_and_7) <| $() {
        build_write_n_values(ctx, iptr, value, ival, isize, 4)
    }
// loop body
    let cb = LLVMGetInsertBlock(ctx.builder)
    var loop = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "loop")
    var loop_cont = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "loop_cont")
    var exit = LLVMInsertBasicBlockInContext(ctx.ctx, loop, "exit")
    var izero = LLVMConstInt(ctx.types.t_int64, 0ul, 0)
// loop:
    LLVMBuildBr(ctx.builder, loop)
    LLVMPositionBuilderAtEnd(ctx.builder, loop)
// while size
    ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var cond = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntNE, ival, izero, "")
    LLVMBuildCondBr(ctx.builder, cond, loop_cont, exit)
    LLVMPositionBuilderAtEnd(ctx.builder, loop_cont)
// ptr[i] = values
    build_write_n_values(ctx, iptr, value, ival, isize, 8)
// goto loop
    LLVMBuildBr(ctx.builder, loop)
// end:
    LLVMPositionBuilderAtEnd(ctx.builder, exit)
    return ival
}

def build_write_if_cond(ctx : JitCtx; cond, ptr, value : LLVMOpaqueValue?) {
    let cb = LLVMGetInsertBlock(ctx.builder)
    var write = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "write")
    var skip = LLVMInsertBasicBlockInContext(ctx.ctx, cb, "skip")
// if cond
    LLVMBuildCondBr(ctx.builder, cond, write, skip)
// write:
    LLVMPositionBuilderAtEnd(ctx.builder, write)
    LLVMBuildStore(ctx.builder, value, ptr)
// *ptr = value
    LLVMBuildBr(ctx.builder, skip)
// skip:
    LLVMPositionBuilderAtEnd(ctx.builder, skip)
}

def intrinsic_memset128(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var size = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], ctx.types.t_int64, "")
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.LLVMInt4Type(), 0u), "")
    return build_memset_128(ctx, ptr, arguments[1], size)
}

def build_memset_skip(var ctx : JitCtx; ptr_type : LLVMOpaqueType?; _ptr, val, _size : LLVMOpaqueValue?) {
    var ptr = _ptr
    var size = _size
// cond
    var size_and_1 = LLVMBuildAnd(ctx.builder, size, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    var cond = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntNE, size_and_1, LLVMConstInt(ctx.types.t_int64, 0ul, 0), "")
// if ( size & 1 ) write 1 value
    build_write_if_cond(ctx, cond, ptr, val)
// ptr += (size & 1)
    var ptr_add_1 = LLVMBuildGEP2(ctx.builder, ptr_type, ptr, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    ptr = LLVMBuildSelect(ctx.builder, cond, ptr_add_1, ptr, "")
// size &= ~1
    var size_sub_1 = LLVMBuildSub(ctx.builder, size, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    size = LLVMBuildSelect(ctx.builder, cond, size_sub_1, size, "")
// half size
    size = LLVMBuildLShr(ctx.builder, size, LLVMConstInt(ctx.types.t_int64, 1ul, 0), "")
    return (ptr, size)
}

def build_memset_64(var ctx : JitCtx; ptr_type : LLVMOpaqueType?; _ptr, val, _size : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    var ps = build_memset_skip(ctx, ptr_type, _ptr, val, _size)
    var ptr = LLVMBuildPointerCast(ctx.builder, ps._0, LLVMPointerType(ctx.types.LLVMInt4Type(), 0u), "")
    var size = ps._1
    var value = LLVMGetUndef(LLVMVectorType(ctx.types.t_int64, 2u))
    value = LLVMBuildInsertElement(ctx.builder, value, val, ctx.types.ConstI32(0ul), "")
    value = LLVMBuildInsertElement(ctx.builder, value, val, ctx.types.ConstI32(1ul), "")
    value = LLVMBuildBitCast(ctx.builder, value, ctx.types.LLVMInt4Type(), "")
    return build_memset_128(ctx, ptr, value, size)
}

def intrinsic_memset64(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var size = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], ctx.types.t_int64, "")
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int64, 0u), "")
    return build_memset_64(ctx, ctx.types.t_int64, ptr, arguments[1], size)
}

def build_memset_32(var ctx : JitCtx; ptr_type : LLVMOpaqueType?; _ptr, val, _size : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    var ps = build_memset_skip(ctx, ptr_type, _ptr, val, _size)
    var ptr = LLVMBuildPointerCast(ctx.builder, ps._0, LLVMPointerType(ctx.types.t_int64, 0u), "")
    var size = ps._1
    var value = LLVMBuildZExtOrBitCast(ctx.builder, val, ctx.types.t_int64, "")
    value = LLVMBuildOr(ctx.builder, value, LLVMBuildShl(ctx.builder, value, LLVMConstInt(ctx.types.t_int64, 32ul, 0), ""), "")
    return build_memset_64(ctx, ctx.types.t_int64, ptr, value, size)
}

def intrinsic_memset32(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var size = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], ctx.types.t_int64, "")
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int32, 0u), "")
    return build_memset_32(ctx, ctx.types.t_int32, ptr, arguments[1], size)
}

def build_memset_16(var ctx : JitCtx; ptr_type : LLVMOpaqueType?; _ptr, val, _size : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    var ps = build_memset_skip(ctx, ptr_type, _ptr, val, _size)
    var ptr = LLVMBuildPointerCast(ctx.builder, ps._0, LLVMPointerType(ctx.types.t_int32, 0u), "")
    var size = ps._1
    var value = LLVMBuildZExtOrBitCast(ctx.builder, val, ctx.types.t_int32, "")
    value = LLVMBuildOr(ctx.builder, value, LLVMBuildShl(ctx.builder, value, ctx.types.ConstI32(16ul), ""), "")
    return build_memset_32(ctx, ctx.types.t_int32, ptr, value, size)
}

def intrinsic_memset16(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var size = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], ctx.types.t_int64, "")
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int16, 0u), "")
    return build_memset_16(ctx, ctx.types.t_int16, ptr, arguments[1], size)
}

def intrinsic_math_gather(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int32, 0u), "")
    var ix = LLVMBuildExtractElement(ctx.builder, arguments[1], ctx.types.ConstI32(0ul), "")
    var iy = LLVMBuildExtractElement(ctx.builder, arguments[1], ctx.types.ConstI32(1ul), "")
    var iz = LLVMBuildExtractElement(ctx.builder, arguments[1], ctx.types.ConstI32(2ul), "")
    var iw = LLVMBuildExtractElement(ctx.builder, arguments[1], ctx.types.ConstI32(3ul), "")
    var xaddr = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, ix, "")
    var yaddr = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iy, "")
    var zaddr = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iz, "")
    var waddr = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iw, "")
    var res = LLVMGetUndef(ctx.types.LLVMInt4Type())
    res = LLVMBuildInsertElement(ctx.builder, res, LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, xaddr, ""), ctx.types.ConstI32(0ul), "")
    res = LLVMBuildInsertElement(ctx.builder, res, LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, yaddr, ""), ctx.types.ConstI32(1ul), "")
    res = LLVMBuildInsertElement(ctx.builder, res, LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, zaddr, ""), ctx.types.ConstI32(2ul), "")
    res = LLVMBuildInsertElement(ctx.builder, res, LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, waddr, ""), ctx.types.ConstI32(3ul), "")
    return res
}

def intrinsic_math_store_neq_mask(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.LLVMInt4Type(), 0u), "")
    var value = arguments[1]
    var mask_v = arguments[2]
    var mask = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, value, mask_v, "")
    var dest = LLVMBuildLoad2(ctx.builder, ctx.types.LLVMInt4Type(), ptr, "")
    LLVMSetAlignment(dest, 4u)
    var res = LLVMBuildSelect(ctx.builder, mask, dest, value, "")
    var wrt = LLVMBuildStore(ctx.builder, res, ptr)
    LLVMSetAlignment(wrt, 4u)
    return res
}

def intrinsic_math_gather_store_mask(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var arg_gather <- [ arguments[1], arguments[2]]
    var gather = intrinsic_math_gather(ctx, expr, arg_gather)
    var arg_store_mask <- [ arguments[0], gather, arguments[3]]
    return intrinsic_math_store_neq_mask(ctx, expr, arg_store_mask)
}

def intrinsic_math_gather_store_stride(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    // gather
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(ctx.types.t_int32, 0u), "")
    var ix = LLVMBuildExtractElement(ctx.builder, arguments[3], ctx.types.ConstI32(0ul), "")
    var iy = LLVMBuildExtractElement(ctx.builder, arguments[3], ctx.types.ConstI32(1ul), "")
    var iz = LLVMBuildExtractElement(ctx.builder, arguments[3], ctx.types.ConstI32(2ul), "")
    var iw = LLVMBuildExtractElement(ctx.builder, arguments[3], ctx.types.ConstI32(3ul), "")
    var X = LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, ix, ""), "")
    var Y = LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iy, ""), "")
    var Z = LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iz, ""), "")
    var W = LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, ptr, iw, ""), "")
    // write it out
    var stride = arguments[1]
    var out = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int32, 0u), "")
    LLVMBuildStore(ctx.builder, X, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, out, stride, "")
    LLVMBuildStore(ctx.builder, Y, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, out, stride, "")
    LLVMBuildStore(ctx.builder, Z, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int32, out, stride, "")
    LLVMBuildStore(ctx.builder, W, out)
    return out
}


/*
    __forceinline void v_scatter ( void * _ptr, vec4f index, vec4f value ) {
        // write 4 floats to memory, using 4 uint32_t indices
        auto ptr = (float *) _ptr
        auto i = v_cast_vec4i(index)
        ptr[uint32_t(v_extract_xi(i))] = v_extract_x(value)
        ptr[uint32_t(v_extract_yi(i))] = v_extract_y(value)
        ptr[uint32_t(v_extract_zi(i))] = v_extract_z(value)
        ptr[uint32_t(v_extract_wi(i))] = v_extract_w(value)
    }
*/

/*
    __forceinline void v_scatter_mask ( void * _ptr, vec4f index, vec4f value, vec4f mask_v ) {
        // write 4 floats to memory, using 4 uint32_t indices, but only for floats, where value[i]!=mask_v[i]
        auto ptr = (float *) _ptr
        auto mask = v_cmp_eqi(v_cast_vec4i(mask_v), v_cast_vec4i(value))
        auto i = v_cast_vec4i(index)
        if ( !v_extract_xi(mask) ) ptr[uint32_t(v_extract_xi(i))] = v_extract_x(value)
        if ( !v_extract_yi(mask) ) ptr[uint32_t(v_extract_yi(i))] = v_extract_y(value)
        if ( !v_extract_zi(mask) ) ptr[uint32_t(v_extract_zi(i))] = v_extract_z(value)
        if ( !v_extract_wi(mask) ) ptr[uint32_t(v_extract_wi(i))] = v_extract_w(value)
    }
*/

// gather_store_neq_mask
// def intrinsic_gather_store_neq_mask

def intrinsic_math_u8x4_gather_store(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    // gather
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(ctx.types.t_int8, 0u), "")
    var ix = LLVMBuildExtractElement(ctx.builder, arguments[2], ctx.types.ConstI32(0ul), "")
    var iy = LLVMBuildExtractElement(ctx.builder, arguments[2], ctx.types.ConstI32(1ul), "")
    var iz = LLVMBuildExtractElement(ctx.builder, arguments[2], ctx.types.ConstI32(2ul), "")
    var iw = LLVMBuildExtractElement(ctx.builder, arguments[2], ctx.types.ConstI32(3ul), "")
    var X = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr, ix, ""), "")
    var Y = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr, iy, ""), "")
    var Z = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr, iz, ""), "")
    var W = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, ptr, iw, ""), "")
    // write it out
    var stride = ctx.types.ConstI32(1ul)
    var out = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int8, 0u), "")
    LLVMBuildStore(ctx.builder, X, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, out, stride, "")
    LLVMBuildStore(ctx.builder, Y, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, out, stride, "")
    LLVMBuildStore(ctx.builder, Z, out)
    out = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, out, stride, "")
    LLVMBuildStore(ctx.builder, W, out)
    return out
}

def intrinsic_math_roundi(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var res = intrinsic_math_any_float_op1("round", ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_trunci(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var res = intrinsic_math_any_float_op1("trunc", ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_sign(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume resType = expr._type
    if (resType.isSignedInteger || (resType.isVectorType && resType.vectorBaseType == Type.tInt)) {
        var itype = base_type_to_llvm_type(resType.isSignedInteger ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector(ctx.builder, resType, LLVMConstInt(itype, 0ul, 0))
        var one = build_broadcast_vector(ctx.builder, resType, LLVMConstInt(itype, 1ul, 0))
        var minus_one = build_broadcast_vector(ctx.builder, resType, LLVMConstInt(itype, uint64(-1), 0))
        var cond_z = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, arguments[0], zero, "")
        var cond_gt = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntSGT, arguments[0], zero, "")
        var res = LLVMBuildSelect(ctx.builder, cond_gt, one, minus_one, "")
        res = LLVMBuildSelect(ctx.builder, cond_z, zero, res, "")
        return res
    } elif (resType.isUnsignedInteger || (resType.isVectorType && resType.vectorBaseType == Type.tUInt)) {
        var itype = base_type_to_llvm_type(resType.isUnsignedInteger ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector(ctx.builder, resType, LLVMConstInt(itype, 0ul, 0))
        var one = build_broadcast_vector(ctx.builder, resType, LLVMConstInt(itype, 1ul, 0))
        var cond = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, arguments[0], zero, "")
        var res = LLVMBuildSelect(ctx.builder, cond, zero, one, "")
        return res
    } elif (resType.isFloatOrDouble || (resType.isVectorType && resType.vectorBaseType == Type.tFloat)) {
        var ftype = base_type_to_llvm_type(resType.isFloatOrDouble ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector(ctx.builder, resType, LLVMConstReal(ftype, 0.0lf))
        var one = build_broadcast_vector(ctx.builder, resType, LLVMConstReal(ftype, 1.0lf))
        var minus_one = build_broadcast_vector(ctx.builder, resType, LLVMConstReal(ftype, -1.0lf))
        var cond_z = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOEQ, arguments[0], zero, "")
        var cond_gt = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOGT, arguments[0], zero, "")
        var res = LLVMBuildSelect(ctx.builder, cond_gt, one, minus_one, "")
        res = LLVMBuildSelect(ctx.builder, cond_z, zero, res, "")
        return res
    } else {
        failed("sign({describe(resType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_mul_128(var ctx : JitCtx; expr : smart_ptr<ExprCallFunc>; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var a = arguments[0]
    var b = arguments[1]
    var A = LLVMBuildZExtOrBitCast(ctx.builder, a, ctx.types.t_int128, "")
    var B = LLVMBuildZExtOrBitCast(ctx.builder, b, ctx.types.t_int128, "")
    var ab = LLVMBuildMul(ctx.builder, A, B, "")
    let lo = LLVMBuildTrunc(ctx.builder, ab, ctx.types.t_int64, "")
    var hi = LLVMBuildLShr(ctx.builder, ab, LLVMConstInt(ctx.types.t_int128, 64ul, 0), "")
    hi = LLVMBuildTrunc(ctx.builder, hi, ctx.types.t_int64, "")
    var value = LLVMGetUndef(ctx.types.LLVMRange64Type())
    value = LLVMBuildInsertElement(ctx.builder, value, lo, ctx.types.ConstI32(0ul), "")
    value = LLVMBuildInsertElement(ctx.builder, value, hi, ctx.types.ConstI32(1ul), "")
    return value
}
