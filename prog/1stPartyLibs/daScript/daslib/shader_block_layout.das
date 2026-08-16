options gen2

// std140 interface-block layout — the single source of truth shared by every shader backend's
// uniform binder. compute_block_layout walks a daslang struct and yields per-field std140 byte
// offsets / sizes / matrix + composite flags; the SPIR-V emitter (dasSpirv) lays out its interface
// block from it, the OpenGL host binder (dasOpenGL) packs its UBO bytes from it, and dasVulkan's
// host binder writes mapped memory from it — so all rails agree on the byte layout. Pure compile-time
// AST logic (no backend types), which is why it lives in daslib and not in either emitter module.

module shader_block_layout shared public

require math
require daslib/ast

// daslang matrices are handled (tHandle) types named float<W>x<C> — W = column-vector width,
// C = column count (MatrixAnnotation<floatW, C>, column-major: m[i] is column i). Returns the
// dimensions so emit_type / the std140 layout / matrix arithmetic can treat them structurally.
def public matrix_info(t : TypeDecl?) : tuple<ok : bool; width : int; cols : int> {
    if (t == null || t.baseType != Type.tHandle || t.annotation == null) return (ok = false, width = 0, cols = 0)
    let nm = "{t.annotation.name}"
    if (nm == "float4x4") return (ok = true, width = 4, cols = 4)
    if (nm == "float3x3") return (ok = true, width = 3, cols = 3)
    if (nm == "float3x4") return (ok = true, width = 3, cols = 4)
    return (ok = false, width = 0, cols = 0)
}

// The base type a VALUE of this declaration carries: an enum has none of its own and lowers to the
// OpTypeInt of its underlying integer, which the Type-keyed classifiers below cannot reach. A
// 64-bit-based enum peels to tInt64/tUInt64, so arith_width_ok refuses it exactly like a bare one.
def public value_base_type(t : TypeDecl?) : Type {
    if (t == null) return Type.none
    if (t.isEnum && t.enumType != null) return t.enumType.baseType
    return t.baseType
}

// The whole 16/8-bit type set of the shader arc: the lattice proper (contiguous appended
// block in the Type enum) PLUS the pre-lattice small scalars, which live in the enum's old
// region (near tInt) and are NOT covered by the range check.
def public lattice_small_type(bt : Type) : bool {
    return ((int(bt) >= int(Type.tFloat16) && int(bt) <= int(Type.tUByte16))
        || bt == Type.tInt8 || bt == Type.tUInt8 || bt == Type.tInt16 || bt == Type.tUInt16)
}

// The 8/16-lane lattice forms (half8, short8/ushort8, byte8/16, ubyte8/16) are CPU-only by
// design: shader vectors cap at 4 lanes on every target (SPIR-V OpTypeVector, GLSL vecN).
// Both shader emitters key their dedicated fail-closed diagnostic on this predicate.
def public cpu_only_lattice_width(bt : Type) : bool {
    return (bt == Type.tHalf8
        || bt == Type.tShort8 || bt == Type.tUShort8
        || bt == Type.tByte8 || bt == Type.tByte16
        || bt == Type.tUByte8 || bt == Type.tUByte16)
}

// Type -> scalar class: 0 signed int, 1 unsigned int, 2 float — scalars and their shader-legal vectors, INCLUDING the 16/8-bit lattice (scalar float16/int8/uint8/int16/uint16 + 2/3/4-lane half/short/ushort/byte/ubyte vectors); -1 for anything else (8/16-lane forms stay -1: CPU-only).
def public scalar_class(bt : Type) : int {
    if (bt == Type.tInt || bt == Type.tInt2 || bt == Type.tInt3 || bt == Type.tInt4
            || bt == Type.tInt16 || bt == Type.tShort2 || bt == Type.tShort3 || bt == Type.tShort4
            || bt == Type.tInt8 || bt == Type.tByte2 || bt == Type.tByte3 || bt == Type.tByte4) return 0
    if (bt == Type.tUInt || bt == Type.tUInt2 || bt == Type.tUInt3 || bt == Type.tUInt4
            || bt == Type.tUInt16 || bt == Type.tUShort2 || bt == Type.tUShort3 || bt == Type.tUShort4
            || bt == Type.tUInt8 || bt == Type.tUByte2 || bt == Type.tUByte3 || bt == Type.tUByte4) return 1
    if (bt == Type.tFloat || bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4
            || bt == Type.tFloat16 || bt == Type.tHalf2 || bt == Type.tHalf3 || bt == Type.tHalf4) return 2
    return -1
}

// Scalar bit width of a classified type: 32/16/8; 0 for unclassified types. The third axis
// next to scalar_class/component_count — emitters key op/type/layout decisions on it.
def public scalar_width(bt : Type) : int {
    if (bt == Type.tFloat16 || bt == Type.tHalf2 || bt == Type.tHalf3 || bt == Type.tHalf4
            || bt == Type.tInt16 || bt == Type.tShort2 || bt == Type.tShort3 || bt == Type.tShort4
            || bt == Type.tUInt16 || bt == Type.tUShort2 || bt == Type.tUShort3 || bt == Type.tUShort4) return 16
    if (bt == Type.tInt8 || bt == Type.tByte2 || bt == Type.tByte3 || bt == Type.tByte4
            || bt == Type.tUInt8 || bt == Type.tUByte2 || bt == Type.tUByte3 || bt == Type.tUByte4) return 8
    if (scalar_class(bt) >= 0 || bt == Type.tBool) return 32
    return 0
}

// Lane count of a scalar/vector Type: 1/2/3/4; 0 for non-scalar/vector types.
def public component_count(bt : Type) : int {
    if (bt == Type.tInt || bt == Type.tUInt || bt == Type.tFloat || bt == Type.tBool) return 1
    if (bt == Type.tInt2 || bt == Type.tUInt2 || bt == Type.tFloat2) return 2
    if (bt == Type.tInt3 || bt == Type.tUInt3 || bt == Type.tFloat3) return 3
    if (bt == Type.tInt4 || bt == Type.tUInt4 || bt == Type.tFloat4) return 4
    if (bt == Type.tFloat16 || bt == Type.tInt8 || bt == Type.tUInt8 || bt == Type.tInt16 || bt == Type.tUInt16) return 1
    if (bt == Type.tHalf2 || bt == Type.tShort2 || bt == Type.tUShort2 || bt == Type.tByte2 || bt == Type.tUByte2) return 2
    if (bt == Type.tHalf3 || bt == Type.tShort3 || bt == Type.tUShort3 || bt == Type.tByte3 || bt == Type.tUByte3) return 3
    if (bt == Type.tHalf4 || bt == Type.tShort4 || bt == Type.tUShort4 || bt == Type.tByte4 || bt == Type.tUByte4) return 4
    return 0
}

// Type -> (class, component bit WIDTH, lanes), extending the 32-bit classifiers above to the 64-bit
// scalars they exclude (int64/uint64/double, which lay out as block members but carry no arithmetic).
// Width travels with the class so a 32-bit-only consumer can test width, not just class.
def public numeric_info(bt : Type) : tuple<ok : bool; cls : int; width : int; lanes : int> {
    if (bt == Type.tInt64) return (ok = true, cls = 0, width = 64, lanes = 1)
    if (bt == Type.tUInt64) return (ok = true, cls = 1, width = 64, lanes = 1)
    if (bt == Type.tDouble) return (ok = true, cls = 2, width = 64, lanes = 1)
    let c = scalar_class(bt)
    if (c < 0) return (ok = false, cls = -1, width = 0, lanes = 0)
    return (ok = true, cls = c, width = scalar_width(bt), lanes = component_count(bt))
}

// ===== interface-block layout rules =====
enum public BlockLayoutRules {
    std140
    std430
}

// The widths the ARITHMETIC rail lowers: the 16/32-bit families (a native OpIAdd on OpTypeInt 16,
// as glslang gives i16vec2) plus the 64-bit FLOAT (OpFAdd on OpTypeFloat 64, GLSL's double). A
// 64-bit INT is excluded -- it lays out as a block member only, so it fails closed before an opcode.
def public arith_width_ok(cls, width : int) : bool {
    return width <= 32 || (width == 64 && cls == 2)
}

// Class and lane count for the arithmetic rail, gated on arith_width_ok rather than a 32-bit width
// (it carries no byte layout). The 32-bit-only scalar_class / component_count stay as they are, so
// no 16-bit component or double can slip into the std140 layout whose sizes are hardcoded 4-byte.
def public arith_class(bt : Type) : int {
    let ni = numeric_info(bt)
    return ni.ok && arith_width_ok(ni.cls, ni.width) ? ni.cls : -1
}

def public arith_lanes(bt : Type) : int {
    if (bt == Type.tBool) return 1
    let ni = numeric_info(bt)
    return ni.ok && arith_width_ok(ni.cls, ni.width) ? ni.lanes : 0
}

// ===== std140 layout for a UBO interface block =====
// A 64-bit scalar (int64/uint64, GL_ARB_gpu_shader_int64) lays out like a double: align and size 8.
def public std140_align(bt : Type) : int {
    if (bt == Type.tInt64 || bt == Type.tUInt64) return 8
    let cc = component_count(bt)
    if (cc == 1) return 4
    if (cc == 2) return 8
    return 16
}

def public std140_size(bt : Type) : int {
    if (bt == Type.tInt64 || bt == Type.tUInt64) return 8
    return component_count(bt) * 4
}

// Width-aware base size / alignment — the std430 generalization of std140_size/std140_align (identical values for 32-bit types). Size = lanes x bytes-per-lane; alignment = scalar -> 1 lane, vec2 -> 2 lanes, vec3/vec4 -> 4 lanes, scaled by the lane width (half3 -> size 6, align 8;
def public block_scalar_size(bt : Type) : int {
    if (bt == Type.tInt64 || bt == Type.tUInt64) return 8
    return component_count(bt) * (scalar_width(bt) / 8)
}

def public block_scalar_align(bt : Type; rules : BlockLayoutRules) : int {
    if (rules == BlockLayoutRules.std140) return std140_align(bt)
    if (bt == Type.tInt64 || bt == Type.tUInt64) return 8
    let cc = component_count(bt)
    let lanes = cc == 1 ? 1 : (cc == 2 ? 2 : 4)
    return lanes * (scalar_width(bt) / 8)
}

def public round_up(off, a : int) : int {
    return ((off + a - 1) / a) * a
}

//! One std140-laid-out field of a @uniform / @push_constant interface block. Returned by compute_block_layout and consumed by host-side bind macros (e.g. dasVulkan's SpirvVulkanShader and dasOpenGL's GlslOpenGLShader) to emit one write per field at the right byte offset.
struct public BlockField {
    name              : string //!< field name as declared in the daslang struct
    offset            : int    //!< std140 byte offset from the block base
    size              : int    //!< std140 base size of the field (matrices: 16 * cols; composites: total laid-out bytes)
    is_matrix         : bool   //!< true if this field is a matrix (MatrixStride 16 in SPIR-V); see struct docstring re: repack
    matrix_width      : int    //!< matrix column-vector width — 4 for float4x4 (memcpy-safe); 3 for float3x3/3x4 (per-column repack)
    matrix_cols       : int    //!< matrix column count (only valid when is_matrix); 0 otherwise
    is_array          : bool   //!< true if this field is a fixed-array (scalar / vector / nested-struct elements)
    array_count       : int    //!< number of array elements (only valid when is_array)
    array_stride      : int    //!< per-element byte stride (std140 rounds each element up to 16); 0 otherwise
    is_struct         : bool   //!< the struct-element marker. True when the field is a nested struct (`is_array=false`) OR a fixed-array whose elements are nested structs (`is_array=true` — disambiguate via is_array). Host bind macros that handle composites recurse into the inner Structure via compute_block_layout in either case.
    is_composite      : bool   //!< convenience flag — true when is_array OR is_struct (host bind macros can't write this with a flat overload)
}

// Array element stride for a fixed-array element type. std140: scalars/vectors round up to vec4 (16 bytes). std430: tight — the element's base size rounded to its base alignment. Nested-struct elements take their laid-out struct size rounded to the struct alignment (handled by the caller).
def public block_array_element_stride(bt : Type; rules : BlockLayoutRules) : int {
    if (rules == BlockLayoutRules.std140) return round_up(std140_size(bt), 16)
    return round_up(block_scalar_size(bt), block_scalar_align(bt, rules))
}

//! Compute the std140 layout of one interface block. Pure: takes a Structure?, fills `fields` with per-field offsets / sizes / matrix and composite flags, returns the block total size and max member alignment. Used by build_block_struct (SPIR-V type emit), the OpenGL UBO pack binder, and dasVulkan's mapped-memory binder. Rejects bool and non-numeric fields fail-closed (no physical std140 layout). `sname` is only used in error messages. `fields` is cleared at entry so reuse across calls and partial-failure returns can't bleed. Supported field shapes (PR-G complete):
[macro_function]
def public compute_block_layout(sname : string; st : Structure?; var fields : array<BlockField>; var errors : array<string>; rules : BlockLayoutRules = BlockLayoutRules.std140) : tuple<ok : bool; size : int; align : int> {
    fields |> clear
    if (st == null || empty(st.fields)) {
        errors |> push("interface block '{sname}' must be a struct with at least one field")
        return (ok = false, size = 0, align = 0)
    }
    var off = 0
    var max_align = 0
    for (fld in st.fields) {
        // matrix field (terminal)
        let mi = matrix_info(fld._type)
        if (mi.ok) {
            off = round_up(off, 16)
            fields |> emplace(BlockField(name := string(fld.name), offset = off, size = 16 * mi.cols,
                is_matrix = true, matrix_width = mi.width, matrix_cols = mi.cols))
            off += 16 * mi.cols
            max_align = max(max_align, 16)
            continue
        }
        // nested struct field (PR-G): recurse. Base alignment = round_up(inner_align, 16);
        if (fld._type != null && fld._type.baseType == Type.tStructure && fld._type.structType != null) {
            var inner : array<BlockField>
            let inner_layout = compute_block_layout("{sname}.{fld.name}", fld._type.structType, inner, errors, rules)
            delete inner
            if (!inner_layout.ok) {
                fields |> clear
                return (ok = false, size = 0, align = 0)
            }
            let struct_align = rules == BlockLayoutRules.std140 ? round_up(inner_layout.align, 16) : inner_layout.align
            off = round_up(off, struct_align)
            let struct_size = round_up(inner_layout.size, struct_align)
            fields |> emplace(BlockField(name := string(fld.name), offset = off, size = struct_size,
                is_matrix = false, matrix_width = 0, matrix_cols = 0,
                is_array = false, array_count = 0, array_stride = 0,
                is_struct = true, is_composite = true))
            off += struct_size
            max_align = max(max_align, struct_align)
            continue
        }
        // fixed-array field (PR-G): element kind decides stride. Scalar/vector elements stride by their size rounded up to 16 (the std140 array padding rule — every array slot is at least vec4-wide). Nested-struct elements stride by their laid-out struct size. Matrix elements and nested-array elements (array of array) are rejected; wrap in a struct.
        if (fld._type != null && fld._type.baseType == Type.tFixedArray && fld._type.firstType != null) {
            let et = fld._type.firstType
            let nelem = fld._type.fixedDim
            if (nelem <= 0) {
                errors |> push("interface block '{sname}' field '{fld.name}': array must have positive count (got {nelem})")
                fields |> clear
                return (ok = false, size = 0, align = 0)
            }
            // scalar / vector element
            if (et.baseType != Type.tStructure && et.baseType != Type.tFixedArray && !matrix_info(et).ok) {
                let ebt = et.baseType
                if (scalar_class(ebt) < 0 || (rules == BlockLayoutRules.std140 && scalar_width(ebt) != 32)) {
                    errors |> push("interface block '{sname}' field '{fld.name}': array element type {ebt} is not supported (only 32-bit int/uint/float scalars, their vectors, or nested structs of those)")
                    fields |> clear
                    return (ok = false, size = 0, align = 0)
                }
                let stride = block_array_element_stride(ebt, rules)
                let abase = rules == BlockLayoutRules.std140 ? 16 : block_scalar_align(ebt, rules)
                off = round_up(off, abase)     // std140 array base alignment is 16; std430 = element alignment
                let total = stride * nelem
                fields |> emplace(BlockField(name := string(fld.name), offset = off, size = total,
                    is_matrix = false, matrix_width = 0, matrix_cols = 0,
                    is_array = true, array_count = nelem, array_stride = stride,
                    is_struct = false, is_composite = true))
                off += total
                max_align = max(max_align, abase)
                continue
            }
            // array-of-struct element
            if (et.baseType == Type.tStructure && et.structType != null) {
                var inner : array<BlockField>
                let inner_layout = compute_block_layout("{sname}.{fld.name}[]", et.structType, inner, errors, rules)
                delete inner
                if (!inner_layout.ok) {
                    fields |> clear
                    return (ok = false, size = 0, align = 0)
                }
                let struct_align = rules == BlockLayoutRules.std140 ? round_up(inner_layout.align, 16) : inner_layout.align
                let stride = round_up(inner_layout.size, struct_align)
                off = round_up(off, struct_align)
                let total = stride * nelem
                fields |> emplace(BlockField(name := string(fld.name), offset = off, size = total,
                    is_matrix = false, matrix_width = 0, matrix_cols = 0,
                    is_array = true, array_count = nelem, array_stride = stride,
                    is_struct = true, is_composite = true))
                off += total
                max_align = max(max_align, struct_align)
                continue
            }
            errors |> push("interface block '{sname}' field '{fld.name}': array element type {et.baseType} is not supported (only scalar/vector or nested-struct elements; matrix-of-array and array-of-array are out of scope)")
            fields |> clear
            return (ok = false, size = 0, align = 0)
        }
        // terminal scalar / vector field (32-bit numerics, or a 64-bit int64/uint64 scalar)
        let fbt = fld._type.baseType
        let is_int64 = fbt == Type.tInt64 || fbt == Type.tUInt64
        if (!is_int64 && (scalar_class(fbt) < 0 || (rules == BlockLayoutRules.std140 && scalar_width(fbt) != 32))) {
            errors |> push("interface block '{sname}' field '{fld.name}': type {fbt} is not supported (only 32-bit int/uint/float scalars, their vectors, matrices, fixed-arrays of those, 64-bit int64/uint64 scalars, and nested structs of those; bool has no physical interface-block layout in Vulkan)")
            fields |> clear
            return (ok = false, size = 0, align = 0)
        }
        let falign = block_scalar_align(fbt, rules)
        off = round_up(off, falign)
        fields |> emplace(BlockField(name := string(fld.name), offset = off, size = block_scalar_size(fbt),
            is_matrix = false, matrix_width = 0, matrix_cols = 0))
        off += block_scalar_size(fbt)
        max_align = max(max_align, falign)
    }
    return (ok = true, size = off, align = max_align)
}

//! True iff the block has a physical std140 layout — i.e. compute_block_layout succeeds. That covers every supported field shape: 32-bit scalar / vector / matrix (float4x4 flat, float3x3 / float3x4 per-column), nested structs, and fixed-arrays of those. False only for blocks with a bool or other non-numeric field (no std140 layout). A backend uses this to choose the std140 UBO pack path vs a loose by-name fallback for a given @uniform block, and the emitter + host binder MUST agree by both consulting it.
[macro_function]
def public std140_block_is_packable(st : Structure?) : bool {
    var fields : array<BlockField>
    var errors : array<string>
    let layout = compute_block_layout("", st, fields, errors)
    delete fields
    delete errors
    return layout.ok
}

//! One std140 leaf write produced by std140_block_writes: a scalar / vector value (a plain field, a matrix COLUMN, or an array ELEMENT) and its std140 byte offset within the block. A backend's bind macro emits one memcpy-style write per entry — GL std140_set, dasVulkan write_field — each copying sizeof(value) bytes. No backend needs matrix / array / nesting knowledge: the decomposition lives in std140_block_writes, so every rail packs the same bytes by construction.
struct public Std140Write {
    offset : int             //!< std140 byte offset of this leaf within the block
    access : ExpressionPtr   //!< expression yielding the scalar/vector leaf value (field / column / element)
}

//! Decompose an interface block into a flat list of leaf writes (Std140Write) — the single shared std140 packing walk for every shader backend's host binder. float3x3 / float3x4 split into per-column writes (column c at offset + c*16; daslang stores width-wide packed columns, std140 pads each to a vec4, and the per-column memcpy gives that padding for free); float4x4 stays one whole-matrix write (its columns are already vec4-wide so a single memcpy is std140-correct); fixed-arrays split per element at the std140 stride; nested structs recurse; array-of-struct recurses per element. `access` is the accessor for the block instance (e.g. an ExprVar of the @uniform global), cloned per leaf;
[macro_function]
def public std140_block_writes(st : Structure?; access : ExpressionPtr; baseOffset : int; var out : array<Std140Write>) {
    var fields : array<BlockField>
    var errors : array<string>
    compute_block_layout("", st, fields, errors)
    for (i in range(length(fields))) {
        let f = fields[i]
        let ftype = st.fields[i]._type
        let off = baseOffset + f.offset
        var faccess = new ExprField(at = access.at, value <- clone_expression(access), name := st.fields[i].name)
        if (f.is_matrix && f.matrix_width != 4) {
            out |> reserve(length(out) + f.matrix_cols)
            for (c in range(f.matrix_cols)) {
                var col = new ExprAt(at = access.at, subexpr <- clone_expression(faccess),
                    index <- new ExprConstInt(at = access.at, value = c))
                out |> push(Std140Write(offset = off + c * 16, access = col))
            }
        } elif (f.is_array && f.is_struct) {
            for (e in range(f.array_count)) {
                var el = new ExprAt(at = access.at, subexpr <- clone_expression(faccess),
                    index <- new ExprConstInt(at = access.at, value = e))
                std140_block_writes(ftype.firstType.structType, el, off + e * f.array_stride, out)
            }
        } elif (f.is_array) {
            out |> reserve(length(out) + f.array_count)
            for (e in range(f.array_count)) {
                var el = new ExprAt(at = access.at, subexpr <- clone_expression(faccess),
                    index <- new ExprConstInt(at = access.at, value = e))
                out |> push(Std140Write(offset = off + e * f.array_stride, access = el))
            }
        } elif (f.is_struct) {
            std140_block_writes(ftype.structType, faccess, off, out)
        } else {
            out |> push(Std140Write(offset = off, access = faccess))
        }
    }
    delete fields
    delete errors
}
