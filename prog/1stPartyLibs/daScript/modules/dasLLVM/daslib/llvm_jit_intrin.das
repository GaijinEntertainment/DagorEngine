options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module llvm_jit_intrin shared private

require llvm/llvm_boost
require llvm/llvm_jit_common
require daslib/ast_boost
require strings

let g_intrin_lookup <- {{
// $, aka builtin
    // misc
        "$::length" => @@intrinsic_builtin_length;
    // pointer math
        "$::i_das_ptr_set_add" => @@intrinsic_das_ptr_set_add;
        "$::i_das_ptr_add" => @@intrinsic_das_ptr_add;
    // memset
        "$::memset8" => @@intrinsic_memset8;
        "$::memset16" => @@intrinsic_memset16;
        "$::memset32" => @@intrinsic_memset32;
        "$::memset64" => @@intrinsic_memset64;
        "$::memset128" => @@intrinsic_memset128;
    // type c-tors
    // vector type c-tors
        "$::range" => @@intrinsic_builtin_range;
        "$::urange" => @@intrinsic_builtin_range;
        "$::range64" => @@intrinsic_builtin_range;
        "$::urange64" => @@intrinsic_builtin_range;
        "$::interval" => @@intrinsic_builtin_range;
        "$::float2" => @@intrinsic_builtin_float_vec;
        "$::float3" => @@intrinsic_builtin_float_vec;
        "$::float4" => @@intrinsic_builtin_float_vec;
        "$::int2" => @@intrinsic_builtin_int_vec;
        "$::int3" => @@intrinsic_builtin_int_vec;
        "$::int4" => @@intrinsic_builtin_int_vec;
        "$::uint2" => @@intrinsic_builtin_int_vec;
        "$::uint3" => @@intrinsic_builtin_int_vec;
        "$::uint4" => @@intrinsic_builtin_int_vec;
    // basic type c-tor
            "$::float" => @@intrinsic_builtin_float;
            "$::double" => @@intrinsic_builtin_float;
            "$::int8" => @@intrinsic_builtin_int;
            "$::uint8" => @@intrinsic_builtin_int;
            "$::int16" => @@intrinsic_builtin_int;
            "$::uint16" => @@intrinsic_builtin_int;
            "$::int" => @@intrinsic_builtin_int;
            "$::uint" => @@intrinsic_builtin_int;
            "$::int64" => @@intrinsic_builtin_int;
            "$::uint64" => @@intrinsic_builtin_int;
            "$::bitfield" => @@intrinsic_builtin_int;
    // bits
        "$::clz" => @@intrinsic_bit_nzp_op1;
        "$::ctz" => @@intrinsic_bit_nzp_op1;
        "$::popcnt" => @@intrinsic_bit_op1;
    // mul
        "$::mul128" => @@intrinsic_mul_128;
    // variant
        "$::set_variant_index" => @@intrinsic_builtin_set_variant_index;
        "$::variant_index" => @@intrinsic_builtin_variant_index;
// math - scatter gather
    "math::gather" => @@intrinsic_math_gather;
    "math::store_neq_mask" => @@intrinsic_math_store_neq_mask;
    "math::gather_store_neq_mask" => @@intrinsic_math_gather_store_mask;
    "math::gather_store_stride" => @@intrinsic_math_gather_store_stride;
    "math::u8x4_gather_store" => @@intrinsic_math_u8x4_gather_store;
// math
    "math::sign" => @@intrinsic_math_sign;
    "math::saturate" => @@intrinsic_math_saturate;
    "math::abs" => @@intrinsic_math_abs;
    "math::mad" => @@intrinsic_math_mad_op3;
    "math::lerp" => @@intrinsic_math_lerp_op3;
    "math::rcp" => @@intrinsic_math_rcp;
    "math::rcp_est"=> @@intrinsic_math_rcp;
    "math::exp" => @@intrinsic_math_float_op1;
    "math::log" => @@intrinsic_math_float_op1;
    "math::exp2" => @@intrinsic_math_float_op1;
    "math::log2" => @@intrinsic_math_float_op1;
    "math::sqrt" => @@intrinsic_math_float_op1;
    "math::rsqrt" => @@intrinsic_math_rsqrt;
    "math::rsqrt_est" => @@intrinsic_math_rsqrt;
    "math::tan" => @@intrinsic_math_tan;
    "math::sin" => @@intrinsic_math_float_op1;
    "math::cos" => @@intrinsic_math_float_op1;
    "math::pow" => @@intrinsic_op2;
    "math::min" => @@intrinsic_math_minmax;
    "math::max" => @@intrinsic_math_minmax;
    "math::clamp" => @@intrinsic_math_clamp;
    "math::floor" => @@intrinsic_math_float_op1;
    "math::ceil" => @@intrinsic_math_float_op1;
    "math::floori" => @@intrinsic_math_float_op1_to_int;
    "math::ceili" => @@intrinsic_math_float_op1_to_int;
    "math::roundi" => @@intrinsic_math_roundi;
    "math::trunci" => @@intrinsic_math_trunci;
    "math::length_sq" => @@intrinsic_math_length_sq;
    "math::dot" => @@intrinsic_math_dot;
    "math::cross" => @@intrinsic_math_cross;
    "math::inv_distance_sq" => @@intrinsic_math_inv_distance_sq;
    "math::inv_length" => @@intrinsic_math_inv_length;
    "math::normalize" => @@intrinsic_math_normalize
}}

def public lookup_intinsic(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var result :  LLVMOpaqueValue?
    let call_name = "{expr.func._module.name}::{expr.func.name}"
    g_intrin_lookup |> get(call_name) <| $ ( pfun )
        result = pfun |> invoke(g_builder, expr, arguments)
    return result

// intrinsics, $ aka builtin

def intrinsic_builtin_variant_index(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    return LLVMBuildLoad2(g_builder, LLVMInt32Type(), arguments[0], "$::variant_index")

def intrinsic_builtin_set_variant_index(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    return LLVMBuildStore2(g_builder, LLVMInt32Type(), arguments[1], arguments[0])

def intrinsic_builtin_length(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume argType = expr.arguments[0]._type
    if argType.isGoodArrayType
        var arr = LLVMBuildLoad2(g_builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "arr")
        return LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY SIZE), "array.size")
    else
        return null

def intrinsic_builtin_int(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    let intType = base_type_to_llvm_type(expr._type.baseType)
    if length(arguments)==0
        return LLVMConstInt(intType, 0ul, 0)
    assume argType = expr.arguments[0]._type
    if argType.baseType==expr._type.baseType
        return arguments[0] // ctor of the same type is the same value, i.e int(a:int) is a
    elif argType.baseType==Type tFloat || argType.baseType==Type tDouble
        if expr._type.isSignedInteger
            return LLVMBuildFPToSI(g_builder, arguments[0], intType, string(expr.name))
        else
            return LLVMBuildFPToUI(g_builder, arguments[0], intType, string(expr.name))
    elif argType.isInteger || argType.isEnum
        if argType.sizeOf>=expr._type.sizeOf
            return LLVMBuildTruncOrBitCast(g_builder, arguments[0],intType, string(expr.name))
        elif argType.isSignedInteger || argType.isEnum
            return LLVMBuildSExtOrBitCast(g_builder, arguments[0], intType, string(expr.name))
        else
            return LLVMBuildZExtOrBitCast(g_builder, arguments[0], intType, string(expr.name))
    else
        return null

def any2int ( g_builder:LLVMOpaqueBuilder?; val:LLVMOpaqueValue?; argType:Type; _outType:LLVMOpaqueType?; inbytes, outbytes:int; outSigned:bool ) : LLVMOpaqueValue?
    var outType = _outType!=null ? _outType : LLVMInt32Type()
    if argType==Type tFloat || argType==Type tDouble
        if outSigned
            return LLVMBuildFPToSI(g_builder, val, outType, "")
        else
            return LLVMBuildFPToUI(g_builder, val, outType, "")
    else
        return val if inbytes==outbytes
        let baseSInt = argType==Type tInt8 || argType==Type tInt16 || argType==Type tInt || argType==Type tInt64
        let baseUInt = argType==Type tUInt8 || argType==Type tUInt16 || argType==Type tUInt || argType==Type tUInt64
        if baseSInt || baseUInt
            if inbytes>=outbytes
                return LLVMBuildTruncOrBitCast(g_builder, val, outType, "")
            elif baseSInt
                return LLVMBuildSExtOrBitCast(g_builder, val, outType, "")
            else
                return LLVMBuildZExtOrBitCast(g_builder, val, outType, "")
        else
            panic("unsupported any2int base type {argType}")
            return null

def intrinsic_builtin_int_vec(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume resType = expr._type
    assume argType = expr.arguments[0]._type
    if length(arguments) == 0
        return build_broadcast_vector (g_builder, resType, LLVMConstInt(LLVMInt32Type(), 0ul, 0))
    let outSigned = resType.vectorBaseType==Type tInt
    // this works around the bug of fp2ui not working correctly for vectors on some values
    var isFloatToUintVec = argType.isVectorType && (argType.vectorBaseType==Type tFloat) && !outSigned
    if length(arguments) == 1 && !isFloatToUintVec
        if argType.isVectorType
            return any2int(g_builder, arguments[0], argType.vectorBaseType, type_to_llvm_type(expr._type), 4, 4, outSigned)
        else
            var val = any2int(g_builder, arguments[0], argType.baseType, null, argType.sizeOf, 4, outSigned)
            return build_broadcast_vector (g_builder, resType, val)
    var vres = LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), uint(expr._type.vectorDim)))
    var elemIndex = 0
    for earg,arg in expr.arguments,arguments
        if earg._type.isVectorType
            for ei in range(earg._type.vectorDim)
                var eval = LLVMBuildExtractElement(g_builder, arg, LLVMConstInt(LLVMInt32Type(), uint64(ei), 0), "")
                var val = any2int(g_builder, eval, earg._type.vectorBaseType, null, 4, 4, outSigned)
                vres = LLVMBuildInsertElement(g_builder, vres, val, LLVMConstInt(LLVMInt32Type(), uint64(elemIndex++), 0), "")
        else
            var val = any2int(g_builder, arg, earg._type.baseType, null, earg._type.sizeOf, 4, outSigned)
            vres = LLVMBuildInsertElement(g_builder, vres, val, LLVMConstInt(LLVMInt32Type(), uint64(elemIndex++), 0), "")
    if elemIndex != resType.vectorDim
        failed_E(expr, "vector ctor {expr.name}({describe(expr._type)}) has {elemIndex} elements, but {resType.vectorDim} are expected")
    return vres

def any2float ( g_builder:LLVMOpaqueBuilder?; val:LLVMOpaqueValue?; argType:Type; _outType:LLVMOpaqueType? = null )
    let outType = _outType!=null ? _outType : LLVMFloatType()
    return val                                              if argType==Type tFloat
    return LLVMBuildFPTrunc(g_builder, val, outType, "")    if argType==Type tDouble
    return LLVMBuildSIToFP(g_builder, val,outType, "")      if argType==Type tInt8 || argType==Type tInt16 || argType==Type tInt || argType==Type tInt64
    return LLVMBuildUIToFP(g_builder, val, outType, "")     if argType==Type tUInt8 || argType==Type tUInt16 || argType==Type tUInt || argType==Type tUInt64
    panic("unsupported any2float base type {argType}")
    return null

def intrinsic_builtin_float_vec(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume resType = expr._type
    if length(arguments) == 0
        return build_broadcast_vector (g_builder, resType, LLVMConstReal(LLVMFloatType(), 0.0lf))
    if length(arguments) == 1
        if expr.arguments[0]._type.isVectorType
            let outType = type_to_llvm_type(expr._type)
            return any2float(g_builder, arguments[0], expr.arguments[0]._type.vectorBaseType, outType)
        else
            var val = any2float(g_builder, arguments[0], expr.arguments[0]._type.baseType)
            return build_broadcast_vector (g_builder, resType, val)
    var vres = LLVMGetUndef(LLVMVectorType(LLVMFloatType(), uint(expr._type.vectorDim)))
    var elemIndex = 0
    for earg,arg in expr.arguments,arguments
        if earg._type.isVectorType
            for ei in range(earg._type.vectorDim)
                var eval = LLVMBuildExtractElement(g_builder, arg, LLVMConstInt(LLVMInt32Type(), uint64(ei), 0), "")
                var val = any2float(g_builder, eval, earg._type.vectorBaseType)
                vres = LLVMBuildInsertElement(g_builder, vres, val, LLVMConstInt(LLVMInt32Type(), uint64(elemIndex++), 0), "")
        else
            var val = any2float(g_builder, arg, earg._type.baseType)
            vres = LLVMBuildInsertElement(g_builder, vres, val, LLVMConstInt(LLVMInt32Type(), uint64(elemIndex++), 0), "")
    if elemIndex != resType.vectorDim
        failed_E(expr, "vector ctor {expr.name}({describe(expr._type)}) has {elemIndex} elements, but {resType.vectorDim} are expected")
    return vres

def intrinsic_builtin_float(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    let floatType = base_type_to_llvm_type(expr._type.baseType)
    if length(arguments)==0
        return LLVMConstReal(floatType, 0.0lf)
    assume argType = expr.arguments[0]._type
    if argType.baseType==expr._type.baseType
        return arguments[0]
    elif argType.isSignedInteger
        return LLVMBuildSIToFP(g_builder, arguments[0], floatType, string(expr.name))
    elif argType.isUnsignedInteger
        return LLVMBuildUIToFP(g_builder, arguments[0], floatType, string(expr.name))
    elif argType.isFloatOrDouble
        if argType.sizeOf > expr._type.sizeOf
            return LLVMBuildFPTrunc(g_builder, arguments[0], floatType, string(expr.name))
        else
            return LLVMBuildFPExt(g_builder, arguments[0], floatType, string(expr.name))
    else
        return null

def intrinsic_builtin_range(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var x, y : LLVMOpaqueValue?
    assume argType = expr.arguments[0]._type
    assume exprBaseType = expr._type.baseType
    let outSigned = exprBaseType==Type tRange || exprBaseType==Type tRange64
    let outBytes = (exprBaseType==Type tRange64 || exprBaseType==Type tURange64) ? 8 : 4
    let intType = (exprBaseType==Type tRange64 || exprBaseType==Type tURange64) ? LLVMInt64Type() : LLVMInt32Type()
    if expr.arguments |> length==1
        if argType.isVectorType
            return any2int(g_builder, arguments[0], argType.vectorBaseType, type_to_llvm_type(expr._type), 4, outBytes, outSigned)
        else
            x = LLVMConstInt(intType, 0ul, 0)
            y = any2int(g_builder, arguments[0], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
    else
        x = any2int(g_builder, arguments[0], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
        y = any2int(g_builder, arguments[1], argType.baseType, intType, argType.sizeOf, outBytes, outSigned)
    return LLVMBuildAnyInt2_xy(g_builder, type_to_llvm_type(expr._type), x, y, string(expr.name))

let private bit_op1_name <- {{
    "clz" => "ctlz";
    "ctz" => "cttz";
    "popcnt" => "ctpop"
}}

def build_op_name ( op_name:string; opType:TypeDeclPtr ) : string
    if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
        if opType.baseType==Type tInt64 || opType.baseType==Type tUInt64
            return "llvm.{op_name}.i64"
        else
            return "llvm.{op_name}.i32"
    elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
        if opType.baseType==Type tDouble
            return "llvm.{op_name}.f64"
        else
            return "llvm.{op_name}.f32"
    else
        failed("{op_name}({describe(opType)}) is not supported (yet?)")
    return ""

def intrinsic_bit_nzp_op1(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) <| $ ( pname )
        op_name = pname
    if op_name==""
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var sqrt_name = build_op_name(op_name, opType)
    if sqrt_name==""
        failed_E(expr, "intrinsic type for {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var args <- [{auto[] arguments[0]; LLVMConstInt(LLVMInt1Type(), 0ul, 0)}]
    var argTypes <- [{auto type_to_llvm_abi_type(expr.arguments[0]._type)}]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; LLVMInt1Type()}])
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))

def intrinsic_bit_op1(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) <| $ ( pname )
        op_name = pname
    if op_name==""
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var sqrt_name = build_op_name(op_name, opType)
    if sqrt_name==""
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var args <- [{auto[] arguments[0]}]
    var argTypes <- [{auto type_to_llvm_abi_type(expr.arguments[0]._type)}]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], argTypes)
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))

let private op2_name <- {{
    "min" => "minimum";
    "max" => "maximum";
    "pow" => "pow"
}}

def intrinsic_op2(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var op_name : string
    op2_name |> get(string(expr.name)) <| $ ( pname )
        op_name = pname
    if op_name==""
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var sqrt_name = build_op_name(op_name, opType)
    if sqrt_name==""
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    var args <- [{auto[] arguments[0]; arguments[1]}]
    var argTypes <- [{auto type_to_llvm_abi_type(expr.arguments[0]._type)}]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; argTypes[0]}])
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))

def build_math_minmax(g_builder:LLVMOpaqueBuilder?; exprName:string; expr:smart_ptr<ExprCallFunc>; arg0,arg1:LLVMOpaqueValue?) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var op_name : string
    if opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
        var pred = exprName=="min" ? LLVMRealPredicate LLVMRealOLT : LLVMRealPredicate LLVMRealOGT
        var cond = LLVMBuildFCmp(g_builder, pred, arg0, arg1, "")
        return LLVMBuildSelect( g_builder, cond, arg0, arg1, "{exprName}")
    elif opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
        op_name = exprName=="min" ? "min" : "max"
        if opType.isSignedIntegerOrIntVec
            op_name = "s{op_name}"
        else
            op_name = "u{op_name}"
    else
        failed_E(expr, "{exprName} {describe(expr._type)} is not supported (yet?)")
        return null
    let sqrt_name = build_op_name(op_name, opType)
    var args <- [{auto[] arg0; arg1}]
    var argTypes <- [{auto type_to_llvm_abi_type(expr.arguments[0]._type)}]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; argTypes[0]}])
    return LLVMBuildCall2(g_builder, typ, decl, args, string(exprName))

def intrinsic_math_minmax(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    return build_math_minmax(g_builder, string(expr.name), expr, arguments[0], arguments[1])

def intrinsic_math_clamp(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var mini = build_math_minmax(g_builder, "max", expr, arguments[0], arguments[1])
    return build_math_minmax(g_builder, "min", expr, mini, arguments[2])

// intrinsics, math

def intrinsic_math_rcp(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume argType = expr.arguments[0]._type
    if argType.baseType==Type tFloat || (argType.isVectorType && argType.vectorBaseType==Type tFloat)
        var one = build_broadcast_vector (g_builder, argType, LLVMConstReal(LLVMFloatType(), 1.0lf))
        return LLVMBuildFDiv(g_builder, one, arguments[0], "rcp")
    elif argType.baseType==Type tDouble
        var one = LLVMConstReal(LLVMDoubleType(), 1.0lf)
        return LLVMBuildFDiv(g_builder, one, arguments[0], "rcp")
    else
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null

def intrinsic_math_rsqrt(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume argType = expr.arguments[0]._type
    if argType.baseType==Type tFloat || (argType.isVectorType && argType.vectorBaseType==Type tFloat)
        var one = build_broadcast_vector (g_builder, argType, LLVMConstReal(LLVMFloatType(), 1.0lf))
        var sq = intrinsic_math_any_float_op1("sqrt", g_builder, expr, arguments)
        return LLVMBuildFDiv(g_builder, one, sq, "rsqrt")
    else
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null

def intrinsic_math_any_float_op1(expr_name:string; g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var sqrt_name = "llvm.{expr_name}.f32"
    assume opType = expr.arguments[0]._type
    if opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
        if opType.baseType==Type tDouble
            sqrt_name = "llvm.{expr_name}.f64"
    else
        failed_E(expr, "{expr_name}({describe(opType)}) is not supported (yet?)")
        return null
    var args <- [{auto[] arguments[0]}]
    var argTypes <- [{auto type_to_llvm_abi_type(expr.arguments[0]._type)}]
    var id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, expr_name)

def intrinsic_math_float_op1(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    return intrinsic_math_any_float_op1(string(expr.name), g_builder, expr, arguments)

def intrinsic_math_float_op1_to_int(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var res = intrinsic_math_any_float_op1(string(expr.name) |> slice(0,-1), g_builder, expr, arguments)
    return LLVMBuildFPToSI(g_builder, res, type_to_llvm_abi_type(expr._type), "")

def intrinsic_math_tan(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var sin_a = intrinsic_math_any_float_op1("sin", g_builder, expr, arguments)
    var cos_a = intrinsic_math_any_float_op1("cos", g_builder, expr, arguments)
    return LLVMBuildFDiv(g_builder, sin_a, cos_a, "tan")

def intrinsic_math_lerp_op3(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var a = arguments[0]
    var b = arguments[1]
    var t = arguments[2]
    if expr.arguments[0]._type.isVectorType && !expr.arguments[2]._type.isVectorType
        t = expand_scalar(g_builder, t, expr.arguments[0]._type)
    var b_sub_a = LLVMBuildFSub(g_builder, b, a, "")
    var b_a_mul_t = LLVMBuildFMul(g_builder, b_sub_a, t, "")
    return LLVMBuildFAdd(g_builder, b_a_mul_t, a, "lerp")

def intrinsic_math_mad_op3(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var a = arguments[0]
    var b = arguments[1]
    if expr.arguments[0]._type.isVectorType && !expr.arguments[1]._type.isVectorType
        b = expand_scalar(g_builder, b, expr.arguments[0]._type)
    var c = arguments[2]
    let bt = expr._type.baseType
    if bt==Type tFloat || bt==Type tDouble || bt==Type tFloat2 || bt==Type tFloat3 || bt==Type tFloat4
        var a_mul_b = LLVMBuildFMul(g_builder, a, b, "")
        return LLVMBuildFAdd(g_builder, a_mul_b, c, "mad")
    else
        var a_mul_b = LLVMBuildMul(g_builder, a, b, "")
        return LLVMBuildAdd(g_builder, a_mul_b, c, "mad")

def intrinsic_math_abs(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var int_name = ""
    if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
        if opType.baseType==Type tInt64 || opType.baseType==Type tUInt64
            int_name = "llvm.abs.i64"
        else
            int_name = "llvm.abs.i32"
        return intrinsic_math_abs_int(int_name, g_builder, expr, arguments)
    elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
        if opType.baseType==Type tDouble
            int_name = "llvm.fabs.f64"
        else
            int_name = "llvm.fabs.f32"
        return intrinsic_math_abs_float(int_name, g_builder, expr, arguments)
    else
        failed("abs({describe(opType)}) is not supported (yet?)")
        return null

def intrinsic_math_abs_float(int_name: string; g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var args <- [{auto[] arguments[0]}]
    var argTypes <- [{auto type_to_llvm_abi_type(opType)}]
    var id = LLVMLookupIntrinsicID(int_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {int_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))

def intrinsic_math_abs_int(abs_name: string; g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    var args <- [{auto[] arguments[0]; LLVMConstInt(LLVMInt1Type(), 0ul, 0)}]
    var argTypes <- [{auto type_to_llvm_abi_type(opType)}]
    var id = LLVMLookupIntrinsicID(abs_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed_E(expr, "missing intrinsic {abs_name}")
        return null
    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; LLVMInt1Type()}]) // type : (a, LLVMInt1Type) -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))

def intrinsic_math_saturate(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    if opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
        var v_0 = LLVMConstReal(LLVMFloatType(), double(0.))
        var v_1 = LLVMConstReal(LLVMFloatType(), double(1.))
        if !opType.isFloatOrDouble
            v_0 = expand_scalar(g_builder, v_0, opType)
            v_1 = expand_scalar(g_builder, v_1, opType)
        var cond_max = LLVMBuildFCmp(g_builder, LLVMRealPredicate LLVMRealOGT, arguments[0], v_0, "")
        var val_max  = LLVMBuildSelect( g_builder, cond_max, arguments[0], v_0, "")
        var cond_min = LLVMBuildFCmp(g_builder, LLVMRealPredicate LLVMRealOLT, val_max, v_1, "")
        return LLVMBuildSelect( g_builder, cond_min, val_max, v_1, "saturate")
    else
        failed_E(expr, "missing intrinsic {expr.name}({describe(opType)})")
        return null

def build_fadd ( g_builder:LLVMOpaqueBuilder?; opType:TypeDeclPtr; v2:LLVMOpaqueValue?; name:string ) : LLVMOpaqueValue?
    let fadd_name = "llvm.vector.reduce.fadd.v{opType.vectorDim}f32"
    let neg_0 = LLVMConstReal(LLVMFloatType(), -double(0.))
    var args_fadd <- [{auto[] neg_0; v2}]
    var argTypes_fadd <- [{auto type_to_llvm_abi_type(opType)}]
    var id = LLVMLookupIntrinsicID(fadd_name)
    var decl_fadd = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes_fadd)
    if decl_fadd==null
        failed("missing intrinsic {fadd_name}")
        return null
    var typ = LLVMFunctionType(LLVMFloatType(), [{auto LLVMFloatType(); type_to_llvm_abi_type(opType)}])
    return LLVMBuildCall2(g_builder, typ, decl_fadd, args_fadd, name)

def build_fsqrt ( g_builder:LLVMOpaqueBuilder?; v2:LLVMOpaqueValue?; name:string ) : LLVMOpaqueValue?
    let fsqrt_name = "llvm.sqrt.f32"
    var args_fsqrt <- [{auto[] v2}]
    var argTypes_fsqrt <- [{auto LLVMFloatType()}]
    var id = LLVMLookupIntrinsicID(fsqrt_name)
    var decl_fsqrt = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes_fsqrt)
    if decl_fsqrt==null
        failed("missing intrinsic {fsqrt_name}")
        return null
    var typ = LLVMFunctionType(LLVMFloatType(), argTypes_fsqrt)
    return LLVMBuildCall2(g_builder, typ, decl_fsqrt, args_fsqrt, name)

def build_frcp ( g_builder:LLVMOpaqueBuilder?; v2:LLVMOpaqueValue?; name:string ) : LLVMOpaqueValue?
    var one = LLVMConstReal(LLVMFloatType(), 1.0lf)
    var fdiv = LLVMBuildFDiv(g_builder, one, v2, "")
    return fdiv

def intrinsic_math_length_sq(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(g_builder, arguments[0], arguments[0], "")
    return build_fadd(g_builder, opType, v2, "length_sq")

def intrinsic_math_dot(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(g_builder, arguments[0], arguments[1], "")
    return build_fadd(g_builder, opType, v2, "dot")

def intrinsic_math_normalize(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(g_builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(g_builder, opType, v2, "")
    let len = build_fsqrt(g_builder, len_sq, "")

    let zero_val = LLVMConstNull(LLVMFloatType())
    let is_zero = g_builder |> LLVMBuildFCmp(LLVMRealPredicate LLVMRealOEQ, len_sq, zero_val, "isVecZero")

    let ffunc       = LLVMGetBasicBlockParent(LLVMGetInsertBlock(g_builder))
    let if_zero     = LLVMAppendBasicBlock(ffunc, "if.zero")
    let if_not_zero = LLVMAppendBasicBlock(ffunc, "if.notzero")
    let merge_bb    = LLVMAppendBasicBlock(ffunc, "merge")
    LLVMBuildCondBr(g_builder, is_zero, if_zero, if_not_zero)

    LLVMPositionBuilderAtEnd(g_builder, if_zero)
    let zero_vec = build_broadcast_vector(g_builder, opType, zero_val)
    LLVMBuildBr(g_builder, merge_bb)

    LLVMPositionBuilderAtEnd(g_builder, if_not_zero)
    let ilen     = build_frcp(g_builder, len, "")
    let vilen    = build_broadcast_vector(g_builder, opType, ilen)
    let norm_vec = LLVMBuildFMul(g_builder, arguments[0], vilen, "normalize")
    LLVMBuildBr(g_builder, merge_bb)

    LLVMPositionBuilderAtEnd(g_builder, merge_bb)
    let phi = LLVMBuildPhi(g_builder, type_to_llvm_type(opType), "result")
    LLVMAddIncoming(phi, [[auto norm_vec; zero_vec]], [[auto if_not_zero; if_zero]])

    return phi

def intrinsic_math_cross(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    //! a.yzx * b.zxy - a.zxy * b.yzx
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType && opType.vectorDim==3)
    let a = arguments[0]
    let b = arguments[1]
    var a_yzx = LLVMBuildShuffleVector(g_builder, a, a, [{auto 1;2;0;-1}], "a_yzx")
    var b_zxy = LLVMBuildShuffleVector(g_builder, b, b, [{auto 2;0;1;-1}], "b_zxy")
    var a_yzx_b_zxy = LLVMBuildFMul(g_builder, a_yzx, b_zxy, "a_yzx_b_zxy")
    var a_zxy = LLVMBuildShuffleVector(g_builder, a, a, [{auto 2;0;1;-1}], "a_zxy")
    var b_yzx = LLVMBuildShuffleVector(g_builder, b, b, [{auto 1;2;0;-1}], "b_yzx")
    var a_zxy_b_yzx = LLVMBuildFMul(g_builder, a_zxy, b_yzx, "a_zxy_b_yzx")
    var cross = LLVMBuildFSub(g_builder, a_yzx_b_zxy, a_zxy_b_yzx, "cross")
    return LLVMBuildShuffleVector(g_builder, cross, cross, [{auto 0;1;2}], "cross")

def intrinsic_math_inv_distance_sq(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v = LLVMBuildFSub(g_builder, arguments[0], arguments[1], "")
    let v2 = LLVMBuildFMul(g_builder, v, v, "")
    let distance_sq = build_fadd(g_builder, opType, v2, "")
    return LLVMBuildFDiv(g_builder, LLVMConstReal(LLVMFloatType(), 1.0lf), distance_sq, "inv_distance_sq")

def intrinsic_math_inv_length(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(g_builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(g_builder, opType, v2, "")
    let len = build_fsqrt(g_builder, len_sq, "")
    return build_frcp(g_builder, len, "inv_length")

def intrinsic_das_ptr_set_add(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var value_stride = LLVMBuildMul(g_builder, arguments[1], arguments[2], "")
    var ptr_value = LLVMBuildLoad2(g_builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "")
    var ptr_value_i8 = LLVMBuildPointerCast(g_builder, ptr_value, LLVMPointerType(LLVMInt8Type(), 0u), "")
    var ptr_value_stride = LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr_value_i8, value_stride, "")
    var vptr_value_stride = LLVMBuildPointerCast(g_builder, ptr_value_stride, type_to_llvm_type(expr.arguments[0]._type), "")
    return LLVMBuildStore(g_builder, vptr_value_stride, arguments[0])

def intrinsic_das_ptr_add(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var value_stride = LLVMBuildMul(g_builder, arguments[1], arguments[2], "")
    var ptr_value = arguments[0]
    var ptr_value_i8 = LLVMBuildPointerCast(g_builder, ptr_value, LLVMPointerType(LLVMInt8Type(), 0u), "")
    var ptr_value_stride = LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr_value_i8, value_stride, "")
    return LLVMBuildPointerCast(g_builder, ptr_value_stride, type_to_llvm_type(expr.arguments[0]._type), "")

def intrinsic_memset8(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    return LLVMBuildMemSet(g_builder, arguments[0], arguments[1], arguments[2], 1u)

def build_cmove_nz(g_builder:LLVMOpaqueBuilder?; val:LLVMOpaqueValue?; blk:block )
    let cb = LLVMGetInsertBlock(g_builder)
    var write = LLVMInsertBasicBlock(cb, "write")
    var skip = LLVMInsertBasicBlock(cb, "skip")
    var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, val, LLVMConstInt(LLVMInt64Type(), 0ul, 0), "")
    LLVMBuildCondBr(g_builder, cond, write, skip)
    LLVMPositionBuilderAtEnd(g_builder, write)
    invoke(blk)
    LLVMBuildBr(g_builder, skip)
    LLVMPositionBuilderAtEnd(g_builder, skip)

def build_write_n_values(g_builder:LLVMOpaqueBuilder?; iptr,value,ival,isize:LLVMOpaqueValue?; count:int)
    var ptr = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMInt4Type(),0u), iptr, "")
    for i in range(count)
        LLVMSetAlignment(LLVMBuildStore(g_builder, value, ptr),4u)
        ptr = LLVMBuildGEP2(g_builder, LLVMInt4Type(), ptr, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    LLVMBuildStore(g_builder, LLVMBuildSub(g_builder, ival, LLVMConstInt(LLVMInt64Type(), uint64(count), 0), ""), isize)
    LLVMBuildStore(g_builder, ptr, iptr)

def build_memset_128(g_builder:LLVMOpaqueBuilder?; _ptr,value,_size:LLVMOpaqueValue? ) : LLVMOpaqueValue?
    var isize = LLVMBuildAlloca(g_builder, LLVMInt64Type(), "isize")
    LLVMSetAlignment(isize, 8u)
    LLVMBuildStore(g_builder, _size, isize)
    var iptr = LLVMBuildAlloca(g_builder, LLVMPointerType(LLVMInt4Type(),0u), "iptr")
    LLVMSetAlignment(iptr, 8u)
    LLVMBuildStore(g_builder, _ptr, iptr)
// if ( size & 1 ) write 1 value
    var ival = LLVMBuildLoad2(g_builder, LLVMInt64Type(), isize, "")
    var size_and_1 = LLVMBuildAnd(g_builder, ival, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    build_cmove_nz(g_builder, size_and_1) <|
        build_write_n_values(g_builder, iptr,value,ival,isize,1)
// if ( size & 3 ) write 2 values
    ival = LLVMBuildLoad2(g_builder, LLVMInt64Type(), isize, "")
    var size_and_3 = LLVMBuildAnd(g_builder, ival, LLVMConstInt(LLVMInt64Type(), 3ul, 0), "")
    build_cmove_nz(g_builder, size_and_3) <|
        build_write_n_values(g_builder, iptr,value,ival,isize,2)
// if ( size & 7 ) write 4 values
    ival = LLVMBuildLoad2(g_builder, LLVMInt64Type(), isize, "")
    var size_and_7 = LLVMBuildAnd(g_builder, ival, LLVMConstInt(LLVMInt64Type(), 7ul, 0), "")
    build_cmove_nz(g_builder, size_and_7) <|
        build_write_n_values(g_builder, iptr,value,ival,isize,4)
// loop body
    let cb = LLVMGetInsertBlock(g_builder)
    var loop = LLVMInsertBasicBlock(cb, "loop")
    var loop_cont = LLVMInsertBasicBlock(cb, "loop_cont")
    var exit = LLVMInsertBasicBlock(loop, "exit")
    var izero = LLVMConstInt(LLVMInt64Type(), 0ul, 0)
// loop:
    LLVMBuildBr(g_builder, loop)
    LLVMPositionBuilderAtEnd(g_builder, loop)
// while size
    ival = LLVMBuildLoad2(g_builder, LLVMInt64Type(), isize, "")
    var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, ival, izero, "")
    LLVMBuildCondBr(g_builder, cond, loop_cont, exit)
    LLVMPositionBuilderAtEnd(g_builder, loop_cont)
// ptr[i] = values
    build_write_n_values(g_builder, iptr,value,ival,isize,8)
// goto loop
    LLVMBuildBr(g_builder, loop)
// end:
    LLVMPositionBuilderAtEnd(g_builder, exit)
    return ival

def build_write_if_cond(g_builder:LLVMOpaqueBuilder?; cond,ptr,value:LLVMOpaqueValue? )
    let cb = LLVMGetInsertBlock(g_builder)
    var write = LLVMInsertBasicBlock(cb, "write")
    var skip = LLVMInsertBasicBlock(cb, "skip")
// if cond
    LLVMBuildCondBr(g_builder, cond, write, skip)
// write:
    LLVMPositionBuilderAtEnd(g_builder, write)
    LLVMBuildStore(g_builder, value, ptr)
// *ptr = value
    LLVMBuildBr(g_builder, skip)
// skip:
    LLVMPositionBuilderAtEnd(g_builder, skip)

def intrinsic_memset128(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    var size = LLVMBuildZExtOrBitCast(g_builder, arguments[2], LLVMInt64Type(), "")
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt4Type(),0u), "")
    return build_memset_128(g_builder, ptr, arguments[1], size)

def build_memset_skip(g_builder:LLVMOpaqueBuilder?; ptr_type:LLVMOpaqueType?; _ptr,val,_size:LLVMOpaqueValue? )
    var ptr = _ptr
    var size = _size
// cond
    var size_and_1 = LLVMBuildAnd(g_builder, size, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, size_and_1, LLVMConstInt(LLVMInt64Type(), 0ul, 0), "")
// if ( size & 1 ) write 1 value
    build_write_if_cond(g_builder, cond, ptr, val)
// ptr += (size & 1)
    var ptr_add_1 = LLVMBuildGEP2(g_builder, ptr_type, ptr, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    ptr = LLVMBuildSelect(g_builder, cond, ptr_add_1, ptr, "")
// size &= ~1
    var size_sub_1 = LLVMBuildSub(g_builder, size, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    size = LLVMBuildSelect(g_builder, cond, size_sub_1, size, "")
// half size
    size = LLVMBuildLShr(g_builder, size, LLVMConstInt(LLVMInt64Type(), 1ul, 0), "")
    return [[auto ptr,size]]

def build_memset_64(g_builder:LLVMOpaqueBuilder?; ptr_type:LLVMOpaqueType?; _ptr,val,_size:LLVMOpaqueValue? ) : LLVMOpaqueValue?
    var ps = build_memset_skip(g_builder, ptr_type, _ptr,val,_size)
    var ptr = LLVMBuildPointerCast(g_builder, ps._0, LLVMPointerType(LLVMInt4Type(),0u), "")
    var size = ps._1
    var value = LLVMGetUndef(LLVMVectorType(LLVMInt64Type(), 2u))
    value = LLVMBuildInsertElement(g_builder, value, val, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    value = LLVMBuildInsertElement(g_builder, value, val, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    value = LLVMBuildBitCast(g_builder, value, LLVMInt4Type(), "")
    return build_memset_128(g_builder, ptr, value, size)

def intrinsic_memset64(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var size = LLVMBuildZExtOrBitCast(g_builder, arguments[2], LLVMInt64Type(), "")
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt64Type(),0u), "")
    return build_memset_64(g_builder, LLVMInt64Type(), ptr, arguments[1], size)

def build_memset_32(g_builder:LLVMOpaqueBuilder?; ptr_type:LLVMOpaqueType?; _ptr,val,_size:LLVMOpaqueValue? ) : LLVMOpaqueValue?
    var ps = build_memset_skip(g_builder, ptr_type, _ptr,val,_size)
    var ptr = LLVMBuildPointerCast(g_builder, ps._0, LLVMPointerType(LLVMInt64Type(),0u), "")
    var size = ps._1
    var value = LLVMBuildZExtOrBitCast(g_builder, val, LLVMInt64Type(), "")
    value = LLVMBuildOr(g_builder, value, LLVMBuildShl(g_builder, value, LLVMConstInt(LLVMInt64Type(), 32ul, 0), ""), "")
    return build_memset_64(g_builder, LLVMInt64Type(), ptr, value, size)

def intrinsic_memset32(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var size = LLVMBuildZExtOrBitCast(g_builder, arguments[2], LLVMInt64Type(), "")
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt32Type(),0u), "")
    return build_memset_32(g_builder, LLVMInt32Type(), ptr, arguments[1], size)

def build_memset_16(g_builder:LLVMOpaqueBuilder?; ptr_type:LLVMOpaqueType?; _ptr,val,_size:LLVMOpaqueValue? ) : LLVMOpaqueValue?
    var ps = build_memset_skip(g_builder, ptr_type, _ptr,val,_size)
    var ptr = LLVMBuildPointerCast(g_builder, ps._0, LLVMPointerType(LLVMInt32Type(),0u), "")
    var size = ps._1
    var value = LLVMBuildZExtOrBitCast(g_builder, val, LLVMInt32Type(), "")
    value = LLVMBuildOr(g_builder, value, LLVMBuildShl(g_builder, value, LLVMConstInt(LLVMInt32Type(), 16ul, 0), ""), "")
    return build_memset_32(g_builder, LLVMInt32Type(), ptr, value, size)

def intrinsic_memset16(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var size = LLVMBuildZExtOrBitCast(g_builder, arguments[2], LLVMInt64Type(), "")
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt16Type(),0u), "")
    return build_memset_16(g_builder, LLVMInt16Type(), ptr, arguments[1], size)

def intrinsic_math_gather(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt32Type(),0u), "")
    var ix = LLVMBuildExtractElement(g_builder, arguments[1], LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    var iy = LLVMBuildExtractElement(g_builder, arguments[1], LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    var iz = LLVMBuildExtractElement(g_builder, arguments[1], LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    var iw = LLVMBuildExtractElement(g_builder, arguments[1], LLVMConstInt(LLVMInt32Type(), 3ul, 0), "")
    var xaddr = LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, ix, "")
    var yaddr = LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iy, "")
    var zaddr = LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iz, "")
    var waddr = LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iw, "")
    var res = LLVMGetUndef(LLVMInt4Type())
    res = LLVMBuildInsertElement(g_builder, res, LLVMBuildLoad2(g_builder, LLVMInt32Type(), xaddr, ""), LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    res = LLVMBuildInsertElement(g_builder, res, LLVMBuildLoad2(g_builder, LLVMInt32Type(), yaddr, ""), LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    res = LLVMBuildInsertElement(g_builder, res, LLVMBuildLoad2(g_builder, LLVMInt32Type(), zaddr, ""), LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    res = LLVMBuildInsertElement(g_builder, res, LLVMBuildLoad2(g_builder, LLVMInt32Type(), waddr, ""), LLVMConstInt(LLVMInt32Type(), 3ul, 0), "")
    return res

def intrinsic_math_store_neq_mask(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var ptr = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt4Type(),0u), "")
    var value = arguments[1]
    var mask_v = arguments[2]
    var mask = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, value, mask_v, "")
    var dest = LLVMBuildLoad2(g_builder, LLVMInt4Type(), ptr, "")
    LLVMSetAlignment(dest, 4u)
    var res = LLVMBuildSelect(g_builder, mask, dest, value, "")
    var wrt = LLVMBuildStore(g_builder, res, ptr)
    LLVMSetAlignment(wrt, 4u)
    return res

def intrinsic_math_gather_store_mask(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var arg_gather <- [{auto arguments[1]; arguments[2]}]
    var gather = intrinsic_math_gather(g_builder, expr, arg_gather)
    var arg_store_mask <- [{auto arguments[0]; gather; arguments[3]}]
    return intrinsic_math_store_neq_mask(g_builder, expr, arg_store_mask)

def intrinsic_math_gather_store_stride(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    // gather
    var ptr = LLVMBuildPointerCast(g_builder, arguments[2], LLVMPointerType(LLVMInt32Type(),0u), "")
    var ix = LLVMBuildExtractElement(g_builder, arguments[3], LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    var iy = LLVMBuildExtractElement(g_builder, arguments[3], LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    var iz = LLVMBuildExtractElement(g_builder, arguments[3], LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    var iw = LLVMBuildExtractElement(g_builder, arguments[3], LLVMConstInt(LLVMInt32Type(), 3ul, 0), "")
    var X = LLVMBuildLoad2(g_builder, LLVMInt32Type(), LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, ix, ""), "")
    var Y = LLVMBuildLoad2(g_builder, LLVMInt32Type(), LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iy, ""), "")
    var Z = LLVMBuildLoad2(g_builder, LLVMInt32Type(), LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iz, ""), "")
    var W = LLVMBuildLoad2(g_builder, LLVMInt32Type(), LLVMBuildGEP2(g_builder, LLVMInt32Type(), ptr, iw, ""), "")
    // write it out
    var stride = arguments[1]
    var out = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt32Type(),0u), "")
    LLVMBuildStore(g_builder, X, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt32Type(), out, stride, "")
    LLVMBuildStore(g_builder, Y, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt32Type(), out, stride, "")
    LLVMBuildStore(g_builder, Z, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt32Type(), out, stride, "")
    LLVMBuildStore(g_builder, W, out)
    return out


/*
    __forceinline void v_scatter ( void * _ptr, vec4f index, vec4f value ) {
        // write 4 floats to memory, using 4 uint32_t indices
        auto ptr = (float *) _ptr;
        auto i = v_cast_vec4i(index);
        ptr[uint32_t(v_extract_xi(i))] = v_extract_x(value);
        ptr[uint32_t(v_extract_yi(i))] = v_extract_y(value);
        ptr[uint32_t(v_extract_zi(i))] = v_extract_z(value);
        ptr[uint32_t(v_extract_wi(i))] = v_extract_w(value);
    }
*/

/*
    __forceinline void v_scatter_mask ( void * _ptr, vec4f index, vec4f value, vec4f mask_v ) {
        // write 4 floats to memory, using 4 uint32_t indices, but only for floats, where value[i]!=mask_v[i]
        auto ptr = (float *) _ptr;
        auto mask = v_cmp_eqi(v_cast_vec4i(mask_v), v_cast_vec4i(value));
        auto i = v_cast_vec4i(index);
        if ( !v_extract_xi(mask) ) ptr[uint32_t(v_extract_xi(i))] = v_extract_x(value);
        if ( !v_extract_yi(mask) ) ptr[uint32_t(v_extract_yi(i))] = v_extract_y(value);
        if ( !v_extract_zi(mask) ) ptr[uint32_t(v_extract_zi(i))] = v_extract_z(value);
        if ( !v_extract_wi(mask) ) ptr[uint32_t(v_extract_wi(i))] = v_extract_w(value);
    }
*/

// gather_store_neq_mask
// def intrinsic_gather_store_neq_mask

def intrinsic_math_u8x4_gather_store(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    // gather
    var ptr = LLVMBuildPointerCast(g_builder, arguments[1], LLVMPointerType(LLVMInt8Type(),0u), "")
    var ix = LLVMBuildExtractElement(g_builder, arguments[2], LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    var iy = LLVMBuildExtractElement(g_builder, arguments[2], LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    var iz = LLVMBuildExtractElement(g_builder, arguments[2], LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    var iw = LLVMBuildExtractElement(g_builder, arguments[2], LLVMConstInt(LLVMInt32Type(), 3ul, 0), "")
    var X = LLVMBuildLoad2(g_builder, LLVMInt8Type(), LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr, ix, ""), "")
    var Y = LLVMBuildLoad2(g_builder, LLVMInt8Type(), LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr, iy, ""), "")
    var Z = LLVMBuildLoad2(g_builder, LLVMInt8Type(), LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr, iz, ""), "")
    var W = LLVMBuildLoad2(g_builder, LLVMInt8Type(), LLVMBuildGEP2(g_builder, LLVMInt8Type(), ptr, iw, ""), "")
    // write it out
    var stride = LLVMConstInt(LLVMInt32Type(), 1ul, 0)
    var out = LLVMBuildPointerCast(g_builder, arguments[0], LLVMPointerType(LLVMInt8Type(),0u), "")
    LLVMBuildStore(g_builder, X, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt8Type(), out, stride, "")
    LLVMBuildStore(g_builder, Y, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt8Type(), out, stride, "")
    LLVMBuildStore(g_builder, Z, out)
    out = LLVMBuildGEP2(g_builder, LLVMInt8Type(), out, stride, "")
    LLVMBuildStore(g_builder, W, out)
    return out

def intrinsic_math_roundi(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var res = intrinsic_math_any_float_op1("round", g_builder, expr, arguments)
    return LLVMBuildFPToSI(g_builder, res, type_to_llvm_abi_type(expr._type), "")

def intrinsic_math_trunci(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var res = intrinsic_math_any_float_op1("trunc", g_builder, expr, arguments)
    return LLVMBuildFPToSI(g_builder, res, type_to_llvm_abi_type(expr._type), "")

def intrinsic_math_sign(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    assume resType = expr._type
    if resType.isSignedInteger || (resType.isVectorType && resType.vectorBaseType==Type tInt)
        var itype = base_type_to_llvm_type(resType.isSignedInteger ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector (g_builder, resType, LLVMConstInt(itype, 0ul, 0))
        var one = build_broadcast_vector (g_builder, resType, LLVMConstInt(itype, 1ul, 0))
        var minus_one = build_broadcast_vector (g_builder, resType, LLVMConstInt(itype, uint64(-1), 0))
        var cond_z = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, arguments[0], zero, "")
        var cond_gt = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntSGT, arguments[0], zero, "")
        var res = LLVMBuildSelect(g_builder, cond_gt, one, minus_one, "")
        res = LLVMBuildSelect(g_builder, cond_z, zero, res, "")
        return res
    elif resType.isUnsignedInteger || (resType.isVectorType && resType.vectorBaseType==Type tUInt)
        var itype = base_type_to_llvm_type(resType.isUnsignedInteger ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector (g_builder, resType, LLVMConstInt(itype, 0ul, 0))
        var one = build_broadcast_vector (g_builder, resType, LLVMConstInt(itype, 1ul, 0))
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, arguments[0], zero, "")
        var res = LLVMBuildSelect(g_builder, cond, zero, one, "")
        return res
    elif resType.isFloatOrDouble || (resType.isVectorType && resType.vectorBaseType==Type tFloat)
        var ftype = base_type_to_llvm_type(resType.isFloatOrDouble ? resType.baseType : resType.vectorBaseType)
        var zero = build_broadcast_vector (g_builder, resType, LLVMConstReal(ftype, 0.0lf))
        var one = build_broadcast_vector (g_builder, resType, LLVMConstReal(ftype, 1.0lf))
        var minus_one = build_broadcast_vector (g_builder, resType, LLVMConstReal(ftype, -1.0lf))
        var cond_z = LLVMBuildFCmp(g_builder, LLVMRealPredicate LLVMRealOEQ, arguments[0], zero, "")
        var cond_gt = LLVMBuildFCmp(g_builder, LLVMRealPredicate LLVMRealOGT, arguments[0], zero, "")
        var res = LLVMBuildSelect(g_builder, cond_gt, one, minus_one, "")
        res = LLVMBuildSelect(g_builder, cond_z, zero, res, "")
        return res
    else
        failed("sign({describe(resType)}) is not supported (yet?)")
        return null

def intrinsic_mul_128(g_builder:LLVMOpaqueBuilder?; expr:smart_ptr<ExprCallFunc>; arguments:array<LLVMOpaqueValue?>) : LLVMOpaqueValue?
    var a = arguments[0]
    var b = arguments[1]
    var A = LLVMBuildZExtOrBitCast(g_builder, a, LLVMInt128Type(), "")
    var B = LLVMBuildZExtOrBitCast(g_builder, b, LLVMInt128Type(), "")
    var ab = LLVMBuildMul(g_builder, A, B, "")
    let lo = LLVMBuildTrunc(g_builder, ab, LLVMInt64Type(), "")
    var hi = LLVMBuildLShr(g_builder, ab, LLVMConstInt(LLVMInt128Type(), 64ul, 0), "")
    hi = LLVMBuildTrunc(g_builder, hi, LLVMInt64Type(), "")
    var value = LLVMGetUndef(LLVMRange64Type())
    value = LLVMBuildInsertElement(g_builder, value, lo, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    value = LLVMBuildInsertElement(g_builder, value, hi, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    return value
