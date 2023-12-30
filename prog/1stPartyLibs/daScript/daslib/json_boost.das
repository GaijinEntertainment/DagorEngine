options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module json_boost shared public

require rtti public
require ast
require strings
require daslib/json public
require daslib/ast_boost
require daslib/contracts
require daslib/templates
require daslib/strings_boost
require daslib/apply
require daslib/macro_boost


def operator ?[] ( a : json::JsonValue? == const; key:string ) : JsonValue? const
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? JVNull() : JVNull()

def operator ?[] ( var a : json::JsonValue? == const; key:string ) : JsonValue?
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? JVNull() : JVNull()

def operator ?. ( a : json::JsonValue? == const; key:string ) : JsonValue? const
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? JVNull() : JVNull()

def operator ?. ( var a : json::JsonValue? == const; key:string ) : JsonValue?
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? JVNull() : JVNull()

def operator ?[] ( a : json::JsonValue? == const; idx:int ) : JsonValue? const
    return a != null && a.value is _array ? (a.value as _array)?[idx] ?? JVNull() : JVNull()

def operator ?[] ( var a : json::JsonValue? == const; idx:int ) : JsonValue?
    return a != null && a.value is _array ? (a.value as _array)?[idx] ?? JVNull() : JVNull()

def operator ?? ( a : json::JsonValue?; val:double ) : double
    return a != null && a.value is _number ? a.value as _number : val

def operator ?? ( a : json::JsonValue?; val:float ) : float
    return a != null && a.value is _number ? float(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:int8 ) : int8
    return a != null && a.value is _number ? int8(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:int16 ) : int16
    return a != null && a.value is _number ? int16(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:int ) : int
    return a != null && a.value is _number ? int(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:int64 ) : int64
    return a != null && a.value is _number ? int64(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:uint8 ) : uint8
    return a != null && a.value is _number ? uint8(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:uint16 ) : uint16
    return a != null && a.value is _number ? uint16(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:uint ) : uint
    return a != null && a.value is _number ? uint(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:uint64 ) : uint64
    return a != null && a.value is _number ? uint64(a.value as _number) : val

def operator ?? ( a : json::JsonValue?; val:bool ) : bool
    return a != null && a.value is _bool ? a.value as _bool : val

def operator ?? ( a : json::JsonValue?; val:string ) : string
    return a != null && a.value is _string ? a.value as _string : val


[macro_function]
def private is_json_ptr_value ( td : TypeDeclPtr )
    if td.baseType != Type tPointer
        return false
    if td.firstType == null
        return false
    if td.firstType.baseType != Type tStructure
        return false
    let st = td.firstType.structType
    if st.name!="JsonValue" && st._module.name!="json"
        return false
    return true


// replacing ExprIsVariant(value,name) => ExprIsVariant(value.value,name)
// replacing ExprAsVariant(value,name) => ExprAsVariant(value.value,name)
// if value is json::JsonValue?
[variant_macro(name="better_json")]
class private BetterJsonMacro : AstVariantMacro
    //! This macro is used to implement `is json_value` and `as json_value` runtime checks.
    //! It essencially substitutes `value as name` with `value.value as name` and `value is name` with `value.value is name`.
    def override visitExprIsVariant(prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprIsVariant>) : ExpressionPtr
        if is_json_ptr_value(expr.value._type)
            var inscope vdr <- new [[ExprField() at=expr.at, name:="value", value <- clone_expression(expr.value)]]
            var inscope isv <- new [[ExprIsVariant() at=expr.at, name:=expr.name, value <- vdr ]]
            isv.genFlags = expr.genFlags
            return <- isv
        return <- [[ExpressionPtr]]
    def override visitExprAsVariant(prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprAsVariant>) : ExpressionPtr
        if is_json_ptr_value(expr.value._type)
            var inscope vdr <- new [[ExprField() at=expr.at, name:="value", value <- clone_expression(expr.value)]]
            var inscope isv <- new [[ExprAsVariant() at=expr.at, name:=expr.name, value <- vdr ]]
            isv.genFlags = expr.genFlags
            return <- isv
        return <- [[ExpressionPtr]]
    def override visitExprSafeAsVariant(prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprSafeAsVariant>) : ExpressionPtr
        if is_json_ptr_value(expr.value._type)
            var inscope vdr <- new [[ExprField() at=expr.at, name:="value", value <- clone_expression(expr.value)]]
            var inscope isv <- new [[ExprSafeAsVariant() at=expr.at, name:=expr.name, value <- vdr ]]
            isv.genFlags = expr.genFlags
            return <- isv
        return <- [[ExpressionPtr]]

[reader_macro(name="json")]
class private JsonReader : AstReaderMacro
    //! This macro implements embedding of the JSON object into the program::
    //!   var jsv = %json~
    //!   {
    //!     "name": "main_window",
    //!     "value": 500,
    //!     "size": [1,2,3]
    //!   } %%
    def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
        append(expr.sequence,ch)
        if ends_with(expr.sequence,"%%")
            let len = length(expr.sequence)
            resize(expr.sequence,len-2)
            return false
        else
            return true
    def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
        let seqStr = string(expr.sequence)
        var error = ""
        var json = read_json(seqStr, error)
        macro_verify(json!=null,prog,expr.at,"json did not parse ({error})")
        return <- convert_to_expression(json,expr.at)

[expect_any_enum(ent), template(ent)]
def from_JV ( v:JsonValue explicit?; ent : auto(EnumT); defV : EnumT = [[EnumT]] ) : EnumT
    //! Parse a JSON value and return the corresponding native value.
    if v==null || !(v.value is _string)
        return defV
    let name = v.value as _string
    var res : auto(EnumTT) = [[EnumT]]
    var ti = typeinfo(rtti_typeinfo type<EnumT>)
    for ef in *ti.enumType
        if name==ef.name
            unsafe
                res = reinterpret<EnumTT> (ef.value)
            return res
    panic("not a valid enumeration {name} in {typeinfo(typename type<EnumT>)}")
    return res

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:string; defV:string="" )
    if v==null || !(v.value is _string)
        return defV
    else
        return v.value as _string

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:bool; defV:bool=false )
    if v==null || !(v.value is _bool)
        return defV
    else
        return v.value as _bool

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:float; defV:float=0. )
    if v==null || !(v.value is _number)
        return defV
    else
        return float(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:double; defV:double=0.lf )
    if v==null || !(v.value is _number)
        return defV
    else
        return v.value as _number

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:int; defV:int=0 )
    if v==null || !(v.value is _number)
        return defV
    else
        return int(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:uint; defV:uint=0u )
    if v==null || !(v.value is _number)
        return defV
    else
        return uint(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:int64; defV:int64=0l )
    if v == null
        return defV
    elif v.value is _number
        return int64(v.value as _number)
    elif v.value is _string
        return to_int64(v.value as _string)
    else
        return defV

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:uint64; defV:uint64=0ul )
    if v == null
        return defV
    elif v.value is _number
        return uint64(v.value as _number)
    elif v.value is _string
        return to_uint64(v.value as _string, true)
    else
        return defV

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:int8; defV:int8=int8(0) )
    if v==null || !(v.value is _number)
        return defV
    else
        return int8(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:uint8; defV:uint8=uint8(0) )
    if v==null || !(v.value is _number)
        return defV
    else
        return uint8(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:int16; defV:int16=int16(0) )
    if v==null || !(v.value is _number)
        return defV
    else
        return int16(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:uint16; defV:uint16=uint16(0) )
    if v==null || !(v.value is _number)
        return defV
    else
        return uint16(v.value as _number)

[generic,template(ent)]
def from_JV ( var v : JsonValue explicit?; ent:bitfield; defV:bitfield=[[bitfield]] )
    if v==null || !(v.value is _number)
        return defV
    else
        return bitfield(v.value as _number)

[expect_any_vector_type(v)]
def JV ( v : auto(VecT) )
    //! Creates `JsonValue` out of value.
    static_if typeinfo(vector_dim type<VecT>)==2
        return JV({{"x" => JV(v.x); "y" => JV(v.y)}})
    static_elif typeinfo(vector_dim type<VecT>)==3
        return JV({{"x" => JV(v.x); "y" => JV(v.y); "z" => JV(v.z)}})
    static_elif typeinfo(vector_dim type<VecT>)==4
        return JV({{"x" => JV(v.x); "y" => JV(v.y); "z" => JV(v.z); "w" => JV(v.w)}})

[expect_any_vector_type(ent),template(ent)]
def from_JV ( var v : JsonValue explicit?; ent: auto(VecT); defV:VecT=[[VecT]]  )
    if v==null
        return defV
    let VecV : auto(VecTT) = VecT()
    var vo & = unsafe(v.value as _object)
    static_if typeinfo(vector_dim VecV)==2
        return VecTT(
            vo["x"].value as _number,
            vo["y"].value as _number)
    static_elif typeinfo(vector_dim VecV)==3
        return VecTT(
            vo["x"].value as _number,
            vo["y"].value as _number,
            vo["z"].value as _number)
    static_elif typeinfo(vector_dim VecV)==4
        return VecTT(
            vo["x"].value as _number,
            vo["y"].value as _number,
            vo["z"].value as _number,
            vo["w"].value as _number)

[template(anything)]
def from_JV ( var v:JsonValue explicit?; anything : table< auto(KT);auto(VT) > )
    var ret : table<KT;VT>
    if v==null || !(v.value is _object)
        return <- ret
    unsafe
        var tab & = v.value as _object
        for k,vv in keys(tab),values(tab)
            move_to_ref ( ret[k], _::from_JV(vv, type<VT-&>) )
    return <- ret

def from_JV ( var v:JsonValue explicit?; anything : auto(TT) )
	static_if typeinfo(is_dim anything)
        var ret : TT[typeinfo(dim anything)]
		if v==null || !(v.value is _array)
			return <- ret
		unsafe
			var arr & = v.value as _array
			if arr |> length != typeinfo(dim anything)
				return <- ret
            for i in 0..typeinfo(dim anything)
                move_to_ref ( ret[i], _::from_JV(arr[i], decltype_noref(anything[0])) )
			return <- ret
	static_elif typeinfo(is_struct anything) || typeinfo(is_tuple anything)
		var ret : TT
	    if v==null || !(v.value is _object)
			return <- ret
		apply(ret) <| $ ( name:string; var field )
            move_to_ref ( field, _::from_JV((v as _object)[name], decltype_noref(field)) )
		return <- ret
    static_elif typeinfo(is_array anything)
        var ret : TT
		if v==null || !(v.value is _array)
            return <- ret
		unsafe
			var arr & = v.value as _array
            for i in range(arr |> length)
                if typeinfo(can_copy anything[0])
                    ret |> push_clone <| _::from_JV(arr[i], decltype_noref(anything[0]))
                else
                    ret |> emplace <| _::from_JV(arr[i], decltype_noref(anything[0]))
			return <- ret
    static_elif typeinfo(is_variant anything)
        var ret : TT
        if v==null || !(v.value is _object)
            return <- ret
        var vv : JsonValue? = (v.value as _object)?["$variant"] ?? [[JsonValue?]]
        if vv==null || !(vv.value is _number)
            return <- ret
        unsafe
            set_variant_index(ret, int(vv.value as _number))
		apply(ret) <| $ ( name:string; var field )
            move_to_ref ( field, _::from_JV((v as _object)[name], decltype_noref(field)) )
        return <- ret
	else
        concept_assert(false, "unsupported json serialization type. missing 'fromJV")

def JV ( v : float )
    return new [[JsonValue value <- [[JsValue _number = double(v)]]]]

def JV ( v : int )
    return new [[JsonValue value <- [[JsValue _number = double(v)]]]]

def JV ( v : bitfield )
    return new [[JsonValue value <- [[JsValue _number = double(v)]]]]

def JV(val: int8): JsonValue?
    return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: uint8): JsonValue?
    return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: int16): JsonValue?
    return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: uint16): JsonValue?
    return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: uint): JsonValue?
    return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: int64): JsonValue?
    if val<int64(INT_MIN) || val>int64(INT_MAX)
        return new [[JsonValue value <- [[JsValue _string = "{val}"]]]]
    else
        return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(val: uint64): JsonValue?
    if val > uint64(UINT_MAX)
        return new [[JsonValue value <- [[JsValue _string = "{val}"]]]]
    else
        return new [[JsonValue value <- [[JsValue _number = double(val)]]]]

def JV(value): JsonValue?
    static_if typeinfo(is_dim value)
        var arr: array<JsonValue?>
        for t in value
            arr |> push_clone(JV(t))
        return _::JV(arr)
    static_if typeinfo(typename value) == "json::JsonValue?" || typeinfo(typename value) == "json::JsonValue? const"
        return value
    static_elif typeinfo(is_pointer value)
        return value == null ? [[JsonValue?]] : _::JV(*value)
    static_elif typeinfo(is_enum value)
        return JV("{value}")
    static_elif typeinfo(is_string value)
        return _::JV(value)
    static_elif typeinfo(is_struct value) || typeinfo(is_tuple value)
        var map: table<string; JsonValue?>
        apply(value) <| $(name: string; field)
            static_if typeinfo(is_function field)
                pass // skip functions
            static_elif typeinfo(is_void_pointer field)
                pass // skip void?
            static_elif typeinfo(is_pointer field)
                if field != null
                    map[name] = _::JV(field)
            else
                map[name] = _::JV(field)
        return _::JV(map)
    static_elif typeinfo(is_table value)
        var map: table<string; JsonValue?>
        for k, v in keys(value), values(value)
            map[k] = _::JV(v)
        return _::JV(map)
    static_elif typeinfo(is_iterable value)
        var arr: array<JsonValue?>
        for x in value
            arr |> push_clone <| _::JV(x)
        return _::JV(arr)
    static_elif typeinfo(is_variant value)
        var map: table<string; JsonValue?>
        map["$variant"] = _::JV(variant_index(value))
        apply(value) <| $(name: string; field)
            map[name] = _::JV(field)
        return _::JV(map)
    else
        to_log(LOG_ERROR, "JSON: unsupported type {typeinfo(typename value)}\n")
        concept_assert(false, "unknown JV()")
        return null


def JV(val1; val2): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2)}])

def JV(val1; val2; val3): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3)}])

def JV(val1; val2; val3; val4): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4)}])

def JV(val1; val2; val3; val4; val5): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5)}])

def JV(val1; val2; val3; val4; val5; val6): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5); _::JV(val6)}])

def JV(val1; val2; val3; val4; val5; val6; val7): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5); _::JV(val6); _::JV(val7)}])

def JV(val1; val2; val3; val4; val5; val6; val7; val8): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5); _::JV(val6); _::JV(val7); _::JV(val8)}])

def JV(val1; val2; val3; val4; val5; val6; val7; val8; val9): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5); _::JV(val6); _::JV(val7); _::JV(val8); _::JV(val9)}])

def JV(val1; val2; val3; val4; val5; val6; val7; val8; val9; val10): JsonValue?
    return _::JV([{auto[] _::JV(val1); _::JV(val2); _::JV(val3); _::JV(val4); _::JV(val5); _::JV(val6); _::JV(val7); _::JV(val8); _::JV(val9); _::JV(val10)}])
