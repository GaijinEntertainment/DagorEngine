options gen2
module phys_kind_wire shared

require math
require DngPhysKind
require daslib/rtti
require daslib/apply
require daslib/enum_trait

struct private WireOpts {
    bits : int = -1
    hasMin : bool
    minValue : float
    maxValue : float
    step : float
    // the farthest snapshot LOD, Num for none
    reach : PhysKindLod
}

def private ann_float(data : RttiValue) : float {
    if (data is tFloat) {
        return data as tFloat
    }
    return data is tInt ? float(data as tInt) : 0.0
}

def private lod_reach(name : string) : PhysKindLod {
    return name == "none" ? PhysKindLod.Num : to_enum(type<PhysKindLod>, name, PhysKindLod.Min)
}

def private wire_opts(annotations : array<tuple<name : string; data : RttiValue>>; outer_reach : PhysKindLod) : WireOpts {
    var opts = WireOpts(reach = outer_reach)
    for (ann in annotations) {
        if (ann.name == "net_bits" && ann.data is tInt) {
            opts.bits = clamp(ann.data as tInt, 0, 255)
        } elif (ann.name == "net_min") {
            opts.hasMin = true
            opts.minValue = ann_float(ann.data)
        } elif (ann.name == "net_max") {
            opts.maxValue = ann_float(ann.data)
        } elif (ann.name == "net_step") {
            opts.step = ann_float(ann.data)
        } elif (ann.name == "net_lod" && ann.data is tString) {
            let reach = lod_reach(ann.data as tString)
            if (outer_reach != PhysKindLod.Num && (reach == PhysKindLod.Num || int(reach) < int(outer_reach))) {
                opts.reach = reach
            }
        }
    }
    return opts
}

def private is_signed_int(name : string) : bool {
    return name == "int8" || name == "int16" || name == "int"
}

def private is_unsigned_int(name : string) : bool {
    return name == "uint8" || name == "uint16" || name == "uint"
}

def private add_wire_scalar(var fields : array<PhysKindWireField>; base : uint64; value : auto&; opts : WireOpts) {
    var f : PhysKindWireField
    unsafe {
        f.offset = uint8(intptr(addr(value)) - base)
    }
    f.size = uint8(typeinfo sizeof(value))
    f.snapLod = opts.reach
    let typeName = typeinfo stripped_typename(value)
    static_if (typeinfo stripped_typename(value) == "bool") {
        f.codec = PhysKindWireCodec.Int
        f.bits = uint8(1)
    } static_elif (typeinfo is_float(value)) {
        if (opts.step != 0.0) {
            f.codec = PhysKindWireCodec.Step
            f.step = opts.step
        } elif (opts.bits >= 0) {
            f.codec = PhysKindWireCodec.Range
            f.bits = uint8(opts.bits)
            f.max = opts.maxValue
            f.min = opts.hasMin ? opts.minValue : -opts.maxValue
        }
    } else {
        if (opts.bits >= 0 && (is_signed_int(typeName) || is_unsigned_int(typeName))) {
            f.codec = PhysKindWireCodec.Int
            f.bits = uint8(opts.bits)
            f.isSigned = is_signed_int(typeName)
        }
    }
    fields |> push(f)
}

def private add_wire_value(var fields : array<PhysKindWireField>; base : uint64; value : auto&; opts : WireOpts) {
    static_if (typeinfo is_struct(value)) {
        apply(value) $(name : string; field; annotations) {
            add_wire_value(fields, base, field, wire_opts(annotations, opts.reach))
        }
    } static_elif (typeinfo is_dim(value)) {
        for (element in value) {
            add_wire_value(fields, base, element, opts)
        }
    } static_elif (typeinfo is_vector(value)) {
        add_wire_scalar(fields, base, value.x, opts)
        add_wire_scalar(fields, base, value.y, opts)
        static_if (typeinfo vector_dim(value) > 2) {
            add_wire_scalar(fields, base, value.z, opts)
        }
        static_if (typeinfo vector_dim(value) > 3) {
            add_wire_scalar(fields, base, value.w, opts)
        }
    } else {
        add_wire_scalar(fields, base, value, opts)
    }
}

//! The wire fields of kind data T, from the field annotations of T and of its structs:
//! @net_bits, @net_min and @net_max, @net_step, @net_lod (see the phys_kind README)
def phys_kind_wire_fields(data_type : auto(T)) : array<PhysKindWireField> {
    var fields : array<PhysKindWireField>
    unsafe {
        var data : T -const -&
        add_wire_value(fields, intptr(addr(data)), data, WireOpts(reach = PhysKindLod.Min))
    }
    return <- fields
}

//! Codes the state and controls data of a kind by their annotations; false and a logerr for a layout the wire cannot take
def phys_kind_set_wire(var phys : PhysKind; state_type : auto(S); controls_type : auto(C)) : bool {
    var inscope state <- phys_kind_wire_fields(type<S>)
    var inscope controls <- phys_kind_wire_fields(type<C>)
    return phys_kind_set_wire_fields(phys, state, controls)
}
