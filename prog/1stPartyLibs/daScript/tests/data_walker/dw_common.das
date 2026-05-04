// Shared helpers and data types for DapiDataWalker tests.
//
// Convention: test files `require dw_common` to access LogWalker,
// walk_and_log, and all shared struct / enum / class definitions.

options gen2
options indenting = 4
options no_unused_function_arguments = false
options no_unused_block_arguments = false

module dw_common shared public

require dastest/testing_boost public
require daslib/rtti public
require daslib/debugger public
require strings public


// ============================================================
// LogWalker — logs callback invocations as "NAME:value\n"
// ============================================================

class LogWalker : DapiDataWalker {
    log : string

    // --- scalars ---
    def override Bool(var value : bool&) : void {
        log += "Bool:{value}\n"
    }
    def override Int8(var value : int8&) : void {
        log += "Int8:{int(value)}\n"
    }
    def override UInt8(var value : uint8&) : void {
        log += "UInt8:{int(value)}\n"
    }
    def override Int16(var value : int16&) : void {
        log += "Int16:{int(value)}\n"
    }
    def override UInt16(var value : uint16&) : void {
        log += "UInt16:{int(value)}\n"
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
    def override UInt(var value : uint&) : void {
        log += "UInt:{value}\n"
    }
    def override Int64(var value : int64&) : void {
        log += "Int64:{value}\n"
    }
    def override UInt64(var value : uint64&) : void {
        log += "UInt64:{value}\n"
    }
    def override Float(var value : float&) : void {
        log += "Float:{value}\n"
    }
    def override Double(var value : double&) : void {
        log += "Double:{value}\n"
    }
    def override String(var value : string&) : void {
        log += "String:{value}\n"
    }

    // --- vectors ---
    def override Int2(var value : int2&) : void {
        log += "Int2:{value}\n"
    }
    def override Int3(var value : int3&) : void {
        log += "Int3:{value}\n"
    }
    def override Int4(var value : int4&) : void {
        log += "Int4:{value}\n"
    }
    def override UInt2(var value : uint2&) : void {
        log += "UInt2:{value}\n"
    }
    def override UInt3(var value : uint3&) : void {
        log += "UInt3:{value}\n"
    }
    def override UInt4(var value : uint4&) : void {
        log += "UInt4:{value}\n"
    }
    def override Float2(var value : float2&) : void {
        log += "Float2:{value}\n"
    }
    def override Float3(var value : float3&) : void {
        log += "Float3:{value}\n"
    }
    def override Float4(var value : float4&) : void {
        log += "Float4:{value}\n"
    }

    // --- ranges ---
    def override Range(var value : range&) : void {
        log += "Range:{value}\n"
    }
    def override URange(var value : urange&) : void {
        log += "URange:{value}\n"
    }
    def override Range64(var value : range64&) : void {
        log += "Range64:{value}\n"
    }
    def override URange64(var value : urange64&) : void {
        log += "URange64:{value}\n"
    }

    // --- enum/bitfield ---
    def override WalkEnumeration(var value : int&; ei : EnumInfo) : void {
        log += "Enum:{ei.name}:{value}\n"
    }
    def override Bitfield(var value : uint&; ti : TypeInfo) : void {
        log += "Bitfield:{value}\n"
    }

    // --- structure ---
    def override beforeStructure(ps : void?; si : StructInfo) : void {
        log += "beforeStruct:{si.name}\n"
    }
    def override afterStructure(ps : void?; si : StructInfo) : void {
        log += "afterStruct:{si.name}\n"
    }
    def override beforeStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        log += "beforeField:{vi.name}:{last}\n"
    }
    def override afterStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        log += "afterField:{vi.name}:{last}\n"
    }

    // --- dim (fixed array) ---
    def override beforeDim(ps : void?; ti : TypeInfo) : void {
        log += "beforeDim\n"
    }
    def override afterDim(ps : void?; ti : TypeInfo) : void {
        log += "afterDim\n"
    }

    // --- dynamic array ---
    def override beforeArray(pa : DapiArray; ti : TypeInfo) : void {
        log += "beforeArray\n"
    }
    def override afterArray(pa : DapiArray; ti : TypeInfo) : void {
        log += "afterArray\n"
    }
    def override beforeArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        log += "beforeArrayData:{int(count)}\n"
    }
    def override afterArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        log += "afterArrayData:{int(count)}\n"
    }
    def override beforeArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        log += "beforeElem:{int(index)}:{last}\n"
    }
    def override afterArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        log += "afterElem:{int(index)}:{last}\n"
    }

    // --- table ---
    def override beforeTable(pa : DapiTable; ti : TypeInfo) : void {
        log += "beforeTable\n"
    }
    def override afterTable(pa : DapiTable; ti : TypeInfo) : void {
        log += "afterTable\n"
    }
    def override beforeTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        log += "beforeKey:{int(index)}:{last}\n"
    }
    def override afterTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        log += "afterKey:{int(index)}:{last}\n"
    }
    def override beforeTableValue(pa : DapiTable; ti : TypeInfo; pv : void?; kv : TypeInfo; index : uint; last : bool) : void {
        log += "beforeVal:{int(index)}:{last}\n"
    }
    def override afterTableValue(pa : DapiTable; ti : TypeInfo; pv : void?; kv : TypeInfo; index : uint; last : bool) : void {
        log += "afterVal:{int(index)}:{last}\n"
    }

    // --- tuple ---
    def override beforeTuple(ps : void?; ti : TypeInfo) : void {
        log += "beforeTuple\n"
    }
    def override afterTuple(ps : void?; ti : TypeInfo) : void {
        log += "afterTuple\n"
    }
    def override beforeTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        log += "beforeTupleEntry:{idx}:{last}\n"
    }
    def override afterTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        log += "afterTupleEntry:{idx}:{last}\n"
    }

    // --- variant ---
    def override beforeVariant(ps : void?; ti : TypeInfo) : void {
        log += "beforeVariant\n"
    }
    def override afterVariant(ps : void?; ti : TypeInfo) : void {
        log += "afterVariant\n"
    }

    // --- pointer ---
    def override beforePtr(ps : void?; ti : TypeInfo) : void {
        log += "beforePtr\n"
    }
    def override afterPtr(ps : void?; ti : TypeInfo) : void {
        log += "afterPtr\n"
    }
    def override Null(ti : TypeInfo) : void {
        log += "Null\n"
    }

    // --- lambda ---
    def override beforeLambda(var value : DapiLambda; ti : TypeInfo) : void {
        log += "beforeLambda\n"
    }
    def override afterLambda(var value : DapiLambda; ti : TypeInfo) : void {
        log += "afterLambda\n"
    }
}


// ============================================================
// Shared data types
// ============================================================

struct SimpleStruct {
    x : int = 0
    y : float = 0.0
}

struct NestedStruct {
    inner : SimpleStruct = SimpleStruct()
    tag : string = ""
}

struct ManyFields {
    a : int = 0
    b : int = 0
    c : int = 0
    d : int = 0
}

enum Color {
    Red
    Green
    Blue
}

bitfield Permissions {
    read
    write
    execute
}

typedef MyVariant = variant<i : int; s : string; f : float>

class Animal {
    name : string = ""
}

class Dog : Animal {
    breed : string = ""
}

struct AllScalars {
    b : bool = false
    i : int = 0
    u : uint = 0u
    f : float = 0.0
    d : double = 0.0lf
    s : string = ""
    i8 : int8 = int8(0)
    u8 : uint8 = uint8(0)
    i16 : int16 = int16(0)
    u16 : uint16 = uint16(0)
    i64 : int64 = 0l
    u64 : uint64 = 0ul
}

struct VectorRangeStruct {
    v2 : float2 = float2(0.0, 0.0)
    v3 : float3 = float3(0.0, 0.0, 0.0)
    v4 : float4 = float4(0.0, 0.0, 0.0, 0.0)
    r : range = range(0, 0)
}

struct TreeNode {
    value : int = 0
    children : array<TreeNode>
}
