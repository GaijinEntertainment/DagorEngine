options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const
options unsafe_table_lookup = false
options no_global_variables = false
// the type factory is exact-type dispatch over the das Type lattice - one arm per base type is the shape
options _cyclomatic_complexity = 0
options _function_length = 0

module llvm_jit_di shared private

require llvm/daslib/llvm_boost
require llvm/bindings/llvm_func
require llvm/bindings/llvm_struct
require llvm/bindings/llvm_enum
require daslib/ast_boost
require daslib/fio
require math

//! DWARF type and expression factory for the JIT's debug-info rail.

let public DW_TAG_STRUCTURE_TYPE = 0x13u

let public DW_ATE_BOOLEAN = 0x02u
let public DW_ATE_FLOAT = 0x04u
let public DW_ATE_SIGNED = 0x05u
let public DW_ATE_SIGNED_CHAR = 0x06u
let public DW_ATE_UNSIGNED = 0x07u

let public DW_OP_DEREF = 0x06ul

[extern(cdecl, name="LLVMDIBuilderCreateStructType", library="LLVM.dll")]
def private llvm_di_create_struct_type(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit;
                                       Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit;
                                       LineNumber : uint; SizeInBits : uint64; AlignInBits : uint; Flags : LLVMDIFlags;
                                       DerivedFrom : LLVMOpaqueMetadata? implicit; Elements : LLVMOpaqueMetadata ?? implicit;
                                       NumElements : uint; RunTimeLang : uint; VTableHolder : LLVMOpaqueMetadata? implicit;
                                       UniqueId : string implicit; UniqueIdLen : uint64) : LLVMOpaqueMetadata? {}

var private g_di_cache_owner : LLVMOpaqueDIBuilder?
var private g_di_cache : table<uint64; LLVMOpaqueMetadata?>
var private g_di_files : table<string; LLVMOpaqueMetadata?>
var private g_di_composites_in_progress : table<uint64; bool>

var private g_di_module : LLVMOpaqueModule?
var private g_di_builder : LLVMOpaqueDIBuilder?

[arch(at="../ARCHITECTURE_DEBUG_INFO.md#di-one-cu-per-module")]
def public di_begin_module(mod : LLVMOpaqueModule?) : LLVMOpaqueDIBuilder? {
    return g_di_builder if (mod == g_di_module && g_di_builder != null)
    di_finalize()
    g_di_module = mod
    g_di_builder = LLVMCreateDIBuilder(mod)
    let cu_name = "das-jit"
    let producer = "daslang jit"
    let cu_file = LLVMDIBuilderCreateFile(g_di_builder, cu_name, uint64(long_length(cu_name)), "", 0ul)
    LLVMDIBuilderCreateCompileUnit(g_di_builder, LLVMDWARFSourceLanguage.C_plus_plus, cu_file,
        producer, uint64(long_length(producer)), 0, "", 0ul, 0u, "", 0ul,
        LLVMDWARFEmissionKind.LLVMDWARFEmissionFull, 0u, 0, 0, "", 0ul, "", 0ul)
    di_reset_type_cache(g_di_builder)
    return g_di_builder
}

//! Called once per module, after the last function is emitted and before anything reads it.
def public di_finalize {
    return if (g_di_builder == null)
    LLVMDIBuilderFinalize(g_di_builder)
    LLVMDisposeDIBuilder(g_di_builder)
    g_di_builder = null
    g_di_module = null
    di_reset_type_cache(null)
}

def private di_reset_type_cache(dib : LLVMOpaqueDIBuilder?) {
    g_di_cache_owner = dib
    g_di_cache |> clear()
    g_di_files |> clear()
    g_di_composites_in_progress |> clear()
}

[arch(at="../ARCHITECTURE_DEBUG_INFO.md#di-one-cu-per-module")]
def private di_cache_drop_if_stale(dib : LLVMOpaqueDIBuilder?) {
    if (g_di_cache_owner != dib) {
        di_reset_type_cache(dib)
    }
}

def public di_file_of(dib : LLVMOpaqueDIBuilder?; filename : string) : LLVMMetadataRef {
    return null if (dib == null)
    di_cache_drop_if_stale(dib)
    let hit = unsafe(g_di_files?[filename] ?? null)
    return hit if (hit != null)
    var full = filename |> empty() ? "unknown_filename" : get_full_file_name(filename)
    if (full |> empty()) {
        full = filename
    }
    let node = LLVMDIBuilderCreateFile(dib, full, uint64(long_length(full)), "", 0ul)
    g_di_files[filename] = node
    return node
}

def private di_basic(dib : LLVMOpaqueDIBuilder?; name : string; bytes : int; encoding : uint) : LLVMMetadataRef {
    return LLVMDIBuilderCreateBasicType(dib, name, uint64(long_length(name)),
        uint64(bytes) * 8ul, encoding, LLVMDIFlags.LLVMDIFlagZero)
}

def private di_vector_of(dib : LLVMOpaqueDIBuilder?; elem : LLVMMetadataRef; count : int; bytes : int; align : int) : LLVMMetadataRef {
    var subs <- array<LLVMMetadataRef>(LLVMDIBuilderGetOrCreateSubrange(dib, 0l, int64(count)))
    return LLVMDIBuilderCreateVectorType(dib, uint64(bytes) * 8ul, uint(align) * 8u,
        elem, array_data_ptr(subs), 1u)
}

def private di_scalar_of(dib : LLVMOpaqueDIBuilder?; bt : Type) : LLVMMetadataRef {
    return di_basic(dib, "int", 4, DW_ATE_SIGNED) if (bt == Type.tInt)
    return di_basic(dib, "uint", 4, DW_ATE_UNSIGNED) if (bt == Type.tUInt || bt == Type.tBitfield)
    return di_basic(dib, "int8", 1, DW_ATE_SIGNED) if (bt == Type.tInt8)
    return di_basic(dib, "uint8", 1, DW_ATE_UNSIGNED) if (bt == Type.tUInt8 || bt == Type.tBitfield8)
    return di_basic(dib, "int16", 2, DW_ATE_SIGNED) if (bt == Type.tInt16)
    return di_basic(dib, "uint16", 2, DW_ATE_UNSIGNED) if (bt == Type.tUInt16 || bt == Type.tBitfield16)
    return di_basic(dib, "int64", 8, DW_ATE_SIGNED) if (bt == Type.tInt64)
    return di_basic(dib, "uint64", 8, DW_ATE_UNSIGNED) if (bt == Type.tUInt64 || bt == Type.tBitfield64)
    return di_basic(dib, "float", 4, DW_ATE_FLOAT) if (bt == Type.tFloat)
    return di_basic(dib, "double", 8, DW_ATE_FLOAT) if (bt == Type.tDouble)
    return di_basic(dib, "float16", 2, DW_ATE_FLOAT) if (bt == Type.tFloat16)
    return di_basic(dib, "bool", 1, DW_ATE_BOOLEAN) if (bt == Type.tBool)
    return null
}

def private di_lane_shape(bt : Type) : tuple<elem : Type; count : int> {
    return (elem = Type.tInt, count = 2) if (bt == Type.tInt2 || bt == Type.tRange)
    return (elem = Type.tInt, count = 3) if (bt == Type.tInt3)
    return (elem = Type.tInt, count = 4) if (bt == Type.tInt4)
    return (elem = Type.tUInt, count = 2) if (bt == Type.tUInt2 || bt == Type.tURange)
    return (elem = Type.tUInt, count = 3) if (bt == Type.tUInt3)
    return (elem = Type.tUInt, count = 4) if (bt == Type.tUInt4)
    return (elem = Type.tInt64, count = 2) if (bt == Type.tRange64)
    return (elem = Type.tUInt64, count = 2) if (bt == Type.tURange64)
    return (elem = Type.tFloat, count = 2) if (bt == Type.tFloat2)
    return (elem = Type.tFloat, count = 3) if (bt == Type.tFloat3)
    return (elem = Type.tFloat, count = 4) if (bt == Type.tFloat4)
    return (elem = Type.tFloat16, count = 2) if (bt == Type.tHalf2)
    return (elem = Type.tFloat16, count = 3) if (bt == Type.tHalf3)
    return (elem = Type.tFloat16, count = 4) if (bt == Type.tHalf4)
    return (elem = Type.tFloat16, count = 8) if (bt == Type.tHalf8)
    return (elem = Type.tInt16, count = 2) if (bt == Type.tShort2)
    return (elem = Type.tInt16, count = 3) if (bt == Type.tShort3)
    return (elem = Type.tInt16, count = 4) if (bt == Type.tShort4)
    return (elem = Type.tInt16, count = 8) if (bt == Type.tShort8)
    return (elem = Type.tUInt16, count = 2) if (bt == Type.tUShort2)
    return (elem = Type.tUInt16, count = 3) if (bt == Type.tUShort3)
    return (elem = Type.tUInt16, count = 4) if (bt == Type.tUShort4)
    return (elem = Type.tUInt16, count = 8) if (bt == Type.tUShort8)
    return (elem = Type.tInt8, count = 2) if (bt == Type.tByte2)
    return (elem = Type.tInt8, count = 3) if (bt == Type.tByte3)
    return (elem = Type.tInt8, count = 4) if (bt == Type.tByte4)
    return (elem = Type.tInt8, count = 8) if (bt == Type.tByte8)
    return (elem = Type.tInt8, count = 16) if (bt == Type.tByte16)
    return (elem = Type.tUInt8, count = 2) if (bt == Type.tUByte2)
    return (elem = Type.tUInt8, count = 3) if (bt == Type.tUByte3)
    return (elem = Type.tUInt8, count = 4) if (bt == Type.tUByte4)
    return (elem = Type.tUInt8, count = 8) if (bt == Type.tUByte8)
    return (elem = Type.tUInt8, count = 16) if (bt == Type.tUByte16)
    return (elem = Type.none, count = 0)
}

def private di_enum_entry_value(e : ExpressionPtr) : tuple<value : int64; known : bool> {
    return (value = int64((e as ExprConstInt).value), known = true) if (e is ExprConstInt)
    return (value = int64((e as ExprConstInt8).value), known = true) if (e is ExprConstInt8)
    return (value = int64((e as ExprConstInt16).value), known = true) if (e is ExprConstInt16)
    return (value = (e as ExprConstInt64).value, known = true) if (e is ExprConstInt64)
    return (value = int64((e as ExprConstUInt).value), known = true) if (e is ExprConstUInt)
    return (value = int64((e as ExprConstUInt8).value), known = true) if (e is ExprConstUInt8)
    return (value = int64((e as ExprConstUInt16).value), known = true) if (e is ExprConstUInt16)
    return (value = int64((e as ExprConstUInt64).value), known = true) if (e is ExprConstUInt64)
    return (value = 0l, known = false)
}

def private di_enumeration(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; t : TypeDeclPtr) : LLVMMetadataRef {
    let en = t.enumType
    let ename = en != null ? "{en.name}" : "enum"
    var entries : array<LLVMMetadataRef>
    let unsigned = t.flags.enumStubIsUnsigned || t.isUnsignedInteger
    if (en != null) {
        for (ee in en.list) {
            let folded = di_enum_entry_value(ee.value)
            continue if (!folded.known)
            let een = "{ee.name}"
            entries |> push <| LLVMDIBuilderCreateEnumerator(dib, een, uint64(long_length(een)), folded.value, unsigned ? 1 : 0)
        }
    }
    let base = di_scalar_of(dib, t.baseSizeOf == 8 ? Type.tInt64 : (t.baseSizeOf == 2 ? Type.tInt16 : (t.baseSizeOf == 1 ? Type.tInt8 : Type.tInt)))
    return LLVMDIBuilderCreateEnumerationType(dib, file, ename, uint64(long_length(ename)),
        file, en != null ? en.at.line : 0u, uint64(t.sizeOf) * 8ul, uint(t.alignOf) * 8u,
        array_data_ptr(entries), uint(length(entries)), base)
}

def private di_opaque(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; name : string; bytes : int; align : int) : LLVMMetadataRef {
    return LLVMDIBuilderCreateForwardDecl(dib, DW_TAG_STRUCTURE_TYPE, name, uint64(long_length(name)),
        file, file, 0u, 0u, uint64(bytes) * 8ul, uint(align) * 8u, name, uint64(long_length(name)))
}

def private di_pointer_to(dib : LLVMOpaqueDIBuilder?; pointee : LLVMMetadataRef; name : string) : LLVMMetadataRef {
    return LLVMDIBuilderCreatePointerType(dib, pointee, uint64(typeinfo sizeof(type<void?>)) * 8ul,
        uint(typeinfo alignof(type<void?>)) * 8u, 0u, name, uint64(long_length(name)))
}

def private di_member(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; scope : LLVMMetadataRef;
                      mname : string; moffset : int; msize : int; malign : int; mty : LLVMMetadataRef) : LLVMMetadataRef {
    return LLVMDIBuilderCreateMemberType(dib, scope, mname, uint64(long_length(mname)),
        file, 0u, uint64(msize) * 8ul, uint(malign) * 8u, uint64(moffset) * 8ul,
        LLVMDIFlags.LLVMDIFlagZero, mty)
}

[arch(at="../ARCHITECTURE_DEBUG_INFO.md#di-composite-types")]
def private di_members_of(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef;
                          t : TypeDeclPtr; var out : array<LLVMMetadataRef>) {
    if (t.isStructure && t.structType != null) {
        for (fld in t.structType.fields) {
            out |> push <| di_member(dib, file, file, "{fld.name}", fld.offset,
                fld._type.sizeOf, fld._type.alignOf, di_type_of(dib, file, fld._type))
        }
    } elif (t.isTuple || t.isVariant) {
        let is_variant = t.isVariant
        if (is_variant) {
            out |> push <| di_member(dib, file, file, "__tag", 0, 4, 4, di_scalar_of(dib, Type.tInt))
        }
        var offset = is_variant ? max(4, t.variantAlign) : 0
        for (at, an, ai in t.argTypes, t.argNames, count()) {
            let aname = an |> empty() ? "_{ai}" : "{an}"
            let aalign = at.alignOf
            if (!is_variant) {
                offset = (offset + aalign - 1) & -aalign
            }
            out |> push <| di_member(dib, file, file, aname, offset, at.sizeOf, aalign, di_type_of(dib, file, at))
            if (!is_variant) {
                offset += at.sizeOf
            }
        }
    }
}

[arch(at="../ARCHITECTURE_DEBUG_INFO.md#di-composite-types")]
def private di_composite(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; t : TypeDeclPtr; name : string) : LLVMMetadataRef {
    let mnh = t.get_mnh
    let line = t.isStructure && t.structType != null ? t.structType.at.line : 0u
    g_di_composites_in_progress[mnh] = true
    var members : array<LLVMMetadataRef>
    di_members_of(dib, file, t, members)
    g_di_composites_in_progress |> erase(mnh)
    return llvm_di_create_struct_type(dib, file, name, uint64(long_length(name)), file, line,
        uint64(t.sizeOf) * 8ul, uint(t.alignOf) * 8u, LLVMDIFlags.LLVMDIFlagZero, null,
        array_data_ptr(members), uint(length(members)), 0u, null, name, uint64(long_length(name)))
}

def private di_fixed_array(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; t : TypeDeclPtr) : LLVMMetadataRef {
    var subs : array<LLVMMetadataRef>
    var leaf = t
    while (leaf.baseType == Type.tFixedArray && leaf.firstType != null) {
        subs |> push <| LLVMDIBuilderGetOrCreateSubrange(dib, 0l, int64(leaf.fixedDim))
        leaf = leaf.firstType
    }
    return LLVMDIBuilderCreateArrayType(dib, uint64(t.sizeOf) * 8ul, uint(t.alignOf) * 8u,
        di_type_of(dib, file, leaf), array_data_ptr(subs), uint(length(subs)))
}

def private di_pointee_of(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; pt : TypeDeclPtr) : LLVMMetadataRef {
    return null if (pt == null || pt.isVoid)
    if (g_di_composites_in_progress?[pt.get_mnh] ?? false) {
        return di_opaque(dib, file, "{describe(pt)}", max(pt.sizeOf, 1), max(pt.alignOf, 1))
    }
    return di_type_of(dib, file, pt)
}

def public di_type_of(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef; t : TypeDeclPtr) : LLVMMetadataRef {
    return null if (dib == null || t == null || t.isVoid)
    di_cache_drop_if_stale(dib)
    let mnh = t.get_mnh
    let hit = unsafe(g_di_cache?[mnh] ?? null)
    return hit if (hit != null)
    var res : LLVMMetadataRef
    if (t.baseType == Type.tDistinct && t.firstType != null) {
        res = di_type_of(dib, file, t.firstType)
    } elif (t.baseType == Type.tFixedArray) {
        res = di_fixed_array(dib, file, t)
    } elif (t.isEnum || t.isEnumT) {
        res = di_enumeration(dib, file, t)
    } elif (t.isString) {
        res = di_pointer_to(dib, di_basic(dib, "char", 1, DW_ATE_SIGNED_CHAR), "string")
    } elif (t.isPointer) {
        res = di_pointer_to(dib, di_pointee_of(dib, file, t.firstType), "{describe(t)}")
    } elif (t.isStructure || t.isTuple || t.isVariant) {
        res = di_composite(dib, file, t, "{describe(t)}")
    } elif (t.isHandle) {
        res = di_opaque(dib, file, t.annotation != null ? "{t.annotation.name}" : "handle", t.sizeOf, t.alignOf)
    } elif (t.baseType == Type.tArray || t.baseType == Type.tTable || t.baseType == Type.tIterator ||
            t.baseType == Type.tBlock || t.baseType == Type.tFunction || t.baseType == Type.tLambda) {
        res = di_opaque(dib, file, "{describe(t)}", t.sizeOf, t.alignOf)
    } else {
        res = di_scalar_of(dib, t.baseType)
        if (res == null) {
            let lanes = di_lane_shape(t.baseType)
            if (lanes.count != 0) {
                res = di_vector_of(dib, di_scalar_of(dib, lanes.elem), lanes.count, t.sizeOf, t.alignOf)
            }
        }
        if (res == null) {
            res = di_opaque(dib, file, "{describe(t)}", max(t.sizeOf, 1), max(t.alignOf, 1))
        }
    }
    g_di_cache[mnh] = res
    return res
}

def public di_subroutine_of(dib : LLVMOpaqueDIBuilder?; file : LLVMMetadataRef;
                            arguments : dasvector`ptr`Variable; result : TypeDeclPtr) : LLVMMetadataRef {
    var types : array<LLVMMetadataRef>
    types |> push <| di_type_of(dib, file, result)
    for (a in arguments) {
        var ty = di_type_of(dib, file, a._type)
        if (a._type != null && a._type.isRef && ty != null) {
            ty = di_pointer_to(dib, ty, "")
        }
        types |> push <| ty
    }
    return LLVMDIBuilderCreateSubroutineType(dib, file, array_data_ptr(types),
        uint(length(types)), LLVMDIFlags.LLVMDIFlagZero)
}

def public di_empty_expression(dib : LLVMOpaqueDIBuilder?) : LLVMMetadataRef {
    return LLVMDIBuilderCreateExpression(dib, null, 0ul)
}

def public di_deref_expression(dib : LLVMOpaqueDIBuilder?) : LLVMMetadataRef {
    var ops <- array<uint64>(DW_OP_DEREF)
    return LLVMDIBuilderCreateExpression(dib, array_data_ptr(ops), 1ul)
}
