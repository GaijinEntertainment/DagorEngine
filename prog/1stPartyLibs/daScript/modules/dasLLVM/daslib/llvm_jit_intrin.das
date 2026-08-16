options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const
options unsafe_table_lookup = false
// intrinsic emitters carry per-function lowering contracts (gates, exactness preconditions,
// saturation bounds) that don't compress to the 3-line cap — keep them at the emitters
options _comment_hygiene = false

module llvm_jit_intrin shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_common
require daslib/ast_boost
require strings

struct public JitCtx {
    ctx : LLVMContextRef
    types : PrimitiveTypes -const? -const
    builder : LLVMOpaqueBuilder -const? -const
}

// Set by llvm_macro before codegen when prog.policies.jit_exe_mode is true.
// Read by intrinsic_is_standalone_exe to fold $::is_standalone_exe() into a constant.
var public g_jit_exe_mode = false

let g_intrin_lookup <- {
// $, aka builtin
    // misc
        "$::length" => @@intrinsic_builtin_length,
        "$::empty" => @@intrinsic_builtin_empty,
        "$::is_standalone_exe" => @@intrinsic_is_standalone_exe,
        "$::hash" => @@intrinsic_builtin_hash_string,
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
    // 16/8-bit int-lattice c-tors + converts (fp16 family lives in the gated table below)
        "$::short2" => @@intrinsic_builtin_lattice_vec,
        "$::short3" => @@intrinsic_builtin_lattice_vec,
        "$::short4" => @@intrinsic_builtin_lattice_vec,
        "$::short8" => @@intrinsic_builtin_lattice_vec,
        "$::ushort2" => @@intrinsic_builtin_lattice_vec,
        "$::ushort3" => @@intrinsic_builtin_lattice_vec,
        "$::ushort4" => @@intrinsic_builtin_lattice_vec,
        "$::ushort8" => @@intrinsic_builtin_lattice_vec,
        "$::byte2" => @@intrinsic_builtin_lattice_vec,
        "$::byte3" => @@intrinsic_builtin_lattice_vec,
        "$::byte4" => @@intrinsic_builtin_lattice_vec,
        "$::byte8" => @@intrinsic_builtin_lattice_vec,
        "$::byte16" => @@intrinsic_builtin_lattice_vec,
        "$::ubyte2" => @@intrinsic_builtin_lattice_vec,
        "$::ubyte3" => @@intrinsic_builtin_lattice_vec,
        "$::ubyte4" => @@intrinsic_builtin_lattice_vec,
        "$::ubyte8" => @@intrinsic_builtin_lattice_vec,
        "$::ubyte16" => @@intrinsic_builtin_lattice_vec,
    // the idot family (integer dots on the 8-bit lattice) + the byte table select
        "$::idot4" => @@intrinsic_lattice_idot4,
        "$::idot" => @@intrinsic_lattice_idot,
        "$::shuffle" => @@intrinsic_lattice_shuffle,
    // saturating narrows (clamp+trunc — backends select sqxtn/packs natively)
        "$::short2_sat" => @@intrinsic_builtin_lattice_sat,
        "$::short3_sat" => @@intrinsic_builtin_lattice_sat,
        "$::short4_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ushort2_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ushort3_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ushort4_sat" => @@intrinsic_builtin_lattice_sat,
        "$::byte2_sat" => @@intrinsic_builtin_lattice_sat,
        "$::byte3_sat" => @@intrinsic_builtin_lattice_sat,
        "$::byte4_sat" => @@intrinsic_builtin_lattice_sat,
        "$::byte8_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ubyte2_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ubyte3_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ubyte4_sat" => @@intrinsic_builtin_lattice_sat,
        "$::ubyte8_sat" => @@intrinsic_builtin_lattice_sat,
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
    "math::exp" => @@intrinsic_math_exp,
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
    "math::hmin" => @@intrinsic_math_hminmax,
    "math::hmax" => @@intrinsic_math_hminmax,
    "math::hadd" => @@intrinsic_math_hadd,
    "math::clamp" => @@intrinsic_math_clamp,
    "math::step" => @@intrinsic_math_step,
    "math::smoothstep" => @@intrinsic_math_smoothstep,
    "math::radians" => @@intrinsic_math_radians,
    "math::degrees" => @@intrinsic_math_degrees,
    "math::fmod" => @@intrinsic_math_fmod,
    "math::sinh" => @@intrinsic_math_sinh_cosh_tanh,
    "math::cosh" => @@intrinsic_math_sinh_cosh_tanh,
    "math::tanh" => @@intrinsic_math_sinh_cosh_tanh,
    "math::round" => @@intrinsic_math_float_op1,
    "math::floor" => @@intrinsic_math_float_op1,
    "math::ceil" => @@intrinsic_math_float_op1,
    "math::floori" => @@intrinsic_math_float_op1_to_int,
    "math::ceili" => @@intrinsic_math_float_op1_to_int,
    "math::roundi" => @@intrinsic_math_roundi,
    "math::trunci" => @@intrinsic_math_trunci,
    "math::length" => @@intrinsic_math_length,
    "math::length_sq" => @@intrinsic_math_length_sq,
    "math::dot" => @@intrinsic_math_dot,
    "math::cross" => @@intrinsic_math_cross,
    "math::inv_distance_sq" => @@intrinsic_math_inv_distance_sq,
    "math::inv_length" => @@intrinsic_math_inv_length,
    "math::fast_normalize" => @@intrinsic_math_fast_normalize,
    "math::normalize" => @@intrinsic_math_normalize
}

// AArch64 NEON intrinsics (the llvm/aarch64_neon header), consulted only when the JIT target is
// aarch64. On any other target has_intrinsic returns false for these keys and the call's own
// daslang body — the portable fallback in the declaring module — compiles as a normal function.
// That silent degrade-to-fallback contract holds for every table in this section.
let g_aarch64_intrin_lookup <- {
    "aarch64_neon::sdot4" => @@intrinsic_sdot4,
    "aarch64_neon::sdot4_laneq" => @@intrinsic_sdot4_laneq,
    "aarch64_neon::sdot4_w" => @@intrinsic_sdot4_w,
    "aarch64_neon::sdot4_laneq_w" => @@intrinsic_sdot4_laneq_w,
    "aarch64_neon::tbl16_lo" => @@intrinsic_tbl16,
    "aarch64_neon::tbl16_hi" => @@intrinsic_tbl16,
    "aarch64_neon::smmla" => @@intrinsic_smmla
}

// x86-64 intrinsics (the llvm/x64_avx header) — same fallback contract, gated on g_target_x64_avx2:
// NOT just the arch, because AVX2 is not universal on x86-64 and emitting llvm.x86.avx2.* against a
// target without +avx2 is a fatal "Cannot select" at codegen. Exactness precondition of the dot
// family (x in [-127,127], the Q8 quantizer's range) is documented at the x64_avx.das declarations.
let g_x64_intrin_lookup <- {
    "x64_avx::dot4" => @@intrinsic_dot4_x64,
    "x64_avx::dot32" => @@intrinsic_dot32_x64,
    "x64_avx::dot32_ps" => @@intrinsic_dot32_ps_x64,
    "x64_avx::dot32_acc8" => @@intrinsic_dot32_acc8_x64,
    "x64_avx::dot32_mx4" => @@intrinsic_dot32_mx4_x64,
    "x64_avx::mx4_dequant32" => @@intrinsic_mx4_dequant32_x64
}

// AVX kernel-matrix tiers — same contract, finer gates (see the g_target_x64_* declarations in
// llvm_jit_common). A name whose gate is false compiles its daslang body, which delegates to the
// tier below (dot32 / dot32_acc8 / mx4_dequant32), so the call degrades to the proven ymm path
// rather than to scalar code.
let g_x64_vnni256_intrin_lookup <- {   // 256-bit VPDPBUSD (VEX AVX-VNNI, or EVEX AVX512-VNNI+VL)
    "x64_avx::dot32_vnni" => @@intrinsic_dot32_vnni_x64,
    "x64_avx::dot32_acc8_vnni" => @@intrinsic_dot32_acc8_vnni_x64,
    "x64_avx::dot32_mx4_vnni" => @@intrinsic_dot32_mx4_vnni_x64
}
let g_x64_avx512bw_intrin_lookup <- {   // zmm byte ops (VPABSB/VPMADDUBSW/VPSHUFB at 512 need BW)
    "x64_avx::dot64_acc16" => @@intrinsic_dot64_acc16_x64,
    "x64_avx::mx4_dequant64" => @@intrinsic_mx4_dequant64_x64
}
let g_x64_avx512vnni_intrin_lookup <- {   // zmm VPDPBUSD (the bw gate is a prerequisite)
    "x64_avx::dot64_acc16_vnni" => @@intrinsic_dot64_acc16_vnni_x64
}

// Half<->float converts (the llvm/daslib/f16_cvt module) — one TARGET-INDEPENDENT table, consulted
// when the target lowers fpext/fptrunc through half in hardware: any aarch64 (scalar fcvt is
// ARMv8.0 baseline, no finer gate) or x86-64 with F16C (g_target_x64_f16c). Elsewhere the calls
// compile their exact bit-twiddle daslang bodies — a half convert without hardware support would
// lower to a compiler-rt libcall the JIT doesn't link, so the gate is emission-correctness, not
// just speed.
let g_f16_cvt_intrin_lookup <- {
    "f16_cvt::f16_to_f32" => @@intrinsic_f16_to_f32,
    "f16_cvt::f32_to_f16" => @@intrinsic_f32_to_f16
}

// fp16-family lattice ctors/converts — same hardware-convert gate as g_f16_cvt above.
// (float2/3/4-from-half and float-from-float16 ride the unconditional ctor entries; the
// fp16-crossing guard in has_intrinsic keeps those off targets without hardware converts.)
let g_f16_lattice_intrin_lookup <- {
    "$::float16" => @@intrinsic_builtin_float16,
    "$::half2" => @@intrinsic_builtin_lattice_vec,
    "$::half3" => @@intrinsic_builtin_lattice_vec,
    "$::half4" => @@intrinsic_builtin_lattice_vec,
    "$::half8" => @@intrinsic_builtin_lattice_vec,
    "$::half8_lo" => @@intrinsic_half8_lo_hi,
    "$::half8_hi" => @@intrinsic_half8_lo_hi
}

def public has_intrinsic(expr : ExprCallFunc?) {
    var result = false
    assume argType = expr.arguments[0]._type
    let call_name = "{expr.func._module.name}::{expr.func.name}"

    // Name match alone is not enough — argument types narrow it: $::length only handles arrays
    // (not table<...> or dasvector wrappers), intrinsics never take handle types (int64(clock)),
    // and $::hash inlines only for string keys (build_string_hash matches the host runtime's
    // fast/safe round by is_safe_hash()). lookup_intrinsic asserts the residual consistency.
    if ((call_name == "$::length" && !argType.isGoodArrayType)
        || (call_name == "$::empty" && !(argType.isGoodArrayType || argType.isGoodTableType))
        || (call_name == "$::hash" && argType.baseType != Type.tString)
        || (!expr.arguments |> empty() && argType.isHandle)) return false
    // fp16-family values crossing ANY intrinsic need hardware half converts — without
    // them fpext/fptrunc through half lower to compiler-rt libcalls the JIT doesn't link
    if (!(g_target_is_aarch64 || g_target_x64_f16c)) {
        return false if (is_f16_small_type(expr._type.baseType))
        for (arg in expr.arguments) {
            return false if (is_f16_small_type(arg._type.baseType))
        }
    }
    g_intrin_lookup |> get(call_name) $(_pfun) {
        result = true
    }
    if (!result && g_target_is_aarch64) {
        g_aarch64_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
    }
    if (!result && g_target_x64_avx2) {
        g_x64_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
    }
    if (!result && g_target_x64_vnni256) {
        g_x64_vnni256_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
    }
    if (!result && g_target_x64_avx512bw) {
        g_x64_avx512bw_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
    }
    if (!result && g_target_x64_avx512vnni) {
        g_x64_avx512vnni_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
    }
    if (!result && (g_target_is_aarch64 || g_target_x64_f16c)) {
        g_f16_cvt_intrin_lookup |> get(call_name) $(_pfun) {
            result = true
        }
        if (!result) {
            g_f16_lattice_intrin_lookup |> get(call_name) $(_pfun) {
                result = true
            }
        }
    }
    return result
}

def public lookup_intinsic(g_ctx : LLVMContextRef; g_builder : LLVMOpaqueBuilder?; var types : PrimitiveTypes?; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var result :  LLVMOpaqueValue?
    let call_name = "{expr.func._module.name}::{expr.func.name}"
    g_intrin_lookup |> get(call_name) $(pfun) {
        result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
    }
    if (result == null && g_target_is_aarch64) {
        g_aarch64_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
    }
    if (result == null && g_target_x64_avx2) {
        g_x64_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
    }
    if (result == null && g_target_x64_vnni256) {
        g_x64_vnni256_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
    }
    if (result == null && g_target_x64_avx512bw) {
        g_x64_avx512bw_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
    }
    if (result == null && g_target_x64_avx512vnni) {
        g_x64_avx512vnni_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
    }
    if (result == null && (g_target_is_aarch64 || g_target_x64_f16c)) {
        g_f16_cvt_intrin_lookup |> get(call_name) $(pfun) {
            result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
        }
        if (result == null) {
            g_f16_lattice_intrin_lookup |> get(call_name) $(pfun) {
                result = pfun |> invoke(JitCtx(ctx = g_ctx, builder = g_builder, types = types), expr, arguments)
            }
        }
    }
    // Check initrinsic consistency
    verify(result != null == has_intrinsic(expr))
    return result
}

// intrinsics, $ aka builtin

def intrinsic_builtin_variant_index(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return LLVMBuildLoad2(ctx.builder, ctx.types.t_int32, arguments[0], "$::variant_index")
}

def intrinsic_builtin_set_variant_index(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return LLVMBuildStore2(ctx.builder, ctx.types, ctx.types.t_int32, arguments[1], arguments[0])
}

def intrinsic_builtin_length(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    assume argType = expr.arguments[0]._type
    if (argType.isGoodArrayType) {
        var arr = LLVMBuildLoad2(ctx.builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "arr")
        var size_i64 = LLVMBuildExtractValue(ctx.builder, arr, uint(JIT_ARRAY.SIZE), "array.size")
        // builtin length() returns int32; Array.size is uint64. Truncate — the >INT_MAX panic
        // matching C++ builtin_array_size is emitted by make_call before this intrinsic runs
        // (it needs basic blocks + the exception rail, which live with the visitor).
        return LLVMBuildTruncOrBitCast(ctx.builder, size_i64, ctx.types.t_int32, "array.size.i32")
    } else {
        return null
    }
}

// empty(array) / empty(table) -> size == 0. SIZE is field 1 in both the Array and Table LLVM
// structs (Table inherits Array), so one body covers both. Returns i1 directly (no truncation).
def intrinsic_builtin_empty(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    assume argType = expr.arguments[0]._type
    if (argType.isGoodArrayType || argType.isGoodTableType) {
        var cont = LLVMBuildLoad2(ctx.builder, type_to_llvm_type(expr.arguments[0]._type), arguments[0], "cont")
        var size_i64 = LLVMBuildExtractValue(ctx.builder, cont, uint(JIT_ARRAY.SIZE), "cont.size")
        return LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, size_i64, LLVMConstInt(ctx.types.t_int64, 0ul, 0), "cont.empty")
    } else {
        return null
    }
}

def intrinsic_is_standalone_exe(var ctx : JitCtx; _expr : ExprCallFunc?; _arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return LLVMConstInt(ctx.types.t_int1, g_jit_exe_mode ? 1ul : 0ul, 0)
}

// Inlined hash(string) -> uint64, reproducing hash_blockz64 (anyhash.h) bit-for-bit:
// FNV-1a over a zero-terminated 16-bit word stream; null -> FNV basis; final clamp
// h<=1 -> prime. is_safe_hash() picks the round body to match the host runtime exactly:
// the fast path does one unaligned 2-byte load per round (over-reading 1 byte past the
// null, as DAS_SAFE_HASH==0 does), the safe path loads byte-by-byte (no over-read, for
// ASAN/DAS_SAFE_HASH==1). Both compute the same value. Shared by the $::hash intrinsic
// and the string-keyed table lookup lowering; for a constant key the optimizer folds the
// whole loop to an immediate (item 4).
def public build_string_hash(var ctx : JitCtx; strPtr : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    return is_safe_hash() ? build_string_hash_safe(ctx, strPtr) : build_string_hash_fast(ctx, strPtr)
}

def build_string_hash_fast(var ctx : JitCtx; strPtr : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let i8ptr = LLVMPointerType(ctx.types.t_int8, 0u)
    let i16ptr = LLVMPointerType(ctx.types.t_int16, 0u)
    var ptr = LLVMBuildPointerCast(ctx.builder, strPtr, i8ptr, "hash.str")
    var basis = LLVMConstInt(ctx.types.t_int64, 14695981039346656037ul, 0)
    var prime = LLVMConstInt(ctx.types.t_int64, 1099511628211ul, 0)
    var one64 = LLVMConstInt(ctx.types.t_int64, 1ul, 0)
    var c256 = LLVMConstInt(ctx.types.t_int64, 256ul, 0)
    var lowmask = LLVMConstInt(ctx.types.t_int64, 255ul, 0)
    var zero64 = LLVMConstInt(ctx.types.t_int64, 0ul, 0)

    var entry = LLVMGetInsertBlock(ctx.builder)
    var ffunc = LLVMGetBasicBlockParent(entry)
    var bb_loop = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.loop")
    var bb_body = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.body")
    var bb_cont = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.cont")
    var bb_merge = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.merge")

    // entry: null string -> basis, else enter loop
    var is_null = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, ptr, LLVMConstPointerNull(i8ptr), "hash.isnull")
    LLVMBuildCondBr(ctx.builder, is_null, bb_merge, bb_loop)

    // loop: v = *(uint16_t*)p (unaligned, zero-extended); if (v & 0xff)==0 break with h
    LLVMPositionBuilderAtEnd(ctx.builder, bb_loop)
    var h_phi = LLVMBuildPhi(ctx.builder, ctx.types.t_int64, "hash.h")
    var p_phi = LLVMBuildPhi(ctx.builder, i8ptr, "hash.p")
    var p16 = LLVMBuildPointerCast(ctx.builder, p_phi, i16ptr, "hash.p16")
    var v16 = LLVMBuildLoad2(ctx.builder, ctx.types.t_int16, p16, "hash.v16")
    LLVMSetAlignment(v16, 1u)
    var v = LLVMBuildZExt(ctx.builder, v16, ctx.types.t_int64, "hash.v")
    var lo = LLVMBuildAnd(ctx.builder, v, lowmask, "hash.lo")
    var lo_is_zero = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, lo, zero64, "hash.lo0")
    LLVMBuildCondBr(ctx.builder, lo_is_zero, bb_merge, bb_body)

    // body: h = (h ^ v) * prime; if v < 0x100 (high byte 0) break with hmul
    LLVMPositionBuilderAtEnd(ctx.builder, bb_body)
    var hx = LLVMBuildXor(ctx.builder, h_phi, v, "hash.hx")
    var hmul = LLVMBuildMul(ctx.builder, hx, prime, "hash.hmul")
    var v_lt_256 = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntULT, v, c256, "hash.vlt")
    LLVMBuildCondBr(ctx.builder, v_lt_256, bb_merge, bb_cont)

    // cont: advance two bytes, loop
    LLVMPositionBuilderAtEnd(ctx.builder, bb_cont)
    var idx2 = LLVMConstInt(ctx.types.t_int64, 2ul, 0)
    var p2 = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, p_phi, idx2, "hash.p2")
    LLVMBuildBr(ctx.builder, bb_loop)

    LLVMAddIncoming(h_phi, [basis, hmul], [entry, bb_cont])
    LLVMAddIncoming(p_phi, [ptr, p2], [entry, bb_cont])

    // merge: pick the hash at whichever exit fired, then clamp h<=1 -> prime
    LLVMPositionBuilderAtEnd(ctx.builder, bb_merge)
    var hres = LLVMBuildPhi(ctx.builder, ctx.types.t_int64, "hash.res")
    LLVMAddIncoming(hres, [basis, h_phi, hmul], [entry, bb_loop, bb_body])
    var clamp = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntULE, hres, one64, "hash.clamp")
    return LLVMBuildSelect(ctx.builder, clamp, prime, hres, "hash")
}

def build_string_hash_safe(var ctx : JitCtx; strPtr : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let i8ptr = LLVMPointerType(ctx.types.t_int8, 0u)
    var ptr = LLVMBuildPointerCast(ctx.builder, strPtr, i8ptr, "hash.str")
    var basis = LLVMConstInt(ctx.types.t_int64, 14695981039346656037ul, 0)
    var prime = LLVMConstInt(ctx.types.t_int64, 1099511628211ul, 0)
    var one64 = LLVMConstInt(ctx.types.t_int64, 1ul, 0)
    var zero8 = LLVMConstInt(ctx.types.t_int8, 0ul, 0)

    var entry = LLVMGetInsertBlock(ctx.builder)
    var ffunc = LLVMGetBasicBlockParent(entry)
    var bb_loop = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.loop")
    var bb_body = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.body")
    var bb_cont = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.cont")
    var bb_merge = LLVMAppendBasicBlockInContext(ctx.ctx, ffunc, "hash.merge")

    // entry: null string -> basis, else enter loop
    var is_null = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, ptr, LLVMConstPointerNull(i8ptr), "hash.isnull")
    LLVMBuildCondBr(ctx.builder, is_null, bb_merge, bb_loop)

    // loop: a = p[0]; if a==0 break with h (never reads past the null)
    LLVMPositionBuilderAtEnd(ctx.builder, bb_loop)
    var h_phi = LLVMBuildPhi(ctx.builder, ctx.types.t_int64, "hash.h")
    var p_phi = LLVMBuildPhi(ctx.builder, i8ptr, "hash.p")
    var a = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, p_phi, "hash.a")
    var a_is_zero = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, a, zero8, "hash.a0")
    LLVMBuildCondBr(ctx.builder, a_is_zero, bb_merge, bb_body)

    // body: b = p[1]; v = a | (b<<8); h = (h ^ v) * prime; if b==0 break with hmul
    LLVMPositionBuilderAtEnd(ctx.builder, bb_body)
    var a64 = LLVMBuildZExt(ctx.builder, a, ctx.types.t_int64, "hash.a64")
    var idx1 = LLVMConstInt(ctx.types.t_int64, 1ul, 0)
    var p1 = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, p_phi, idx1, "hash.p1")
    var bbyte = LLVMBuildLoad2(ctx.builder, ctx.types.t_int8, p1, "hash.b")
    var b64 = LLVMBuildZExt(ctx.builder, bbyte, ctx.types.t_int64, "hash.b64")
    var sh8 = LLVMConstInt(ctx.types.t_int64, 8ul, 0)
    var bsh = LLVMBuildShl(ctx.builder, b64, sh8, "hash.bsh")
    var v = LLVMBuildOr(ctx.builder, a64, bsh, "hash.v")
    var hx = LLVMBuildXor(ctx.builder, h_phi, v, "hash.hx")
    var hmul = LLVMBuildMul(ctx.builder, hx, prime, "hash.hmul")
    var b_is_zero = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntEQ, bbyte, zero8, "hash.b0")
    LLVMBuildCondBr(ctx.builder, b_is_zero, bb_merge, bb_cont)

    // cont: advance two bytes, loop
    LLVMPositionBuilderAtEnd(ctx.builder, bb_cont)
    var idx2 = LLVMConstInt(ctx.types.t_int64, 2ul, 0)
    var p2 = LLVMBuildGEP2(ctx.builder, ctx.types.t_int8, p_phi, idx2, "hash.p2")
    LLVMBuildBr(ctx.builder, bb_loop)

    LLVMAddIncoming(h_phi, [basis, hmul], [entry, bb_cont])
    LLVMAddIncoming(p_phi, [ptr, p2], [entry, bb_cont])

    // merge: pick the hash at whichever exit fired, then clamp h<=1 -> prime
    LLVMPositionBuilderAtEnd(ctx.builder, bb_merge)
    var hres = LLVMBuildPhi(ctx.builder, ctx.types.t_int64, "hash.res")
    LLVMAddIncoming(hres, [basis, h_phi, hmul], [entry, bb_loop, bb_body])
    var clamp = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntULE, hres, one64, "hash.clamp")
    return LLVMBuildSelect(ctx.builder, clamp, prime, hres, "hash")
}

def intrinsic_builtin_hash_string(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    if (empty(arguments) || expr.arguments[0]._type.baseType != Type.tString) return null
    return build_string_hash(ctx, arguments[0])
}

def intrinsic_builtin_int(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let intType = base_type_to_llvm_type(expr._type.baseType)
    if (empty(arguments)) return LLVMConstInt(intType, 0ul, 0)
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
        return LLVMBuildTruncOrBitCast(ctx.builder, arguments[0], intType, string(expr.name)) if (argType.sizeOf >= expr._type.sizeOf)
        // For enums, the sign of the underlying type drives sign vs zero extension —
        // uint8/uint16/uint64-backed enums must zero-extend (200u8 → 0xc8u, not 0xffffffc8u).
        var widenSigned = argType.isSignedInteger
        if (argType.isEnum) {
            let ebt = argType.enumType.baseType
            widenSigned = ebt == Type.tInt8 || ebt == Type.tInt16 || ebt == Type.tInt || ebt == Type.tInt64
        }
        if (widenSigned) {
            return LLVMBuildSExtOrBitCast(ctx.builder, arguments[0], intType, string(expr.name))
        } else {
            return LLVMBuildZExtOrBitCast(ctx.builder, arguments[0], intType, string(expr.name))
        }
    } else {
        return null
    }
}

def any2int(var ctx : JitCtx; val : LLVMOpaqueValue?; argType : Type; _outType : LLVMOpaqueType?; inbytes, outbytes : int; outSigned : bool) : LLVMOpaqueValue? {   // nolint:LINT014 — private helper called from intrinsics that share the table signature
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

def intrinsic_builtin_int_vec(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume resType = expr._type
    assume argType = expr.arguments[0]._type
    if (empty(arguments)) return build_broadcast_vector(ctx.builder, resType, ctx.types.ConstI32(0ul))
    let outSigned = resType.vectorBaseType == Type.tInt
    // this works around the bug of fp2ui not working correctly for vectors on some values
    let isFloatToUintVec = argType.isVectorType && (argType.vectorBaseType == Type.tFloat) && !outSigned
    if (length(arguments) == 1 && !isFloatToUintVec) {
        if (argType.isVectorType) {
            // source element size decides widen-vs-trunc inside any2int (the lattice widen converts)
            assume srcBt = argType.vectorBaseType
            var srcElemBytes = 4
            if (srcBt == Type.tInt16 || srcBt == Type.tUInt16) {
                srcElemBytes = 2
            } elif (srcBt == Type.tInt8 || srcBt == Type.tUInt8) {
                srcElemBytes = 1
            }
            return any2int(ctx, arguments[0], argType.vectorBaseType, type_to_llvm_type(expr._type), srcElemBytes, 4, outSigned)
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

def any2float(var ctx : JitCtx; val : LLVMOpaqueValue?; argType : Type; _outType : LLVMOpaqueType? = null) {   // nolint:LINT014 — private helper called from intrinsics that share the table signature
    let outType = _outType != null ? _outType : ctx.types.t_float
    return val                                              if (argType == Type.tFloat)
    return LLVMBuildFPExt(ctx.builder, val, outType, "") if (argType == Type.tFloat16)
    return LLVMBuildFPTrunc(ctx.builder, val, outType, "") if (argType == Type.tDouble)
    return LLVMBuildSIToFP(ctx.builder, val, outType, "") if (argType == Type.tInt8 || argType == Type.tInt16 || argType == Type.tInt || argType == Type.tInt64)
    return LLVMBuildUIToFP(ctx.builder, val, outType, "") if (argType == Type.tUInt8 || argType == Type.tUInt16 || argType == Type.tUInt || argType == Type.tUInt64)
    panic("unsupported any2float base type {argType}")
    return null
}

def intrinsic_builtin_float_vec(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume resType = expr._type
    if (empty(arguments)) return build_broadcast_vector(ctx.builder, resType, LLVMConstReal(ctx.types.t_float, 0.0lf))
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

def intrinsic_builtin_float(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let floatType = base_type_to_llvm_type(expr._type.baseType)
    if (empty(arguments)) return LLVMConstReal(floatType, 0.0lf)
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
    } elif (argType.baseType == Type.tFloat16) {
        return LLVMBuildFPExt(ctx.builder, arguments[0], floatType, string(expr.name))
    } else {
        return null
    }
}

// ===== 16/8-bit lattice ctors / converts =====

def lattice_scalar_to_elem(var ctx : JitCtx; val : LLVMOpaqueValue?; srcBt : Type; dstBt : Type) : LLVMOpaqueValue? {   // nolint:LINT014 — private helper shared by the lattice ctor intrinsics
    // one ctor/splat scalar argument -> one lattice lane, matching SVecConv semantics
    if (dstBt == Type.tFloat16) {
        // SVecConv<float16_t> is float16_t(float(v)) — always promote through float32
        if (srcBt == Type.tFloat16) {
            return val
        }
        var f = val
        if (srcBt == Type.tDouble) {
            f = LLVMBuildFPTrunc(ctx.builder, val, ctx.types.t_float, "")
        } elif (srcBt == Type.tInt) {
            f = LLVMBuildSIToFP(ctx.builder, val, ctx.types.t_float, "")
        } elif (srcBt == Type.tUInt) {
            f = LLVMBuildUIToFP(ctx.builder, val, ctx.types.t_float, "")
        }
        return LLVMBuildFPTrunc(ctx.builder, f, ctx.types.t_half, "")
    }
    let dstT = base_type_to_llvm_type(dstBt)
    if (srcBt == Type.tFloat || srcBt == Type.tDouble) {
        let dstSigned = dstBt == Type.tInt16 || dstBt == Type.tInt8
        if (dstSigned) {
            return LLVMBuildFPToSI(ctx.builder, val, dstT, "")
        }
        return LLVMBuildFPToUI(ctx.builder, val, dstT, "")
    }
    return LLVMBuildTruncOrBitCast(ctx.builder, val, dstT, "")      // int/uint -> C truncation
}

def intrinsic_builtin_lattice_vec(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    assume resType = expr._type
    let dstVecT = type_to_llvm_type(resType)
    if (empty(arguments)) {
        return LLVMConstNull(dstVecT)       // zero ctor (also the default<T> lowering)
    }
    assume argType = expr.arguments[0]._type
    let dstBt = resType.vectorBaseType
    if (length(arguments) == 1) {
        if (argType.baseType == resType.baseType) {
            return arguments[0]             // pass-through ctor
        }
        if (argType.isVectorType) {
            // lane-matched converts (the das_sv_cvt externs share the ctor name)
            assume srcBt = argType.vectorBaseType
            if (dstBt == Type.tFloat16 && srcBt == Type.tFloat) {
                return LLVMBuildFPTrunc(ctx.builder, arguments[0], dstVecT, string(expr.name))
            }
            if ((srcBt == Type.tInt || srcBt == Type.tUInt) ||
                    (srcBt == Type.tInt16 && dstBt == Type.tInt8) ||
                    (srcBt == Type.tUInt16 && dstBt == Type.tUInt8)) {
                return LLVMBuildTrunc(ctx.builder, arguments[0], dstVecT, string(expr.name))    // C-truncating narrow
            }
            if (srcBt == Type.tInt8 && dstBt == Type.tInt16) {
                return LLVMBuildSExt(ctx.builder, arguments[0], dstVecT, string(expr.name))     // byte -> short widen
            }
            if (srcBt == Type.tUInt8 && dstBt == Type.tUInt16) {
                return LLVMBuildZExt(ctx.builder, arguments[0], dstVecT, string(expr.name))     // ubyte -> ushort widen
            }
            failed_E(expr, "unsupported lattice convert {expr.name}({describe(argType)})")
            return null
        }
        let lane = lattice_scalar_to_elem(ctx, arguments[0], argType.baseType, dstBt)
        return build_broadcast_vector(ctx.builder, resType, lane)   // scalar splat
    }
    if (resType.baseType == Type.tHalf8 && length(arguments) == 2 && argType.baseType == Type.tFloat4) {
        // half8(float4 lo, float4 hi) pack
        let half4T = LLVMVectorType(ctx.types.t_half, 4u)
        var lo = LLVMBuildFPTrunc(ctx.builder, arguments[0], half4T, "half8_pack_lo")
        var hi = LLVMBuildFPTrunc(ctx.builder, arguments[1], half4T, "half8_pack_hi")
        return LLVMBuildShuffleVector(ctx.builder, ctx.types, lo, hi, [0, 1, 2, 3, 4, 5, 6, 7], "half8_pack")
    }
    // per-lane ctor (registrations are scalar-args-only for the lattice families)
    var vres = LLVMGetUndef(dstVecT)
    var elemIndex = 0
    for (earg, arg in expr.arguments, arguments) {
        let lane = lattice_scalar_to_elem(ctx, arg, earg._type.baseType, dstBt)
        vres = LLVMBuildInsertElement(ctx.builder, vres, lane, ctx.types.ConstI32(uint64(elemIndex++)), "")
    }
    if (elemIndex != resType.vectorDim) {
        failed_E(expr, "vector ctor {expr.name}({describe(expr._type)}) has {elemIndex} elements, but {resType.vectorDim} are expected")
    }
    return vres
}

def build_const_int_splat(elemT : LLVMOpaqueType?; lanes : int; value : int) : LLVMOpaqueValue? {
    var elems <- [for (_i in range(lanes)); LLVMConstInt(elemT, uint64(int64(value)), value < 0 ? 1 : 0)]
    return LLVMConstVector(array_data_ptr(elems), uint(lanes))
}

def build_int_minmax_call(var ctx : JitCtx; iname : string; a, b : LLVMOpaqueValue?; vecT : LLVMOpaqueType?) : LLVMOpaqueValue? {
    var argTypes <- [vecT]
    let id = LLVMLookupIntrinsicID(iname)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    var typ = LLVMFunctionType(vecT, [vecT, vecT])
    var args <- [a, b]
    return LLVMBuildCall2(ctx.builder, typ, decl, args, "")
}

def intrinsic_builtin_lattice_sat(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    // saturating narrow: clamp to the target range, then trunc. The clamp+trunc shape is
    // what both backends pattern-match to native saturating narrows (aarch64 sqxtn/uqxtn,
    // x64 packs/packus) — no target-specific gate needed, semantics match das_sv_cvt_sat.
    assume resType = expr._type
    assume argType = expr.arguments[0]._type
    let dstBt = resType.vectorBaseType
    let srcVecT = type_to_llvm_type(argType)
    let srcElemT = base_type_to_llvm_type(argType.vectorBaseType)
    let lanes = resType.vectorDim
    var clamped = arguments[0]
    if (dstBt == Type.tInt16 || dstBt == Type.tInt8) {
        let lim = dstBt == Type.tInt16 ? 32767 : 127
        clamped = build_int_minmax_call(ctx, "llvm.smin", clamped, build_const_int_splat(srcElemT, lanes, lim), srcVecT)
        clamped = build_int_minmax_call(ctx, "llvm.smax", clamped, build_const_int_splat(srcElemT, lanes, -lim - 1), srcVecT)
    } else {
        let lim = dstBt == Type.tUInt16 ? 65535 : 255
        clamped = build_int_minmax_call(ctx, "llvm.umin", clamped, build_const_int_splat(srcElemT, lanes, lim), srcVecT)
    }
    return LLVMBuildTrunc(ctx.builder, clamped, type_to_llvm_type(resType), string(expr.name))
}

def intrinsic_builtin_float16(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    // scalar float16(float) — the only registered scalar fp16 ctor
    return LLVMBuildFPTrunc(ctx.builder, arguments[0], ctx.types.t_half, string(expr.name))
}

def intrinsic_half8_lo_hi(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let base = expr.name == "half8_lo" ? 0 : 4
    let mask <- [for (i in range(4)); base + i]
    var quad = LLVMBuildShuffleVector(ctx.builder, ctx.types, arguments[0], arguments[0], mask, string(expr.name))
    return LLVMBuildFPExt(ctx.builder, quad, LLVMVectorType(ctx.types.t_float, 4u), "")
}

def intrinsic_builtin_range(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_bit_nzp_op1(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    let sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type for {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0], LLVMConstInt(ctx.types.t_int1, 0ul, 0)]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    let id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], ctx.types.t_int1])
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def intrinsic_bit_op1(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    bit_op1_name |> get(string(expr.name)) $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    let sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0]]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    let id = LLVMLookupIntrinsicID(sqrt_name)
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

def intrinsic_op2(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    op2_name |> get(string(expr.name)) $(pname) {
        op_name = pname
    }
    if (op_name == "") {
        failed_E(expr, "intrinsic {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    let sqrt_name = build_op_name(op_name, opType)
    if (sqrt_name == "") {
        failed_E(expr, "intrinsic type {expr.name}({describe(opType)}) is not supported (yet?)")
        return null
    }
    var args <- [ arguments[0], arguments[1]]
    var argTypes <- [ type_to_llvm_abi_type(expr.arguments[0]._type)]
    let id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0]])
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def build_math_minmax(g_builder : LLVMOpaqueBuilder?; exprName : string; expr : ExprCallFunc?; arg0, arg1 : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var op_name : string
    if (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
        let pred = exprName == "min" ? LLVMRealPredicate.LLVMRealOLT : LLVMRealPredicate.LLVMRealOGT
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
    let id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0]])
    return LLVMBuildCall2(g_builder, typ, decl, args, exprName)
}

def intrinsic_math_minmax(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return build_math_minmax(ctx.builder, string(expr.name), expr, arguments[0], arguments[1])
}

def intrinsic_math_clamp(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var mini = build_math_minmax(ctx.builder, "max", expr, arguments[0], arguments[1])
    return build_math_minmax(ctx.builder, "min", expr, mini, arguments[2])
}

// intrinsics, math

def intrinsic_math_rcp(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_rsqrt(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_any_float_op1(expr_name : string; g_builder : LLVMOpaqueBuilder?; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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
    let id = LLVMLookupIntrinsicID(sqrt_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {sqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, expr_name)
}

def intrinsic_math_float_op1(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return intrinsic_math_any_float_op1(string(expr.name), ctx.builder, expr, arguments)
}

def intrinsic_math_float_op1_to_int(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var op_name = ""
    peek(expr.name) $(s) {
        op_name = slice(s, 0, -1)
    }
    var res = intrinsic_math_any_float_op1(op_name, ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_tan(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var sin_a = intrinsic_math_any_float_op1("sin", ctx.builder, expr, arguments)
    var cos_a = intrinsic_math_any_float_op1("cos", ctx.builder, expr, arguments)
    return LLVMBuildFDiv(ctx.builder, sin_a, cos_a, "tan")
}

def intrinsic_math_lerp_op3(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_step(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        var zero = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 0.0lf))
        var one = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 1.0lf))
        // GLSL step(edge,x) = x<edge ? 0 : 1. fcmp OLT(x,edge) -> select(cmp, 0, 1).
        var cmp = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOLT, arguments[1], arguments[0], "")
        return LLVMBuildSelect(ctx.builder, cmp, zero, one, "step")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_smoothstep(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        var zero = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 0.0lf))
        var one = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 1.0lf))
        var two = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 2.0lf))
        var three = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 3.0lf))
        // t = clamp((x-e0)/(e1-e0), 0, 1)
        var x_minus_e0 = LLVMBuildFSub(ctx.builder, arguments[2], arguments[0], "")
        var e1_minus_e0 = LLVMBuildFSub(ctx.builder, arguments[1], arguments[0], "")
        var t_raw = LLVMBuildFDiv(ctx.builder, x_minus_e0, e1_minus_e0, "")
        var cmp_lo = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOGT, t_raw, zero, "")
        var t_lo = LLVMBuildSelect(ctx.builder, cmp_lo, t_raw, zero, "")
        var cmp_hi = LLVMBuildFCmp(ctx.builder, LLVMRealPredicate.LLVMRealOLT, t_lo, one, "")
        var t = LLVMBuildSelect(ctx.builder, cmp_hi, t_lo, one, "")
        // t*t*(3 - 2*t)
        var two_t = LLVMBuildFMul(ctx.builder, two, t, "")
        var three_minus_two_t = LLVMBuildFSub(ctx.builder, three, two_t, "")
        var t_sq = LLVMBuildFMul(ctx.builder, t, t, "")
        return LLVMBuildFMul(ctx.builder, t_sq, three_minus_two_t, "smoothstep")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_radians(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        let k = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 3.14159265358979323846lf / 180.0lf))
        return LLVMBuildFMul(ctx.builder, arguments[0], k, "radians")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_degrees(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        let k = build_broadcast_vector(ctx.builder, argType, LLVMConstReal(ctx.types.t_float, 180.0lf / 3.14159265358979323846lf))
        return LLVMBuildFMul(ctx.builder, arguments[0], k, "degrees")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_fmod(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    assume argType = expr.arguments[0]._type
    if (argType.baseType == Type.tFloat || argType.baseType == Type.tDouble || (argType.isVectorType && argType.vectorBaseType == Type.tFloat)) {
        return LLVMBuildFRem(ctx.builder, arguments[0], arguments[1], "fmod")
    } else {
        failed_E(expr, "{expr.name}({describe(argType)}) is not supported (yet?)")
        return null
    }
}

def intrinsic_math_sinh_cosh_tanh(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    return intrinsic_math_any_float_op1(string(expr.name), ctx.builder, expr, arguments)
}

def private vexpf_fma(builder : LLVMOpaqueBuilder?; decl : LLVMOpaqueValue?; fty : LLVMOpaqueType?;
                      a, b, c : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    if (decl != null) {
        return LLVMBuildCall2(builder, fty, decl, [ a, b, c], "")   // @llvm.fmuladd: a*b+c
    }
    return LLVMBuildFAdd(builder, LLVMBuildFMul(builder, a, b, ""), c, "")
}

def private vexpf_isplat(ctx : JitCtx; dim : int; v : uint64) : LLVMOpaqueValue? {
    var res = LLVMGetUndef(LLVMVectorType(ctx.types.t_int32, uint(dim)))
    let cv = LLVMConstInt(ctx.types.t_int32, v, 0)
    for (d in range(dim)) {
        res = LLVMBuildInsertElement(ctx.builder, res, cv, LLVMConstInt(ctx.types.t_int32, uint64(d), 0), "")
    }
    return res
}

def private vexpf_fsplat(ctx : JitCtx; opType : TypeDeclPtr; v : double) : LLVMOpaqueValue? {
    return build_broadcast_vector(ctx.builder, opType, LLVMConstReal(ctx.types.t_float, v))
}

// Vectorized expf emitted as inline IR — ggml_v_expf (ARM optimized-routines), max rel err ~2 ulp.
// The default lowering (@llvm.exp.vNf32) scalarizes to N libm expf calls on targets without a vector
// libm (Darwin-ARM, most others) — ~7x slower. The |n|>126 overflow/underflow path is a branchless
// select tree (correct inf/0), so this is a full-range drop-in: callers need no clamp. The always-
// computed guard costs ~2.4x of the throughput (~2.9x vs scalar on M1); on x64 (znver2, 2026-07-02)
// it is fatal — 1.12x scalar — so this stays aarch64-only BY MEASUREMENT (gate in intrinsic_math_exp).
// A ggml-style fast-path branch over the |n|>126 mask would recover ~7x on the common case (unclaimed).
def private build_vector_expf(ctx : JitCtx; opType : TypeDeclPtr; x : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let bld = ctx.builder
    let dim = opType.vectorDim
    let lt = type_to_llvm_type(opType)
    let fty = LLVMFunctionType(lt, [ lt, lt, lt])
    let fid = LLVMLookupIntrinsicID(build_op_name("fmuladd", opType))
    let fma_decl = fid != 0u ? LLVMGetIntrinsicDeclaration(g_mod, fid, [ lt]) : null
    let R = vexpf_fsplat(ctx, opType, 12582912.0lf); let LE = vexpf_fsplat(ctx, opType, 1.4426950216293335lf)
    let NH = vexpf_fsplat(ctx, opType, -0.693145751953125lf); let NL = vexpf_fsplat(ctx, opType, -1.428606765330187e-06lf)
    let C0 = vexpf_fsplat(ctx, opType, 0.9999994039535522lf); let C1 = vexpf_fsplat(ctx, opType, 0.4999912679195404lf)
    let C2 = vexpf_fsplat(ctx, opType, 0.16668395698070526lf); let C3 = vexpf_fsplat(ctx, opType, 0.04189976677298546lf)
    let C4 = vexpf_fsplat(ctx, opType, 0.008247390389442444lf)
    let z = vexpf_fma(bld, fma_decl, fty, x, LE, R)                          // x*log2e + magic
    let n = LLVMBuildFSub(bld, z, R, "")                                     // round(x*log2e)
    let bred = vexpf_fma(bld, fma_decl, fty, n, NL, vexpf_fma(bld, fma_decl, fty, n, NH, x))  // x - n*ln2
    let u = LLVMBuildFMul(bld, bred, bred, "")
    let inner = vexpf_fma(bld, fma_decl, fty, vexpf_fma(bld, fma_decl, fty, C4, bred, C3), u,
        vexpf_fma(bld, fma_decl, fty, C2, bred, C1))
    let poly = vexpf_fma(bld, fma_decl, fty, inner, u, LLVMBuildFMul(bld, C0, bred, ""))
    let e = LLVMBuildShl(bld, LLVMBuildBitCast(bld, z, LLVMVectorType(ctx.types.t_int32, uint(dim)), ""),
        vexpf_isplat(ctx, dim, 23ul), "")                                   // exponent field of 2^n
    let k = LLVMBuildBitCast(bld, LLVMBuildAdd(bld, e, vexpf_isplat(ctx, dim, 0x3f800000ul), ""), lt, "")
    let fast = vexpf_fma(bld, fma_decl, fty, poly, k, k)                     // k*(1+poly), normal range
    let fabs_decl = LLVMGetIntrinsicDeclaration(g_mod, LLVMLookupIntrinsicID(build_op_name("fabs", opType)), [ lt])
    let nabs = LLVMBuildCall2(bld, LLVMFunctionType(lt, [ lt]), fabs_decl, [ n], "")
    let c126 = LLVMBuildFCmp(bld, LLVMRealPredicate.LLVMRealOGT, nabs, vexpf_fsplat(ctx, opType, 126.0lf), "")
    let c192 = LLVMBuildFCmp(bld, LLVMRealPredicate.LLVMRealOGT, nabs, vexpf_fsplat(ctx, opType, 192.0lf), "")
    let nlez = LLVMBuildFCmp(bld, LLVMRealPredicate.LLVMRealOLE, n, vexpf_fsplat(ctx, opType, 0.0lf), "")
    let d = LLVMBuildSelect(bld, nlez, vexpf_isplat(ctx, dim, 0x82000000ul), vexpf_isplat(ctx, dim, 0ul), "")
    let s1 = LLVMBuildBitCast(bld, LLVMBuildAdd(bld, d, vexpf_isplat(ctx, dim, 0x7f000000ul), ""), lt, "")
    let s2 = LLVMBuildBitCast(bld, LLVMBuildSub(bld, e, d, ""), lt, "")
    let mid = LLVMBuildFMul(bld, vexpf_fma(bld, fma_decl, fty, s2, poly, s2), s1, "")  // |n|>126 underflow/overflow
    let big = LLVMBuildFMul(bld, s1, s1, "")                                 // |n|>192 saturates to inf/0
    return LLVMBuildSelect(bld, c192, big, LLVMBuildSelect(bld, c126, mid, fast, ""), "")
}

// exp: aarch64 vector-float takes the inline polynomial (measurements + rationale at
// build_vector_expf above); scalar float/double and all non-aarch64 vector exp keep @llvm.exp —
// libm is correctly rounded and a single scalar call carries no scalarization penalty.
def intrinsic_math_exp(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    if (g_target_is_aarch64 && opType.isVectorType && opType.vectorBaseType == Type.tFloat) {
        return build_vector_expf(ctx, opType, arguments[0])
    }
    return intrinsic_math_any_float_op1("exp", ctx.builder, expr, arguments)
}

// ===== the idot family: exact integer dots on the 8-bit lattice =====
// Two lowerings: the signed x signed form emits ONE @llvm.aarch64.neon.sdot on the aarch64
// host-features rail (g_target_host_features = exactly when +dotprod is force-appended, see
// init_jit_target_flags); everything else gets generic widen-multiply IR. The AArch64 backend
// does NOT fold the generic form to SDOT (probe: zip/uzp/smull expansion), hence the native arm.
// The unsigned x signed form needs USDOT (i8mm — absent on M1); it rides the i8mm un-gate item.
def private idot_products(var ctx : JitCtx; expr : ExprCallFunc?; a, b : LLVMOpaqueValue?; aIdx : int) : LLVMOpaqueValue? {   // nolint:LINT014 — shared by the idot intrinsics
    let v16i32 = LLVMVectorType(ctx.types.t_int32, 16u)
    let aUnsigned = expr.arguments[aIdx]._type.vectorBaseType == Type.tUInt8
    var wa = aUnsigned ? LLVMBuildZExt(ctx.builder, a, v16i32, "") : LLVMBuildSExt(ctx.builder, a, v16i32, "")
    var wb = LLVMBuildSExt(ctx.builder, b, v16i32, "")
    return LLVMBuildMul(ctx.builder, wa, wb, "idot_prod")
}

// One @llvm.aarch64.neon.sdot(acc, a, b) over byte16 values (the idot ss form). Returns null
// when the intrinsic is unavailable so callers fall through to the generic lowering.
def private idot_sdot_aarch64(var ctx : JitCtx; acc, a, b : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.sdot")
    var declTypes <- [v4i32, v16i8]                 // sdot is overloaded on result + operand vector types
    var decl = id != 0u ? LLVMGetIntrinsicDeclaration(g_mod, id, declTypes) : null
    if (decl == null) {
        return null
    }
    var fnArgTypes <- [v4i32, v16i8, v16i8]
    var fnTy = LLVMFunctionType(v4i32, fnArgTypes)
    var callArgs <- [acc, a, b]
    return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "idot4")
}

def intrinsic_lattice_idot(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let aUnsigned = expr.arguments[0]._type.vectorBaseType == Type.tUInt8
    if (g_target_is_aarch64 && g_target_host_features && !aUnsigned) {
        let v4i32 = ctx.types.LLVMInt4Type()
        var s = idot_sdot_aarch64(ctx, LLVMConstNull(v4i32), arguments[0], arguments[1])
        if (s != null) {
            let rid = LLVMLookupIntrinsicID("llvm.vector.reduce.add")
            var rtys <- [v4i32]
            var rdecl = rid != 0u ? LLVMGetIntrinsicDeclaration(g_mod, rid, rtys) : null
            if (rdecl != null) {
                var rtyp = LLVMFunctionType(ctx.types.t_int32, [v4i32])
                var rargs <- [s]
                return LLVMBuildCall2(ctx.builder, rtyp, rdecl, rargs, "idot")
            }
        }
    }
    var prod = idot_products(ctx, expr, arguments[0], arguments[1], 0)
    let v16i32 = LLVMVectorType(ctx.types.t_int32, 16u)
    let id = LLVMLookupIntrinsicID("llvm.vector.reduce.add")
    var tys <- [v16i32]
    var decl = id != 0u ? LLVMGetIntrinsicDeclaration(g_mod, id, tys) : null
    if (decl == null) {
        failed_E(expr, "idot: llvm.vector.reduce.add unavailable on this LLVM")
        return null
    }
    var ftyp = LLVMFunctionType(ctx.types.t_int32, [v16i32])
    var fargs <- [prod]
    return LLVMBuildCall2(ctx.builder, ftyp, decl, fargs, "idot")
}

def intrinsic_lattice_idot4(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let hasAcc = length(arguments) == 3
    let aIdx = hasAcc ? 1 : 0
    let aUnsigned = expr.arguments[aIdx]._type.vectorBaseType == Type.tUInt8
    if (g_target_is_aarch64 && g_target_host_features && !aUnsigned) {
        let v4i32 = ctx.types.LLVMInt4Type()
        var acc = hasAcc ? arguments[0] : LLVMConstNull(v4i32)
        var s = idot_sdot_aarch64(ctx, acc, arguments[aIdx], arguments[aIdx + 1])
        if (s != null) {
            return s
        }
    }
    var prod = idot_products(ctx, expr, arguments[aIdx], arguments[aIdx + 1], aIdx)
    // per-dword-lane partial sums: stride-4 shuffles of the products, summed pairwise
    var p0 = LLVMBuildShuffleVector(ctx.builder, ctx.types, prod, prod, [0, 4, 8, 12], "")
    var p1 = LLVMBuildShuffleVector(ctx.builder, ctx.types, prod, prod, [1, 5, 9, 13], "")
    var p2 = LLVMBuildShuffleVector(ctx.builder, ctx.types, prod, prod, [2, 6, 10, 14], "")
    var p3 = LLVMBuildShuffleVector(ctx.builder, ctx.types, prod, prod, [3, 7, 11, 15], "")
    var s = LLVMBuildAdd(ctx.builder, LLVMBuildAdd(ctx.builder, p0, p1, ""), LLVMBuildAdd(ctx.builder, p2, p3, ""), "idot4")
    if (hasAcc) {
        s = LLVMBuildAdd(ctx.builder, arguments[0], s, "idot4_acc")
    }
    return s
}

// dynamic table select: idx masked to 16 entries. aarch64 emits one @llvm.aarch64.neon.tbl1
// (baseline NEON — same gate as the tbl16 intrinsics); elsewhere 16 dynamically-indexed lane
// reads, generic IR every backend accepts (pshufb single-op = the x64 wave)
[unused_argument(expr)]
def intrinsic_lattice_shuffle(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    var idx = LLVMBuildAnd(ctx.builder, arguments[1], splat_v16i8(ctx, 15), "shuffle_idx")
    if (g_target_is_aarch64) {
        let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.tbl1")
        var declTypes <- [v16i8]
        var decl = id != 0u ? LLVMGetIntrinsicDeclaration(g_mod, id, declTypes) : null
        if (decl != null) {
            var fnArgTypes <- [v16i8, v16i8]
            var fnTy = LLVMFunctionType(v16i8, fnArgTypes)
            var callArgs <- [arguments[0], idx]
            return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "shuffle")
        }
    }
    var vres = LLVMGetUndef(v16i8)
    for (i in range(16)) {
        var lane = LLVMBuildExtractElement(ctx.builder, idx, ctx.types.ConstI32(uint64(i)), "")
        var val = LLVMBuildExtractElement(ctx.builder, arguments[0], lane, "")
        vres = LLVMBuildInsertElement(ctx.builder, vres, val, ctx.types.ConstI32(uint64(i)), "")
    }
    return vres
}

def intrinsic_math_mad_op3(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var a = arguments[0]
    var b = arguments[1]
    if (expr.arguments[0]._type.isVectorType && !expr.arguments[1]._type.isVectorType) {
        b = expand_scalar(ctx.builder, b, expr.arguments[0]._type)
    }
    var c = arguments[2]
    let bt = expr._type.baseType
    if (bt == Type.tFloat || bt == Type.tDouble || bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4) {
        // a*b+c as @llvm.fmuladd — the backend fuses to one fmla (mad is a MAC by contract)
        let lt = type_to_llvm_abi_type(expr._type)
        let id = LLVMLookupIntrinsicID(build_op_name("fmuladd", expr._type))
        var fatys <- [ lt]
        var decl = id != 0u ? LLVMGetIntrinsicDeclaration(g_mod, id, fatys) : null
        if (decl != null) {
            var fargs <- [ a, b, c]
            var ftyp = LLVMFunctionType(lt, [ lt, lt, lt])
            return LLVMBuildCall2(ctx.builder, ftyp, decl, fargs, "mad")
        }
        var a_mul_b = LLVMBuildFMul(ctx.builder, a, b, "")   // fallback: fmuladd unavailable
        return LLVMBuildFAdd(ctx.builder, a_mul_b, c, "mad")
    } else {
        var a_mul_b = LLVMBuildMul(ctx.builder, a, b, "")
        return LLVMBuildAdd(ctx.builder, a_mul_b, c, "mad")
    }
}

def intrinsic_math_abs(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_abs_float(int_name : string; g_builder : LLVMOpaqueBuilder?; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var args <- [ arguments[0]]
    var argTypes <- [ type_to_llvm_abi_type(opType)]
    let id = LLVMLookupIntrinsicID(int_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {int_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], argTypes) // type : a -> a
    return LLVMBuildCall2(g_builder, typ, decl, args, string(expr.name))
}

def intrinsic_math_abs_int(abs_name : string; ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    var args <- [ arguments[0], LLVMConstInt(ctx.types.t_int1, 0ul, 0)]
    var argTypes <- [ type_to_llvm_abi_type(opType)]
    let id = LLVMLookupIntrinsicID(abs_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed_E(expr, "missing intrinsic {abs_name}")
        return null
    }
    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], ctx.types.t_int1]) // type : (a, LLVMInt1Type) -> a
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

def intrinsic_math_saturate(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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
    let id = LLVMLookupIntrinsicID(fadd_name)
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
    let id = LLVMLookupIntrinsicID(fsqrt_name)
    var decl_fsqrt = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes_fsqrt)
    if (decl_fsqrt == null) {
        failed("missing intrinsic {fsqrt_name}")
        return null
    }
    var typ = LLVMFunctionType(ctx.types.t_float, argTypes_fsqrt)
    return LLVMBuildCall2(ctx.builder, typ, decl_fsqrt, args_fsqrt, name)
}

def build_frcp(var ctx : JitCtx; v2 : LLVMOpaqueValue?; _name : string) : LLVMOpaqueValue? {   // nolint:LINT014 — private helper called from intrinsics that share the table signature
    var one = LLVMConstReal(ctx.types.t_float, 1.0lf)
    var fdiv = LLVMBuildFDiv(ctx.builder, one, v2, "")
    return fdiv
}

def intrinsic_math_length(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    return build_fsqrt(ctx, len_sq, "length")
}

def intrinsic_math_length_sq(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    return build_fadd(ctx, opType, v2, "length_sq")
}

def intrinsic_math_dot(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[1], "")
    return build_fadd(ctx, opType, v2, "dot")
}

// hmin/hmax: horizontal min/max via llvm.vector.reduce.fmin/fmax (one IR op, no extern call).
def intrinsic_math_hminmax(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let op = expr.name == "hmin" ? "fmin" : "fmax"
    let iname = "llvm.vector.reduce.{op}.v{opType.vectorDim}f32"
    var argTypes <- [ type_to_llvm_abi_type(opType)]
    let id = LLVMLookupIntrinsicID(iname)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if (decl == null) {
        failed("missing intrinsic {iname}")
        return null
    }
    var args <- [ arguments[0]]
    var typ = LLVMFunctionType(ctx.types.t_float, [ type_to_llvm_abi_type(opType)])
    return LLVMBuildCall2(ctx.builder, typ, decl, args, string(expr.name))
}

// hadd: horizontal sum via llvm.vector.reduce.fadd (shares build_fadd with dot/length).
def intrinsic_math_hadd(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    return build_fadd(ctx, opType, arguments[0], "hadd")
}

def intrinsic_math_fast_normalize(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    let len = build_fsqrt(ctx, len_sq, "")
    let ilen = build_frcp(ctx, len, "")
    let vilen = build_broadcast_vector(ctx.builder, opType, ilen)
    return LLVMBuildFMul(ctx.builder, arguments[0], vilen, "fast_normalize")
}


def intrinsic_math_normalize(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_cross(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_inv_distance_sq(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v = LLVMBuildFSub(ctx.builder, arguments[0], arguments[1], "")
    let v2 = LLVMBuildFMul(ctx.builder, v, v, "")
    let distance_sq = build_fadd(ctx, opType, v2, "")
    return LLVMBuildFDiv(ctx.builder, LLVMConstReal(ctx.types.t_float, 1.0lf), distance_sq, "inv_distance_sq")
}

def intrinsic_math_inv_length(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    assume opType = expr.arguments[0]._type
    assert(opType.isVectorType)
    let v2 = LLVMBuildFMul(ctx.builder, arguments[0], arguments[0], "")
    let len_sq = build_fadd(ctx, opType, v2, "")
    let len = build_fsqrt(ctx, len_sq, "")
    return build_frcp(ctx, len, "inv_length")
}

def intrinsic_das_ptr_set_add(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_das_ptr_set_sub(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var args := arguments
    args[1] = LLVMBuildNeg(ctx.builder, args[1], "")
    return intrinsic_das_ptr_set_add(ctx, expr, args)
}

def intrinsic_das_ptr_add(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_das_ptr_sub(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var args := arguments
    args[1] = LLVMBuildNeg(ctx.builder, args[1], "")
    return intrinsic_das_ptr_add(ctx, expr, args)
}

def intrinsic_memset8(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    assume opType = expr.arguments[0]._type
    assert(opType.isPointer)
    return LLVMBuildMemSet(ctx.builder, arguments[0], arguments[1], arguments[2], 1u)
}

def build_cmove_nz(var ctx : JitCtx; val : LLVMOpaqueValue?; blk : block) {   // nolint:LINT014 — private helper called from intrinsics that share the table signature
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
    for (_i in range(count)) {
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
    build_cmove_nz(ctx, size_and_1) {
        build_write_n_values(ctx, iptr, value, ival, isize, 1)
    }
// if ( size & 3 ) write 2 values
    ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var size_and_3 = LLVMBuildAnd(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, 3ul, 0), "")
    build_cmove_nz(ctx, size_and_3) {
        build_write_n_values(ctx, iptr, value, ival, isize, 2)
    }
// if ( size & 7 ) write 4 values
    ival = LLVMBuildLoad2(ctx.builder, ctx.types.t_int64, isize, "")
    var size_and_7 = LLVMBuildAnd(ctx.builder, ival, LLVMConstInt(ctx.types.t_int64, 7ul, 0), "")
    build_cmove_nz(ctx, size_and_7) {
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

def intrinsic_memset128(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_memset64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_memset32(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_memset16(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var size = LLVMBuildZExtOrBitCast(ctx.builder, arguments[2], ctx.types.t_int64, "")
    var ptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(ctx.types.t_int16, 0u), "")
    return build_memset_16(ctx, ctx.types.t_int16, ptr, arguments[1], size)
}

def intrinsic_math_gather(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

// Emitters for f16_cvt::f16_to_f32 / f32_to_f16 (gate: g_f16_cvt_intrin_lookup — aarch64, or x64
// with F16C). The half IR lowers to scalar fcvt / vcvtph2ps+vcvtps2ph and the loop vectorizer widens
// it to the packed forms. NaN caveat (documented at the f16_cvt module): hardware quietens sNaN and
// keeps payloads where the daslang fallback flattens f32->f16 NaN to sign|0x7E00 — both stay NaN.
def intrinsic_f16_to_f32(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let h16 = LLVMBuildTrunc(ctx.builder, arguments[0], ctx.types.t_int16, "")
    let hval = LLVMBuildBitCast(ctx.builder, h16, LLVMHalfTypeInContext(ctx.ctx), "")
    return LLVMBuildFPExt(ctx.builder, hval, ctx.types.t_float, "f16_to_f32")
}

def intrinsic_f32_to_f16(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let hval = LLVMBuildFPTrunc(ctx.builder, arguments[0], LLVMHalfTypeInContext(ctx.ctx), "")
    let h16 = LLVMBuildBitCast(ctx.builder, hval, ctx.types.t_int16, "")
    return LLVMBuildZExt(ctx.builder, h16, ctx.types.t_int32, "f32_to_f16")
}

// aarch64_neon::sdot4(acc:int4; w,x:int8 const?):int4 — one @llvm.aarch64.neon.sdot: 16 int8·int8
// MACs into the 4 lanes of `acc`, exact int32 accumulate, NO horizontal reduce (the per-block addv
// is what drains the auto-vectorized path). Pointers arrive PRE-offset — just unaligned v16i8 loads.
def intrinsic_sdot4(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var wptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v16i8, 0u), "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var wvec = LLVMBuildLoad2(ctx.builder, v16i8, wptr, "")
    LLVMSetAlignment(wvec, 1u)
    var xvec = LLVMBuildLoad2(ctx.builder, v16i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.sdot")
    var declTypes <- [v4i32, v16i8]                 // sdot is overloaded on result + operand vector types
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.sdot (non-ARM target?)")
        return null
    }
    var fnArgTypes <- [v4i32, v16i8, v16i8]
    var fnTy = LLVMFunctionType(v4i32, fnArgTypes)
    var callArgs <- [arguments[0], wvec, xvec]
    return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "sdot4")
}

// aarch64_neon::sdot4_laneq(acc:int4; w,x:int8 const?; lane:int):int4 — the lane-indexed SDOT:
// `w` is 4 interleaved weight rows, and `x`'s 32-bit group `lane` is splatted across all 4 lanes
// via the i32-view shufflevector the AArch64 backend folds into the `Vm.4b[lane]` indexed sdot
// form. No addv: each accumulator lane is a distinct output row.
def intrinsic_sdot4_laneq(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var wptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v16i8, 0u), "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var wvec = LLVMBuildLoad2(ctx.builder, v16i8, wptr, "")
    LLVMSetAlignment(wvec, 1u)
    var xvec = LLVMBuildLoad2(ctx.builder, v16i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    // lane is baked into the shuffle mask (the indexed SDOT has no runtime-lane form), and the
    // function is public — reject non-constant / out-of-range rather than assert or miscompile
    if (LLVMIsAConstantInt(arguments[3]) == null) {
        failed_E(_expr, "aarch64_neon::sdot4_laneq: lane must be a compile-time constant")
        return null
    }
    let lane = int(LLVMConstIntGetZExtValue(arguments[3]))
    if (lane < 0 || lane > 3) {
        failed_E(_expr, "aarch64_neon::sdot4_laneq: lane must be in 0..3")
        return null
    }
    var xi32 = LLVMBuildBitCast(ctx.builder, xvec, v4i32, "")
    var xspl = LLVMBuildShuffleVector(ctx.builder, ctx.types, xi32, xi32, [lane, lane, lane, lane], "xlane")
    var xbcast = LLVMBuildBitCast(ctx.builder, xspl, v16i8, "")
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.sdot")
    var declTypes <- [v4i32, v16i8]
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.sdot (non-ARM target?)")
        return null
    }
    var fnArgTypes <- [v4i32, v16i8, v16i8]
    var fnTy = LLVMFunctionType(v4i32, fnArgTypes)
    var callArgs <- [arguments[0], wvec, xbcast]
    return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "sdot4_laneq")
}

// aarch64_neon::sdot4_w — sdot4 with the weight operand as a VALUE (a tbl16_lo/tbl16_hi result,
// bitcast v4i32->v16i8 in-register), so LUT-dequantized weights feed SDOT with no memory round-trip.
def intrinsic_sdot4_w(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var wvec = LLVMBuildBitCast(ctx.builder, arguments[1], v16i8, "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var xvec = LLVMBuildLoad2(ctx.builder, v16i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.sdot")
    var declTypes <- [v4i32, v16i8]
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.sdot (non-ARM target?)")
        return null
    }
    var fnArgTypes <- [v4i32, v16i8, v16i8]
    var fnTy = LLVMFunctionType(v4i32, fnArgTypes)
    var callArgs <- [arguments[0], wvec, xvec]
    return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "sdot4_w")
}

// aarch64_neon::smmla (i8mm gate): a and b are each a 2x8 int8 matrix (rows 0/1 at bytes 0..7 /
// 8..15); the 2x2 int32 product a*bT accumulates into `acc` laid out [a0b0, a0b1, a1b0, a1b1] —
// 32 int8 MACs per instruction (2x sdot4), no horizontal reduce.
def intrinsic_smmla(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var aptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v16i8, 0u), "")
    var bptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var avec = LLVMBuildLoad2(ctx.builder, v16i8, aptr, "")
    LLVMSetAlignment(avec, 1u)
    var bvec = LLVMBuildLoad2(ctx.builder, v16i8, bptr, "")
    LLVMSetAlignment(bvec, 1u)
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.smmla")
    var declTypes <- [v4i32, v16i8]                 // smmla is overloaded on result + operand vector types
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.smmla (non-ARM / no i8mm target?)")
        return null
    }
    var fnArgTypes2 <- [v4i32, v16i8, v16i8]
    var fnTy2 = LLVMFunctionType(v4i32, fnArgTypes2)
    var callArgs <- [arguments[0], avec, bvec]
    return LLVMBuildCall2(ctx.builder, fnTy2, decl, callArgs, "smmla")
}

// aarch64_neon::sdot4_laneq_w — sdot4_laneq with the interleaved weight operand as a VALUE
// (bitcast in-register, as in sdot4_w); lane splat and const-lane contract as in sdot4_laneq.
def intrinsic_sdot4_laneq_w(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var wvec = LLVMBuildBitCast(ctx.builder, arguments[1], v16i8, "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var xvec = LLVMBuildLoad2(ctx.builder, v16i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    if (LLVMIsAConstantInt(arguments[3]) == null) {
        failed_E(_expr, "aarch64_neon::sdot4_laneq_w: lane must be a compile-time constant")
        return null
    }
    let lane = int(LLVMConstIntGetZExtValue(arguments[3]))
    if (lane < 0 || lane > 3) {
        failed_E(_expr, "aarch64_neon::sdot4_laneq_w: lane must be in 0..3")
        return null
    }
    var xi32 = LLVMBuildBitCast(ctx.builder, xvec, v4i32, "")
    var xspl = LLVMBuildShuffleVector(ctx.builder, ctx.types, xi32, xi32, [lane, lane, lane, lane], "xlane")
    var xbcast = LLVMBuildBitCast(ctx.builder, xspl, v16i8, "")
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.sdot")
    var declTypes <- [v4i32, v16i8]
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.sdot (non-ARM target?)")
        return null
    }
    var fnArgTypes <- [v4i32, v16i8, v16i8]
    var fnTy = LLVMFunctionType(v4i32, fnArgTypes)
    var callArgs <- [arguments[0], wvec, xbcast]
    return LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "sdot4_laneq_w")
}

// v16i8 splat of a small constant (AND mask / shift amount for the nibble ops)
def private splat_v16i8(ctx : JitCtx; v : int) : LLVMOpaqueValue? {
    var elems <- [for (_i in range(16)); LLVMConstInt(ctx.types.t_int8, uint64(v), 0)]
    return LLVMConstVector(array_data_ptr(elems), 16u)
}

// v32i8 twin (the mx4_dequant64 two-block nibble ops)
def private splat_v32i8(ctx : JitCtx; v : int) : LLVMOpaqueValue? {
    var elems <- [for (_i in range(32)); LLVMConstInt(ctx.types.t_int8, uint64(v), 0)]
    return LLVMConstVector(array_data_ptr(elems), 32u)
}

// aarch64_neon::tbl16_lo / tbl16_hi — one @llvm.aarch64.neon.tbl1 nibble-LUT lookup; the int8
// result is bitcast to v4i32 so it flows as a das int4 straight into sdot4_w. The masked/shifted
// index is always 0..15, so TBL's out-of-range-to-zero path never fires.
def intrinsic_tbl16(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var tptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(v16i8, 0u), "")
    var nptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v16i8, 0u), "")
    var tvec = LLVMBuildLoad2(ctx.builder, v16i8, tptr, "")
    LLVMSetAlignment(tvec, 1u)
    var nvec = LLVMBuildLoad2(ctx.builder, v16i8, nptr, "")
    LLVMSetAlignment(nvec, 1u)
    var idx : LLVMOpaqueValue?
    if (_expr.func.name == "tbl16_lo") {
        idx = LLVMBuildAnd(ctx.builder, nvec, splat_v16i8(ctx, 15), "nib.lo")
    } else {
        idx = LLVMBuildLShr(ctx.builder, nvec, splat_v16i8(ctx, 4), "nib.hi")
    }
    let id = LLVMLookupIntrinsicID("llvm.aarch64.neon.tbl1")
    var declTypes <- [v16i8]
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, declTypes)
    if (decl == null) {
        failed_E(_expr, "missing intrinsic llvm.aarch64.neon.tbl1 (non-ARM target?)")
        return null
    }
    var fnArgTypes <- [v16i8, v16i8]
    var fnTy = LLVMFunctionType(v16i8, fnArgTypes)
    var callArgs <- [tvec, idx]
    var tbl = LLVMBuildCall2(ctx.builder, fnTy, decl, callArgs, "tbl16")
    return LLVMBuildBitCast(ctx.builder, tbl, v4i32, "")
}

// x64_avx::dot4 — intrinsic_sdot4's contract on pre-VNNI x86, which has no s8xs8 dot, via the
// exact sign trick: uw = |w| as u8 (@llvm.abs poison-off, so |-128| wraps to bit pattern 0x80 =
// u8 128), sx = PSIGNB(x, w) (x with w's sign, 0 where w is 0), PMADDUBSW(uw, sx) sums u8xs8
// pairs into v8i16 — saturating, but it cannot fire: the dot4 precondition |x| <= 127 bounds every
// pair sum by 128*127*2 = 32512 < 32767 — then PMADDWD(pairs, 1) sums i16 pairs exactly into v4i32.
def intrinsic_dot4_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v8i16 = LLVMVectorType(ctx.types.t_int16, 8u)
    let v4i32 = ctx.types.LLVMInt4Type()
    var wptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v16i8, 0u), "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v16i8, 0u), "")
    var wvec = LLVMBuildLoad2(ctx.builder, v16i8, wptr, "")
    LLVMSetAlignment(wvec, 1u)
    var xvec = LLVMBuildLoad2(ctx.builder, v16i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    // uw = |w| — generic @llvm.abs.v16i8 (lowers to PABSB); is_int_min_poison = false keeps -128 defined
    let abs_id = LLVMLookupIntrinsicID("llvm.abs")
    var absTypes <- [v16i8]
    var abs_decl = LLVMGetIntrinsicDeclaration(g_mod, abs_id, absTypes)
    var absArgTypes <- [v16i8, ctx.types.t_int1]
    var absTy = LLVMFunctionType(v16i8, absArgTypes)
    var absArgs <- [wvec, ctx.types.ConstI1(false)]
    var uw = LLVMBuildCall2(ctx.builder, absTy, abs_decl, absArgs, "dot4.absw")
    // sx = PSIGNB(x, w)
    let psign_id = LLVMLookupIntrinsicID("llvm.x86.ssse3.psign.b.128")
    var noTypes : array<LLVMOpaqueType?>
    var psign_decl = LLVMGetIntrinsicDeclaration(g_mod, psign_id, noTypes)
    if (psign_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.ssse3.psign.b.128 (non-x64 target?)")
        return null
    }
    var b2Types <- [v16i8, v16i8]
    var psignTy = LLVMFunctionType(v16i8, b2Types)
    var psignArgs <- [xvec, wvec]
    var sx = LLVMBuildCall2(ctx.builder, psignTy, psign_decl, psignArgs, "dot4.signx")
    // pairs = PMADDUBSW(uw, sx) : v8i16
    let madd_id = LLVMLookupIntrinsicID("llvm.x86.ssse3.pmadd.ub.sw.128")
    var madd_decl = LLVMGetIntrinsicDeclaration(g_mod, madd_id, noTypes)
    if (madd_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.ssse3.pmadd.ub.sw.128 (non-x64 target?)")
        return null
    }
    var maddTy = LLVMFunctionType(v8i16, b2Types)
    var maddArgs <- [uw, sx]
    var pairs = LLVMBuildCall2(ctx.builder, maddTy, madd_decl, maddArgs, "dot4.pairs")
    // quads = PMADDWD(pairs, splat(1)) : v4i32
    let maddwd_id = LLVMLookupIntrinsicID("llvm.x86.sse2.pmadd.wd")
    var maddwd_decl = LLVMGetIntrinsicDeclaration(g_mod, maddwd_id, noTypes)
    if (maddwd_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.sse2.pmadd.wd (non-x64 target?)")
        return null
    }
    let one16 = ctx.types.ConstI16(int16(1))
    var oneElems <- [one16, one16, one16, one16, one16, one16, one16, one16]
    var ones = LLVMConstVector(array_data_ptr(oneElems), uint(length(oneElems)))
    var w2Types <- [v8i16, v8i16]
    var maddwdTy = LLVMFunctionType(v4i32, w2Types)
    var maddwdArgs <- [pairs, ones]
    var quads = LLVMBuildCall2(ctx.builder, maddwdTy, maddwd_decl, maddwdArgs, "dot4.quads")
    return LLVMBuildAdd(ctx.builder, arguments[0], quads, "dot4")
}

// x64_avx::dot32 — the 256-bit dot4: one full Q8_0 block per call, dot4's exact sign-trick chain
// on ymm, then the v8i32 quads fold to int4 by adding the high 128-bit half onto the low. Lane j
// therefore sums quads j and j+4 — exactly the daslang fallback's grouping, so both paths agree
// bit for bit. Saturation analysis is dot4's, unchanged.
def intrinsic_dot32_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    var folded = emit_dot32_folded(ctx, _expr, arguments[1], arguments[2], false)
    if (folded == null) {
        return null
    }
    return LLVMBuildAdd(ctx.builder, arguments[0], folded, "dot32")
}

// Emitter for x64_avx::dot32_vnni (gated by g_x64_vnni256_intrin_lookup): dot32's chain and fold
// with the VPMADDUBSW+VPMADDWD pair replaced by ONE VPDPBUSD — identical exact int4 result.
def intrinsic_dot32_vnni_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    var folded = emit_dot32_folded(ctx, _expr, arguments[1], arguments[2], true)
    if (folded == null) {
        return null
    }
    return LLVMBuildAdd(ctx.builder, arguments[0], folded, "dot32_vnni")
}

// x64_avx::dot32_ps — dot32's exact chain and fold, then int->float convert (partials bounded
// < 2^24, so exact), scale-multiply, accumulate. Lane for lane the daslang fallback under strict
// FP; _jit_fast_math may contract the mul+add, the same laxity as every float kernel.
def intrinsic_dot32_ps_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    var folded = emit_dot32_folded(ctx, _expr, arguments[1], arguments[2], false)
    if (folded == null) {
        return null
    }
    let v4f32 = LLVMVectorType(ctx.types.t_float, 4u)
    var fp = LLVMBuildSIToFP(ctx.builder, folded, v4f32, "dot32ps.cvt")
    var sv = LLVMGetUndef(v4f32)
    sv = LLVMBuildInsertElement(ctx.builder, sv, arguments[3], ctx.types.ConstI32(0ul), "")
    sv = LLVMBuildShuffleVector(ctx.builder, ctx.types, sv, sv, [0, 0, 0, 0], "dot32ps.splat")
    var scaled = LLVMBuildFMul(ctx.builder, fp, sv, "dot32ps.scale")
    return LLVMBuildFAdd(ctx.builder, arguments[0], scaled, "dot32_ps")
}

// x64_avx::dot32_acc8 — dot32's exact chain WITHOUT the fold: the 8 quads convert to float
// (exact, < 2^24), scale, and accumulate into the caller's 8-float memory accumulator (LICM
// promotes it to a ymm across a block loop — the llama.cpp vec_dot shape: no per-block fold or
// hsum, ONE reduce per row in the caller). The das call is void — the non-null return only marks
// "intrinsic emitted" for make_call.
def intrinsic_dot32_acc8_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot32_acc8(ctx, _expr, arguments, false)
}

// Emitter for x64_avx::dot32_acc8_vnni (gated by g_x64_vnni256_intrin_lookup): the acc8 shape with
// the quads from ONE VPDPBUSD — identical exact quads, identical float tail.
def intrinsic_dot32_acc8_vnni_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot32_acc8(ctx, _expr, arguments, true)
}

def emit_dot32_acc8(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>; use_vnni : bool) : LLVMOpaqueValue? {
    var quads = emit_dot32_quads(ctx, _expr, arguments[1], arguments[2], use_vnni)
    if (quads == null) {
        return null
    }
    let v8f32 = LLVMVectorType(ctx.types.t_float, 8u)
    var accPtr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(v8f32, 0u), "")
    var accVec = LLVMBuildLoad2(ctx.builder, v8f32, accPtr, "acc8.load")
    LLVMSetAlignment(accVec, 4u)
    var fp = LLVMBuildSIToFP(ctx.builder, quads, v8f32, "acc8.cvt")
    var sv = LLVMGetUndef(v8f32)
    sv = LLVMBuildInsertElement(ctx.builder, sv, arguments[3], ctx.types.ConstI32(0ul), "")
    sv = LLVMBuildShuffleVector(ctx.builder, ctx.types, sv, sv, [0, 0, 0, 0, 0, 0, 0, 0], "acc8.splat")
    var scaled = LLVMBuildFMul(ctx.builder, fp, sv, "acc8.scale")
    var sum = LLVMBuildFAdd(ctx.builder, accVec, scaled, "acc8.sum")
    var wrt = LLVMBuildStore(ctx.builder, sum, accPtr)
    LLVMSetAlignment(wrt, 4u)
    return sum
}

// x64_avx::dot32_mx4 — LUT dequant in-register (emit_mx4_dequant_v), then dot32's exact chain +
// fold on the dequantized bytes (|lut| <= 12 bounds PMADDUBSW pair sums to 3072 — no saturation).
// Integer arithmetic throughout, so this and the daslang fallback agree bit for bit.
def intrinsic_dot32_mx4_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot32_mx4(ctx, _expr, arguments, false)
}

// Emitter for x64_avx::dot32_mx4_vnni (gated by g_x64_vnni256_intrin_lookup): the mx4 LUT dequant
// unchanged, the dot step as ONE VPDPBUSD — identical exact int4 result.
def intrinsic_dot32_mx4_vnni_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot32_mx4(ctx, _expr, arguments, true)
}

def emit_dot32_mx4(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>; use_vnni : bool) : LLVMOpaqueValue? {
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    var wvec = emit_mx4_dequant_v(ctx, _expr, arguments[1], arguments[2])
    if (wvec == null) {
        return null
    }
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[3], LLVMPointerType(v32i8, 0u), "")
    var xvec = LLVMBuildLoad2(ctx.builder, v32i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    var quads = emit_dot32_quads_v(ctx, _expr, wvec, xvec, use_vnni)
    if (quads == null) {
        return null
    }
    var qlo = LLVMBuildShuffleVector(ctx.builder, ctx.types, quads, quads, [0, 1, 2, 3], "mx4.lo")
    var qhi = LLVMBuildShuffleVector(ctx.builder, ctx.types, quads, quads, [4, 5, 6, 7], "mx4.hi")
    var folded = LLVMBuildAdd(ctx.builder, qlo, qhi, "mx4.fold")
    return LLVMBuildAdd(ctx.builder, arguments[0], folded, "dot32_mx4")
}

// x64_avx::mx4_dequant32 — the same in-register LUT dequant, stored as one 32-byte write instead
// of feeding the dot chain (fills the mx4 batch kernels' Q8 strip scratch — ~8x fewer ops than the
// scalar loop). Void das call — the non-null return only marks "intrinsic emitted" for make_call.
def intrinsic_mx4_dequant32_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    var wvec = emit_mx4_dequant_v(ctx, _expr, arguments[0], arguments[1])
    if (wvec == null) {
        return null
    }
    var dptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v32i8, 0u), "")
    var wrt = LLVMBuildStore(ctx.builder, wvec, dptr)
    LLVMSetAlignment(wrt, 1u)
    return wvec
}

// The shared MXFP4 LUT-dequant front end: lo/hi nibble indices concat to v32i8 [lo | hi], looked
// up with ONE VPSHUFB against the LUT broadcast to both 128-bit lanes — indices are 0..15 with
// bit 7 clear, so PSHUFB's zeroing path never fires and its per-128-bit-lane semantics match the
// [lo | hi] layout exactly. Returns the dequantized v32i8, or null after failed_E.
def emit_mx4_dequant_v(var ctx : JitCtx; _expr : ExprCallFunc?; lutArg, wnArg : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    var tptr = LLVMBuildPointerCast(ctx.builder, lutArg, LLVMPointerType(v16i8, 0u), "")
    var nptr = LLVMBuildPointerCast(ctx.builder, wnArg, LLVMPointerType(v16i8, 0u), "")
    var tvec = LLVMBuildLoad2(ctx.builder, v16i8, tptr, "mx4.lut")
    LLVMSetAlignment(tvec, 1u)
    var nvec = LLVMBuildLoad2(ctx.builder, v16i8, nptr, "mx4.nib")
    LLVMSetAlignment(nvec, 1u)
    var lo = LLVMBuildAnd(ctx.builder, nvec, splat_v16i8(ctx, 15), "mx4.nib.lo")
    var hi = LLVMBuildLShr(ctx.builder, nvec, splat_v16i8(ctx, 4), "mx4.nib.hi")
    var idx = LLVMBuildShuffleVector(ctx.builder, ctx.types, lo, hi,
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31], "mx4.idx")
    var tbl = LLVMBuildShuffleVector(ctx.builder, ctx.types, tvec, tvec,
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15], "mx4.tbl")
    // w = VPSHUFB(tbl, idx) : v32i8 — both 128-bit lanes index the same 16 LUT bytes
    let shuf_id = LLVMLookupIntrinsicID("llvm.x86.avx2.pshuf.b")
    var noTypes : array<LLVMOpaqueType?>
    var shuf_decl = LLVMGetIntrinsicDeclaration(g_mod, shuf_id, noTypes)
    if (shuf_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.avx2.pshuf.b (non-x64 target?)")
        return null
    }
    var b2Types <- [v32i8, v32i8]
    var shufTy = LLVMFunctionType(v32i8, b2Types)
    var shufArgs <- [tbl, idx]
    return LLVMBuildCall2(ctx.builder, shufTy, shuf_decl, shufArgs, "mx4.w")
}

// dot32's folded partials: emit_dot32_quads + the v8i32 → v4i32 fold (high 128-bit half added onto
// the low — VEXTRACTI128 + VPADDD). Lane j = quads j + j+4. Null propagates from the core.
def emit_dot32_folded(var ctx : JitCtx; _expr : ExprCallFunc?; wArg, xArg : LLVMOpaqueValue?; use_vnni : bool) : LLVMOpaqueValue? {
    var quads = emit_dot32_quads(ctx, _expr, wArg, xArg, use_vnni)
    if (quads == null) {
        return null
    }
    var lo = LLVMBuildShuffleVector(ctx.builder, ctx.types, quads, quads, [0, 1, 2, 3], "dot32.lo")
    var hi = LLVMBuildShuffleVector(ctx.builder, ctx.types, quads, quads, [4, 5, 6, 7], "dot32.hi")
    return LLVMBuildAdd(ctx.builder, lo, hi, "dot32.fold")
}

// The shared dot32 core: loads + sign-trick chain to the raw v8i32 quads (quad j = the dot of bytes
// 4j..4j+3), NO fold. Returns the quads, or null after failed_E on a missing target intrinsic.
def emit_dot32_quads(var ctx : JitCtx; _expr : ExprCallFunc?; wArg, xArg : LLVMOpaqueValue?; use_vnni : bool) : LLVMOpaqueValue? {
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    var wptr = LLVMBuildPointerCast(ctx.builder, wArg, LLVMPointerType(v32i8, 0u), "")
    var xptr = LLVMBuildPointerCast(ctx.builder, xArg, LLVMPointerType(v32i8, 0u), "")
    var wvec = LLVMBuildLoad2(ctx.builder, v32i8, wptr, "")
    LLVMSetAlignment(wvec, 1u)
    var xvec = LLVMBuildLoad2(ctx.builder, v32i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    return emit_dot32_quads_v(ctx, _expr, wvec, xvec, use_vnni)
}

// The dot32 sign-trick chain on already-materialized v32i8 VALUES (w may be an in-register
// dequant result). use_vnni replaces the VPMADDUBSW+VPMADDWD pair with ONE VPDPBUSD — identical
// exact quads, no 16-bit intermediate — reached only through the vnni gate. Returns the raw v8i32
// quads (quad j = the dot of bytes 4j..4j+3), or null after failed_E.
def emit_dot32_quads_v(var ctx : JitCtx; _expr : ExprCallFunc?; wvec, xvec : LLVMOpaqueValue?; use_vnni : bool) : LLVMOpaqueValue? {
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    let v16i16 = LLVMVectorType(ctx.types.t_int16, 16u)
    let v8i32 = LLVMVectorType(ctx.types.t_int32, 8u)
    // uw = |w| — generic @llvm.abs.v32i8 (lowers to VPABSB ymm); is_int_min_poison = false keeps -128 defined
    let abs_id = LLVMLookupIntrinsicID("llvm.abs")
    var absTypes <- [v32i8]
    var abs_decl = LLVMGetIntrinsicDeclaration(g_mod, abs_id, absTypes)
    var absArgTypes <- [v32i8, ctx.types.t_int1]
    var absTy = LLVMFunctionType(v32i8, absArgTypes)
    var absArgs <- [wvec, ctx.types.ConstI1(false)]
    var uw = LLVMBuildCall2(ctx.builder, absTy, abs_decl, absArgs, "dot32.absw")
    // sx = VPSIGNB(x, w)
    let psign_id = LLVMLookupIntrinsicID("llvm.x86.avx2.psign.b")
    var noTypes : array<LLVMOpaqueType?>
    var psign_decl = LLVMGetIntrinsicDeclaration(g_mod, psign_id, noTypes)
    if (psign_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.avx2.psign.b (non-x64 target?)")
        return null
    }
    var b2Types <- [v32i8, v32i8]
    var psignTy = LLVMFunctionType(v32i8, b2Types)
    var psignArgs <- [xvec, wvec]
    var sx = LLVMBuildCall2(ctx.builder, psignTy, psign_decl, psignArgs, "dot32.signx")
    if (use_vnni) {
        // quads = VPDPBUSD(0, uw, sx) : v8i32 — the whole u8×s8 quad dot in one instruction
        let dp_id = LLVMLookupIntrinsicID("llvm.x86.avx512.vpdpbusd.256")
        var dp_decl = LLVMGetIntrinsicDeclaration(g_mod, dp_id, noTypes)
        if (dp_decl == null) {
            failed_E(_expr, "missing intrinsic llvm.x86.avx512.vpdpbusd.256 (non-x64 target?)")
            return null
        }
        var dpArgTypes <- [v8i32, v32i8, v32i8]
        var dpTy = LLVMFunctionType(v8i32, dpArgTypes)
        var dpArgs <- [LLVMConstNull(v8i32), uw, sx]
        return LLVMBuildCall2(ctx.builder, dpTy, dp_decl, dpArgs, "dot32.quads")
    }
    // pairs = VPMADDUBSW(uw, sx) : v16i16
    let madd_id = LLVMLookupIntrinsicID("llvm.x86.avx2.pmadd.ub.sw")
    var madd_decl = LLVMGetIntrinsicDeclaration(g_mod, madd_id, noTypes)
    if (madd_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.avx2.pmadd.ub.sw (non-x64 target?)")
        return null
    }
    var maddTy = LLVMFunctionType(v16i16, b2Types)
    var maddArgs <- [uw, sx]
    var pairs = LLVMBuildCall2(ctx.builder, maddTy, madd_decl, maddArgs, "dot32.pairs")
    // quads = VPMADDWD(pairs, splat(1)) : v8i32
    let maddwd_id = LLVMLookupIntrinsicID("llvm.x86.avx2.pmadd.wd")
    var maddwd_decl = LLVMGetIntrinsicDeclaration(g_mod, maddwd_id, noTypes)
    if (maddwd_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.avx2.pmadd.wd (non-x64 target?)")
        return null
    }
    let one16 = ctx.types.ConstI16(int16(1))
    var oneElems <- [one16, one16, one16, one16, one16, one16, one16, one16,
                     one16, one16, one16, one16, one16, one16, one16, one16]
    var ones = LLVMConstVector(array_data_ptr(oneElems), uint(length(oneElems)))
    var w2Types <- [v16i16, v16i16]
    var maddwdTy = LLVMFunctionType(v8i32, w2Types)
    var maddwdArgs <- [pairs, ones]
    return LLVMBuildCall2(ctx.builder, maddwdTy, maddwd_decl, maddwdArgs, "dot32.quads")
}

def intrinsic_dot64_acc16_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot64_acc16(ctx, _expr, arguments, false)
}

def intrinsic_dot64_acc16_vnni_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    return emit_dot64_acc16(ctx, _expr, arguments, true)
}

// Core for x64_avx::dot64_acc16 / _vnni: TWO Q8 blocks per call on zmm. There is no VPSIGNB at
// 512 bits, so sign application is compare+negate+select — w==0 lanes keep x instead of zeroing,
// but they multiply |w| = 0 anyway, so the quads are unchanged. use_vnni swaps the madd pair for
// ONE VPDPBUSD (saturation headroom is dot32's, unchanged). The quads convert to float (exact,
// < 2^24) and accumulate against the [s0 x8 | s1 x8] scale vector into the caller's 16-float
// memory accumulator. Void das call — the non-null return only marks "intrinsic emitted".
def emit_dot64_acc16(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>; use_vnni : bool) : LLVMOpaqueValue? {
    let v64i8 = LLVMVectorType(ctx.types.t_int8, 64u)
    let v32i16 = LLVMVectorType(ctx.types.t_int16, 32u)
    let v16i32 = LLVMVectorType(ctx.types.t_int32, 16u)
    let v16f32 = LLVMVectorType(ctx.types.t_float, 16u)
    var wptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v64i8, 0u), "")
    var xptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v64i8, 0u), "")
    var wvec = LLVMBuildLoad2(ctx.builder, v64i8, wptr, "")
    LLVMSetAlignment(wvec, 1u)
    var xvec = LLVMBuildLoad2(ctx.builder, v64i8, xptr, "")
    LLVMSetAlignment(xvec, 1u)
    // uw = |w| — generic @llvm.abs.v64i8 (lowers to VPABSB zmm); is_int_min_poison = false keeps -128 defined
    let abs_id = LLVMLookupIntrinsicID("llvm.abs")
    var absTypes <- [v64i8]
    var abs_decl = LLVMGetIntrinsicDeclaration(g_mod, abs_id, absTypes)
    var absArgTypes <- [v64i8, ctx.types.t_int1]
    var absTy = LLVMFunctionType(v64i8, absArgTypes)
    var absArgs <- [wvec, ctx.types.ConstI1(false)]
    var uw = LLVMBuildCall2(ctx.builder, absTy, abs_decl, absArgs, "dot64.absw")
    // sx = x with w's sign (no zeroing — see above): select(w < 0, -x, x)
    var zero64 = LLVMConstNull(v64i8)
    var wneg = LLVMBuildICmp(ctx.builder, LLVMIntPredicate.LLVMIntSLT, wvec, zero64, "dot64.wneg")
    var xneg = LLVMBuildSub(ctx.builder, zero64, xvec, "dot64.negx")
    var sx = LLVMBuildSelect(ctx.builder, wneg, xneg, xvec, "dot64.signx")
    var quads : LLVMOpaqueValue?
    var noTypes : array<LLVMOpaqueType?>
    if (use_vnni) {
        // quads = VPDPBUSD(0, uw, sx) : v16i32
        let dp_id = LLVMLookupIntrinsicID("llvm.x86.avx512.vpdpbusd.512")
        var dp_decl = LLVMGetIntrinsicDeclaration(g_mod, dp_id, noTypes)
        if (dp_decl == null) {
            failed_E(_expr, "missing intrinsic llvm.x86.avx512.vpdpbusd.512 (non-x64 target?)")
            return null
        }
        var dpArgTypes <- [v16i32, v64i8, v64i8]
        var dpTy = LLVMFunctionType(v16i32, dpArgTypes)
        var dpArgs <- [LLVMConstNull(v16i32), uw, sx]
        quads = LLVMBuildCall2(ctx.builder, dpTy, dp_decl, dpArgs, "dot64.quads")
    } else {
        // pairs = VPMADDUBSW(uw, sx) : v32i16 ; quads = VPMADDWD(pairs, splat(1)) : v16i32
        let madd_id = LLVMLookupIntrinsicID("llvm.x86.avx512.pmaddubs.w.512")
        var madd_decl = LLVMGetIntrinsicDeclaration(g_mod, madd_id, noTypes)
        if (madd_decl == null) {
            failed_E(_expr, "missing intrinsic llvm.x86.avx512.pmaddubs.w.512 (non-x64 target?)")
            return null
        }
        var b2Types <- [v64i8, v64i8]
        var maddTy = LLVMFunctionType(v32i16, b2Types)
        var maddArgs <- [uw, sx]
        var pairs = LLVMBuildCall2(ctx.builder, maddTy, madd_decl, maddArgs, "dot64.pairs")
        let maddwd_id = LLVMLookupIntrinsicID("llvm.x86.avx512.pmaddw.d.512")
        var maddwd_decl = LLVMGetIntrinsicDeclaration(g_mod, maddwd_id, noTypes)
        if (maddwd_decl == null) {
            failed_E(_expr, "missing intrinsic llvm.x86.avx512.pmaddw.d.512 (non-x64 target?)")
            return null
        }
        var oneElems <- [for (_j in range(32)); ctx.types.ConstI16(int16(1))]
        var ones = LLVMConstVector(array_data_ptr(oneElems), uint(length(oneElems)))
        var w2Types <- [v32i16, v32i16]
        var maddwdTy = LLVMFunctionType(v16i32, w2Types)
        var maddwdArgs <- [pairs, ones]
        quads = LLVMBuildCall2(ctx.builder, maddwdTy, maddwd_decl, maddwdArgs, "dot64.quads")
    }
    var accPtr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(v16f32, 0u), "")
    var accVec = LLVMBuildLoad2(ctx.builder, v16f32, accPtr, "acc16.load")
    LLVMSetAlignment(accVec, 4u)
    var fp = LLVMBuildSIToFP(ctx.builder, quads, v16f32, "acc16.cvt")
    // scale vector = [s0 x8 | s1 x8]: block 0's quads scale by s0, block 1's by s1
    var sv = LLVMGetUndef(v16f32)
    sv = LLVMBuildInsertElement(ctx.builder, sv, arguments[3], ctx.types.ConstI32(0ul), "")
    sv = LLVMBuildInsertElement(ctx.builder, sv, arguments[4], ctx.types.ConstI32(1ul), "")
    sv = LLVMBuildShuffleVector(ctx.builder, ctx.types, sv, sv, [for (j in range(16)); j / 8], "acc16.splat")
    var scaled = LLVMBuildFMul(ctx.builder, fp, sv, "acc16.scale")
    var sum = LLVMBuildFAdd(ctx.builder, accVec, scaled, "acc16.sum")
    var wrt = LLVMBuildStore(ctx.builder, sum, accPtr)
    LLVMSetAlignment(wrt, 4u)
    return sum
}

// x64_avx::mx4_dequant64 — TWO MXFP4 blocks' nibbles in the zmm index layout [lo0 | hi0 | lo1 |
// hi1] (128-bit lane per 16-byte group), ONE VPSHUFB zmm against the LUT broadcast to all four
// lanes (indices 0..15, bit 7 clear — zeroing never fires, per-lane semantics match the layout),
// one 64-byte store. Byte-for-byte the daslang fallback (two mx4_dequant32 calls). Void das call —
// the non-null return only marks "intrinsic emitted".
def intrinsic_mx4_dequant64_x64(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {   // nolint:LINT014 — table-uniform signature required by g_intrin_lookup
    let v16i8 = LLVMVectorType(ctx.types.t_int8, 16u)
    let v32i8 = LLVMVectorType(ctx.types.t_int8, 32u)
    let v64i8 = LLVMVectorType(ctx.types.t_int8, 64u)
    var tptr = LLVMBuildPointerCast(ctx.builder, arguments[0], LLVMPointerType(v16i8, 0u), "")
    var nptr = LLVMBuildPointerCast(ctx.builder, arguments[1], LLVMPointerType(v32i8, 0u), "")
    var tvec = LLVMBuildLoad2(ctx.builder, v16i8, tptr, "mx4z.lut")
    LLVMSetAlignment(tvec, 1u)
    var nvec = LLVMBuildLoad2(ctx.builder, v32i8, nptr, "mx4z.nib")
    LLVMSetAlignment(nvec, 1u)
    var lo = LLVMBuildAnd(ctx.builder, nvec, splat_v32i8(ctx, 15), "mx4z.nib.lo")
    var hi = LLVMBuildLShr(ctx.builder, nvec, splat_v32i8(ctx, 4), "mx4z.nib.hi")
    // idx byte j: 128-bit lane j/16 alternates lo/hi, advancing 16 source bytes every 2 lanes
    var idx = LLVMBuildShuffleVector(ctx.builder, ctx.types, lo, hi,
        [for (j in range(64)); ((j / 16) % 2 == 0 ? 0 : 32) + (j / 32) * 16 + (j % 16)], "mx4z.idx")
    var tbl = LLVMBuildShuffleVector(ctx.builder, ctx.types, tvec, tvec,
        [for (j in range(64)); j % 16], "mx4z.tbl")
    // w = VPSHUFB(tbl, idx) : v64i8 — all four 128-bit lanes index the same 16 LUT bytes
    let shuf_id = LLVMLookupIntrinsicID("llvm.x86.avx512.pshuf.b.512")
    var noTypes : array<LLVMOpaqueType?>
    var shuf_decl = LLVMGetIntrinsicDeclaration(g_mod, shuf_id, noTypes)
    if (shuf_decl == null) {
        failed_E(_expr, "missing intrinsic llvm.x86.avx512.pshuf.b.512 (non-x64 target?)")
        return null
    }
    var b2Types <- [v64i8, v64i8]
    var shufTy = LLVMFunctionType(v64i8, b2Types)
    var shufArgs <- [tbl, idx]
    var w = LLVMBuildCall2(ctx.builder, shufTy, shuf_decl, shufArgs, "mx4z.w")
    var dptr = LLVMBuildPointerCast(ctx.builder, arguments[2], LLVMPointerType(v64i8, 0u), "")
    var wrt = LLVMBuildStore(ctx.builder, w, dptr)
    LLVMSetAlignment(wrt, 1u)
    return w
}

def intrinsic_math_store_neq_mask(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_gather_store_mask(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    let arg_gather <- [ arguments[1], arguments[2]]
    var gather = intrinsic_math_gather(ctx, expr, arg_gather)
    let arg_store_mask <- [ arguments[0], gather, arguments[3]]
    return intrinsic_math_store_neq_mask(ctx, expr, arg_store_mask)
}

def intrinsic_math_gather_store_stride(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_u8x4_gather_store(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_math_roundi(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var res = intrinsic_math_any_float_op1("round", ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_trunci(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
    var res = intrinsic_math_any_float_op1("trunc", ctx.builder, expr, arguments)
    return LLVMBuildFPToSI(ctx.builder, res, type_to_llvm_abi_type(expr._type), "")
}

def intrinsic_math_sign(var ctx : JitCtx; expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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

def intrinsic_mul_128(var ctx : JitCtx; _expr : ExprCallFunc?; arguments : array<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
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
