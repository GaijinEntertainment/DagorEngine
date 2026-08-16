options gen2
options indenting = 4
options strict_smart_pointers = true

module json_boost shared public

//! JSON ↔ struct automatic serialization.
//!
//! Generates ``from_JsValue`` and ``to_JsValue`` conversions for user
//! structs using compile-time reflection. Also provides the ``%json~``
//! reader macro for embedding JSON literals in source code.
//! Requires the ``json`` module.

require daslib/rtti public
require daslib/ast
require strings
require daslib/json public
require daslib/ast_boost
require daslib/contracts
require daslib/templates
require daslib/strings_boost
require daslib/apply
require daslib/macro_boost
require daslib/static_let
require daslib/enum_trait
require daslib/constant_expression

def operator ?[] (a : json::JsonValue const? ==const; key : string) : JsonValue? const {
    //! Returns the value of the key in the JSON object, if it exists.
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? json_null_sentinel() : json_null_sentinel()
}

def operator ?[] (var a : json::JsonValue? ==const; key : string) : JsonValue? {
    //! Returns the value of the key in the JSON object, if it exists.
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? json_null_sentinel() : json_null_sentinel()
}

def operator ?.(a : json::JsonValue const? ==const; key : string) : JsonValue? const {
    //! Returns the value of the key in the JSON object, if it exists.
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? json_null_sentinel() : json_null_sentinel()
}

def operator ?.(var a : json::JsonValue? ==const; key : string) : JsonValue? {
    //! Returns the value of the key in the JSON object, if it exists.
    return a != null && a.value is _object ? (a.value as _object)?[key] ?? json_null_sentinel() : json_null_sentinel()
}

def operator ?[] (a : json::JsonValue const? ==const; idx : int) : JsonValue? const {
    //! Returns the value of the index in the JSON array, if it exists.
    return a != null && a.value is _array ? (a.value as _array)?[idx] ?? json_null_sentinel() : json_null_sentinel()
}

def operator ?[] (var a : json::JsonValue? ==const; idx : int) : JsonValue? {
    //! Returns the value of the index in the JSON array, if it exists.
    return a != null && a.value is _array ? (a.value as _array)?[idx] ?? json_null_sentinel() : json_null_sentinel()
}

def private null_coalescing(a : json::JsonValue const?; val : auto(TT)) : TT {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    if (a != null) {
        if (a.value is _number) {
            return TT(a.value as _number)   // nolint:PERF020 generic -- cast redundant for TT=double, needed for other TT
        } elif (a.value is _longint) return TT(a.value as _longint)
    }
    return val
}

def private null_coalescing_with_string(a : json::JsonValue const?; val : auto(TT)) : TT {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    if (a != null) {
        if (a.value is _number) {
            return TT(a.value as _number)
        } elif (a.value is _longint) {
            return TT(a.value as _longint)   // nolint:PERF020 generic -- cast redundant for TT=int64, needed for other TT
        } elif (a.value is _string) return TT(a.value as _string)
    }
    return val
}

def operator ??(a : json::JsonValue const?; val : double) : double {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : float) : float {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : int8) : int8 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : int16) : int16 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : int) : int {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : int64) : int64 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing_with_string(a, val)
}

def operator ??(a : json::JsonValue const?; val : uint8) : uint8 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : uint16) : uint16 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : uint) : uint {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing(a, val)
}

def operator ??(a : json::JsonValue const?; val : uint64) : uint64 {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return null_coalescing_with_string(a, val)
}

def operator ??(a : json::JsonValue const?; val : bool) : bool {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return a != null && a.value is _bool ? a.value as _bool : val
}

def operator ??(a : json::JsonValue const?; val : string) : string {
    //! Returns the value of the JSON object, if it exists, otherwise returns the default value.
    return a != null && a.value is _string ? a.value as _string : val
}

def operator ?. value(a : JsonValue const? ==const) : JsValue const? {
    //! Returns the value of the JSON object, if it exists.
    return a . ?. value
}

def operator ?. value(var a : JsonValue? ==const) : JsValue? {
    //! Returns the value of the JSON object, if it exists.
    return a . ?. value
}

[macro_function]
def private is_json_ptr_value(td : TypeDeclPtr) {
    //! Checks if the type is a pointer to json::JsonValue
    if (td.baseType != Type.tPointer || td.firstType == null || td.firstType.baseType != Type.tStructure) return false
    let st = td.firstType.structType
    return st.name == "JsonValue" && st._module.name == "json"
}


// replacing ExprIsVariant(value,name) => ExprIsVariant(value.value,name)
// replacing ExprAsVariant(value,name) => ExprAsVariant(value.value,name)
// if value is json::JsonValue?
[variant_macro(name="better_json")]
class private BetterJsonMacro : AstVariantMacro {
    //! This macro is used to implement `is json_value` and `as json_value` runtime checks.
    //! It essentially substitutes `value as name` with `value.value as name` and `value is name` with `value.value is name`.
    def override visitExprIsVariant(prog : ProgramPtr; mod : Module?; expr : ExprIsVariant?) : ExpressionPtr {
        //! Visit ExprIsVariant node.
        if (is_json_ptr_value(expr.value._type)) {
            var vdr = new ExprField(at = expr.at, name := "value", value <- clone_expression(expr.value))
            var isv = new ExprIsVariant(at = expr.at, name := expr.name, value <- vdr)
            isv.genFlags = expr.genFlags
            return <- isv
        }
        return null
    }
    def override visitExprAsVariant(prog : ProgramPtr; mod : Module?; expr : ExprAsVariant?) : ExpressionPtr {
        //! Visit ExprAsVariant node.
        if (is_json_ptr_value(expr.value._type)) {
            var vdr = new ExprField(at = expr.at, name := "value", value <- clone_expression(expr.value))
            var isv = new ExprAsVariant(at = expr.at, name := expr.name, value <- vdr)
            isv.genFlags = expr.genFlags
            return <- isv
        }
        return null
    }
    def override visitExprSafeAsVariant(prog : ProgramPtr; mod : Module?; expr : ExprSafeAsVariant?) : ExpressionPtr {
        //! Visit ExprSafeAsVariant node.
        if (is_json_ptr_value(expr.value._type)) {
            var vdr = new ExprField(at = expr.at, name := "value", value <- clone_expression(expr.value))
            var isv = new ExprSafeAsVariant(at = expr.at, name := expr.name, value <- vdr)
            isv.genFlags = expr.genFlags
            return <- isv
        }
        return null
    }
}

[reader_macro(name="json")]
class private JsonReader : AstReaderMacro {
    //! This macro embeds a JSON object into the program.
    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        //! Implement the accept method to read until '%%' is found.
        append(expr.sequence, ch)
        if (ends_with(expr.sequence, "%%")) {
            let len = length(expr.sequence)
            resize(expr.sequence, len - 2)
            return false
        } else {
            return true
        }
    }
    def override visit(prog : ProgramPtr; mod : Module?; expr : ExprReader?) : ExpressionPtr {
        //! Visit the ExprReader node and parse the JSON content.
        let seqStr = string(expr.sequence)
        var error = ""
        var json = read_json(seqStr, error)
        macro_verify(json != null, prog, expr.at, "json did not parse ({error})")
        return <- convert_to_expression(json, expr.at)
    }
}

[expect_any_enum(ent), template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : auto(EnumT); defV : EnumT = default<EnumT>) : EnumT {
    //! Parse a JSON value and return the corresponding native value.
    if (v == null || !((v.value is _string) || (v.value is _number) || (v.value is _longint))) return defV
    if (v.value is _string) {
        let name = v.value as _string
        let ti = typeinfo rtti_typeinfo(type<EnumT>)
        for (ef in *ti.enumType) {
            return unsafe(reinterpret<EnumT>(ef.value)) if (name == ef.name)
        }
        panic("not a valid enumeration {name} in {typeinfo typename(type<EnumT>)}")
    } else {
        let intValue = (v.value is _number) ? int(v.value as _number) : int(v.value as _longint)
        for (it in defV) {
            if (intValue == int(it)) return it
        }
        panic("not a valid enumeration {intValue} in {typeinfo typename(type<EnumT>)}")
    }
    return defV
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : string; defV : string = "") {
    //! Parse a JSON value and return the corresponding native value.
    if (v == null || !(v.value is _string)) {
        return defV
    } else {
        return v.value as _string
    }
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : bool; defV : bool = false) {
    //! Parse a JSON value and return the corresponding native value.
    if (v == null || !(v.value is _bool)) {
        return defV
    } else {
        return v.value as _bool
    }
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : float; defV : float = 0.) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : double; defV : double = 0.lf) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : int; defV : int = 0) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : uint; defV : uint = 0u) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : int64; defV : int64 = 0l) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing_with_string(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : uint64; defV : uint64 = 0ul) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing_with_string(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : int8; defV : int8 = int8(0)) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : uint8; defV : uint8 = uint8(0)) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : int16; defV : int16 = int16(0)) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : uint16; defV : uint16 = uint16(0)) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : bitfield; defV : bitfield = bitfield()) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : bitfield : uint8 < >; defV : bitfield8 = bitfield8()) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : bitfield : uint16 < >; defV : bitfield16 = bitfield16()) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[generic, template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : bitfield : uint64 < >; defV : bitfield64 = bitfield64()) {
    //! Parse a JSON value and return the corresponding native value.
    return null_coalescing(v, defV)
}

[expect_any_vector_type(v)]
def JV(v : auto(VecT)) {
    //! Creates `JsonValue` out of value.
    static_if (typeinfo vector_dim(type<VecT>) == 2) {
        return JV({"x" => JV(v.x), "y" => JV(v.y)})
    } static_elif (typeinfo vector_dim(type<VecT>) == 3) {
        return JV({"x" => JV(v.x), "y" => JV(v.y), "z" => JV(v.z)})
    } static_elif (typeinfo vector_dim(type<VecT>) == 4) {
        return JV({"x" => JV(v.x), "y" => JV(v.y), "z" => JV(v.z), "w" => JV(v.w)})
    }
}

[expect_any_vector_type(ent), template(ent), unused_argument(ent)]
def from_JV(v : JsonValue const explicit?; ent : auto(VecT); defV : VecT = VecT()) {
    //! Parse a JSON value and return the corresponding vector value. Accepts both the object form ``{"x":1,"y":2}`` (what ``JV(v)`` emits) and the array form ``[1,2]`` (what ``sprint_json`` / ``debug_json_value`` emit for the same vector). Missing or short arrays fall back to ``defV`` field-by-field.
    if (v == null) return defV
    if (v.value is _array) {
        unsafe {
            let arr & = v.value as _array
            let n = length(arr)
            static_if (typeinfo vector_dim(defV) == 2) {
                return VecT(
                    n > 0 ? (arr[0] ?? defV.x) : defV.x,
                    n > 1 ? (arr[1] ?? defV.y) : defV.y)
            } static_elif (typeinfo vector_dim(defV) == 3) {
                return VecT(
                    n > 0 ? (arr[0] ?? defV.x) : defV.x,
                    n > 1 ? (arr[1] ?? defV.y) : defV.y,
                    n > 2 ? (arr[2] ?? defV.z) : defV.z)
            } static_elif (typeinfo vector_dim(defV) == 4) {
                return VecT(
                    n > 0 ? (arr[0] ?? defV.x) : defV.x,
                    n > 1 ? (arr[1] ?? defV.y) : defV.y,
                    n > 2 ? (arr[2] ?? defV.z) : defV.z,
                    n > 3 ? (arr[3] ?? defV.w) : defV.w)
            }
        }
    }
    assume vo = v
    static_if (typeinfo vector_dim(defV) == 2) {
        return VecT(vo?["x"] ?? defV.x, vo?["y"] ?? defV.y)
    } static_elif (typeinfo vector_dim(defV) == 3) {
        return VecT(vo?["x"] ?? defV.x, vo?["y"] ?? defV.y, vo?["z"] ?? defV.z)
    } static_elif (typeinfo vector_dim(defV) == 4) {
        return VecT(vo?["x"] ?? defV.x, vo?["y"] ?? defV.y, vo?["z"] ?? defV.z, vo?["w"] ?? defV.w)
    }
}

[template(anything), unused_argument(anything)]
def from_JV(v : JsonValue const explicit?; anything : table< auto(KT); auto(VT) >) {
    //! Parse a JSON value and return the corresponding table value.
    var ret : table<KT; VT>
    if (v == null || !(v.value is _object)) return <- ret
    unsafe {
        let tab & = v.value as _object
        for (k, vv in keys(tab), values(tab)) {
            // VT can be scalar or struct/table — keep from_JV uniform; ?? has
            // no overload for non-scalar VT.
            move_to_ref(ret[k], _::from_JV(vv, type<VT -&>))  // nolint:STYLE020
        }
    }
    return <- ret
}

struct public JsonFieldState {
    //! Per-field serialization options for JSON struct conversion.
    argName : string        //! name of the field in JSON
    enumAsInt : bool        //! whether to parse enum as integer
    unescape : bool         //! whether to unescape strings
    embed : bool            //! whether to embed the field
    optional : bool         //! whether the field is optional
}

def public parse_json_annotation(name : string; annotation : array<tuple<name : string; data : RttiValue>>) : JsonFieldState {
    //! Parse JSON field annotations and return the corresponding JsonFieldState.
    var fieldState : JsonFieldState
    fieldState.argName = name
    for (ann in annotation) {
        if (ann.name == "rename") {
            if (ann.data is tString) {
                fieldState.argName = ann.data as tString
            } elif (ann.data is tBool && !empty(name) && first_character(name) == '_') {
                fieldState.argName = slice(name, 1)
            }
        } elif (ann.name == "enum_as_int" && ann.data is tBool) {
            fieldState.enumAsInt = ann.data as tBool
        } elif (ann.name == "unescape" && ann.data is tBool) {
            fieldState.unescape = ann.data as tBool
        } elif (ann.name == "embed" && ann.data is tBool) {
            fieldState.embed = ann.data as tBool
        } elif (ann.name == "optional" && ann.data is tBool) {
            fieldState.optional = ann.data as tBool
        }
    }
    return <- fieldState
}

[unused_argument(anything)]
def from_JV(v : JsonValue const explicit?; anything : auto(TT)) {
    //! Parse a JSON value and return the corresponding value of any type.
    //! This is the main dispatch function that handles various types.
    static_if (typeinfo is_dim(anything)) {
        var ret : TT -const -&   // TT IS the matched fixed-array type
        if (v == null || !(v.value is _array)) return <- ret
        unsafe {
            let arr & = v.value as _array
            if (arr |> length != typeinfo dim(anything)) return <- ret
            for (i in 0..typeinfo dim(anything)) {
                move_to_ref(ret[i], _::from_JV(arr[i], decltype_noref(anything[0])))
            }
            return <- ret
        }
    } static_elif (typeinfo is_struct(anything)) {
        var ret <- default<TT -const>
        if (v == null || !(v.value is _object)) return <- ret
        apply(ret) $(name : string; var field; annotations) {
            static_let("{typeinfo typename(type<TT-const-&>)}`{name}") {
                var fieldState : JsonFieldState
                var fieldStateParsed : bool = false
            }
            if (!fieldStateParsed) {
                fieldState <- parse_json_annotation(name, annotations)
                fieldStateParsed = true
            }
            (v as _object) |> get(fieldState.argName) $(val) {
                move_to_ref(field, _::from_JV(val, decltype_noref(field)))
            }
        }
        return <- ret
    } static_elif (typeinfo is_tuple(anything)) {
        var ret <- default<TT -const>
        if (v == null || !(v.value is _object)) return <- ret
        apply(ret) $(name : string; var field) {
            (v as _object) |> get(name) $(val) {
                move_to_ref(field, _::from_JV(val, decltype_noref(field)))
            }
        }
        return <- ret
    } static_elif (typeinfo is_array(anything)) {
        var ret : TT
        if (v == null || !(v.value is _array)) return <- ret
        unsafe {
            let arr & = v.value as _array
            ret |> reserve(arr |> long_length)
            for (a in arr) {
                static_if (typeinfo can_copy(anything[0])) {
                    ret |> push_clone <| _::from_JV(a, decltype_noref(anything[0]))
                } else {
                    ret |> emplace <| _::from_JV(a, decltype_noref(anything[0]))
                }
            }
            return <- ret
        }
    } static_elif (typeinfo is_variant(anything)) {
        var ret : TT
        if (v == null || !(v.value is _object)) return <- ret
        let vv : JsonValue? = (v.value as _object)?["$variant"] ?? default<JsonValue?>
        if (vv == null || !((vv.value is _number) || (vv.value is _longint))) return <- ret
        let index = (vv.value is _number) ? int(vv.value as _number) : int(vv.value as _longint)
        unsafe(set_variant_index(ret, index))
        apply(ret) $(name : string; var field) {
            (v as _object) |> get(name) $(val) {
                move_to_ref(field, _::from_JV(val, decltype_noref(field)))
            }
        }
        return <- ret
    } else {
        concept_assert(false, "unsupported json serialization type. missing 'fromJV")
    }
}

def JV(value : auto(TT)) : JsonValue? {
    //! Creates `JsonValue` out of value.
    //! This is the main dispatch function that handles various types.
    static_if (typeinfo is_dim(value)) {
        var arr : array<JsonValue?>
        arr |> reserve(length(value))
        for (t in value) {
            arr |> push(JV(t))
        }
        return _::JV(arr)
    }
    static_if (typeinfo typename(value) == "json::JsonValue?" || typeinfo typename(value) == "json::JsonValue? const") {
        // Pass-through: JsonValue? IS a JsonValue?. Const-strip the pointer
        // so callers walking const containers (apply on a const struct/tuple
        // hands fields as const) round-trip cleanly.
        unsafe {
            return reinterpret<JsonValue?>(value)
        }
    } static_elif (typeinfo is_pointer(value)) {
        return value == null ? default<JsonValue?> : _::JV(*value)
    } static_elif (typeinfo is_enum(value)) {
        return JV("{value}")
    } static_elif (typeinfo is_string(value)) {
        return _::JV(value)
    } static_elif (typeinfo is_struct(value)) {
        var map : table<string; JsonValue?>
        apply(value) $(name : string; field; annotations) {
            static_if (typeinfo is_function(field)) {
                pass // skip functions
            } static_elif (typeinfo is_void_pointer(field)) {
                pass // skip void?
            } static_elif (typeinfo is_pointer(field)) {
                if (field != null) {
                    static_let("{typeinfo typename(type<TT-const-&>)}`{name}") {
                        var fieldState : JsonFieldState
                        var fieldStateParsed : bool = false
                    }
                    if (!fieldStateParsed) {
                        fieldState <- parse_json_annotation(name, annotations)
                        fieldStateParsed = true
                    }
                    map |> insert(fieldState.argName, _::JV(field))
                }
            } else {
                static_let("{typeinfo typename(type<TT-const-&>)}`{name}") {
                    var fieldState : JsonFieldState
                    var fieldStateParsed : bool = false
                }
                if (!fieldStateParsed) {
                    fieldState <- parse_json_annotation(name, annotations)
                    fieldStateParsed = true
                }
                // No `return` to skip/replace a field: a `return` in the apply block routes the whole walk to the slow per-field invoke fallback (PR #3008). Pivot enum/string handling on a static type guard with an explicit default `else`, and gate @optional with `if (!skip)`, so the block stays return-free and inlines.
                var skip = false   // nolint:LINT003 — written only in static_if branches; some instantiations write, some don't
                if (fieldState.optional) {
                    // skip @optional fields at default (zero) value, matching sprint_json behavior
                    static_if (typeinfo is_string(field)) {
                        skip = empty(field)
                    } static_elif (typeinfo is_array(field)) {
                        skip = empty(field)
                    } static_elif (typeinfo is_table(field)) {
                        skip = empty(field)
                    } static_elif (typeinfo is_pointer(field)) {
                        skip = field == null
                    } else {
                        static_if (typeinfo is_workhorse(field)) {
                            // bool path uses `!field` (avoids STYLE018 on `bool == false` after
                            // generic instantiation); numeric workhorses keep the generic-zero compare.
                            static_if (typeinfo stripped_typename(field) == "bool") {
                                skip = !field
                            } else {
                                skip = field == default<typedecl(field)>
                            }
                        }
                    }
                }
                if (!skip) {
                    if (fieldState.enumAsInt) {     // we only need to handle it as we are writing out, reading handles it at the enum level
                        static_if (typeinfo is_enum(field)) {
                            let fieldStateEnumValue : int = int(field)
                            map |> insert(fieldState.argName, _::JV(fieldStateEnumValue))
                        } else {
                            map |> insert(fieldState.argName, _::JV(field))
                        }
                    } elif (fieldState.unescape) {
                        static_if (typeinfo is_string(field)) {
                            map |> insert(fieldState.argName, _::JV(unescape(field)))
                        } else {
                            map |> insert(fieldState.argName, _::JV(field))
                        }
                    } elif (fieldState.embed) {
                        static_if (typeinfo is_string(field)) {
                            var errors : string
                            var subexpr = read_json(field, errors)
                            if (subexpr == null) {
                                to_log(LOG_ERROR, "JSON: failed to parse embedded json field '{fieldState.argName}': {errors}\n")
                                panic("failed to parse embedded json field '{fieldState.argName}': {errors}")
                            }
                            map |> insert(fieldState.argName, subexpr)
                        } else {
                            map |> insert(fieldState.argName, _::JV(field))
                        }
                    } else {
                        map |> insert(fieldState.argName, _::JV(field))
                    }
                }
            }
        }
        return _::JV(map)
    } static_elif (typeinfo is_tuple(value)) {
        var map : table<string; JsonValue?>
        apply(value) $(name : string; field) {
            static_if (typeinfo is_function(field)) {
                pass // skip functions
            } static_elif (typeinfo is_void_pointer(field)) {
                pass // skip void?
            } static_elif (typeinfo is_pointer(field)) {
                if (field != null) {
                    map |> insert(name, _::JV(field))
                }
            } else {
                map |> insert(name, _::JV(field))
            }
        }
        return _::JV(map)
    } static_elif (typeinfo is_table(value)) {
        var map : table<string; JsonValue?>
        for (k, v in keys(value), values(value)) {
            map |> insert(k, _::JV(v))
        }
        return _::JV(map)
    } static_elif (typeinfo is_iterable(value)) {
        var arr : array<JsonValue?>
        static_if (typeinfo is_array(value)) {
            arr |> reserve(long_length(value))
        } static_elif (typeinfo is_dim(value)) {
            arr |> reserve(typeinfo dim(value))
        }
        for (x in value) {
            arr |> push <| _::JV(x)
        }
        return _::JV(arr)
    } static_elif (typeinfo is_variant(value)) {
        var map : table<string; JsonValue?>
        map |> insert("$variant", _::JV(variant_index(value)))
        apply(value) $(name : string; field) {
            map |> insert(name, _::JV(field))
        }
        return _::JV(map)
    } else {
        to_log(LOG_ERROR, "JSON: unsupported type {typeinfo typename(value)}\n")
        concept_assert(false, "unknown JV()")
        return null
    }
}

def JV(val1; val2) : JsonValue? {
    //! Creates array of two JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2)])
}

def JV(val1; val2; val3) : JsonValue? {
    //! Creates array of three JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3)])
}

def JV(val1; val2; val3; val4) : JsonValue? {
    //! Creates array of four JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4)])
}

def JV(val1; val2; val3; val4; val5) : JsonValue? {
    //! Creates array of five JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5)])
}

def JV(val1; val2; val3; val4; val5; val6) : JsonValue? {
    //! Creates array of six JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5), _::JV(val6)])
}

def JV(val1; val2; val3; val4; val5; val6; val7) : JsonValue? {
    //! Creates array of seven JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5), _::JV(val6), _::JV(val7)])
}

def JV(val1; val2; val3; val4; val5; val6; val7; val8) : JsonValue? {
    //! Creates array of eight JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5), _::JV(val6), _::JV(val7), _::JV(val8)])
}

def JV(val1; val2; val3; val4; val5; val6; val7; val8; val9) : JsonValue? {
    //! Creates array of nine JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5), _::JV(val6), _::JV(val7), _::JV(val8), _::JV(val9)])
}

def JV(val1; val2; val3; val4; val5; val6; val7; val8; val9; val10) : JsonValue? {
    //! Creates array of ten JsonValues.
    return _::JV([ _::JV(val1), _::JV(val2), _::JV(val3), _::JV(val4), _::JV(val5), _::JV(val6), _::JV(val7), _::JV(val8), _::JV(val9), _::JV(val10)])
}

// ===== linq_fold JSON source =====

// By-name materialize of one JSON object into a struct TT: read each top-level field by name (the from_json query contract — like from_xml_node), so it never calls an overridable whole-row from_JV.
[unused_argument(t)]
def from_json_row(jv : JsonValue const explicit?; t : auto(TT)) : TT {
    //! Materialize one JSON object into *TT* **by name** — the per-element worker behind ``from_json``.
    static_if (typeinfo is_struct(type<TT>)) {
        var ret <- default<TT -const>
        if (jv.value is _object) {
            apply_imm(ret) $(name : string; var field) {
                field = read_json_field(jv, name, field)
            }
        }
        return <- ret
    } static_elif (typeinfo is_string(type<TT>)) {
        return jv.value ?as _string ?? default<TT -const>   // ?as: one .value access, payload type == TT
    } static_elif (typeinfo stripped_typename(type<TT>) == "bool") {
        return jv.value ?as _bool ?? default<TT -const>
    } static_elif (typeinfo is_enum(type<TT>)) {
        return from_JV(jv, type<TT>)   // enum: string-name or ordinal dispatch lives in from_JV
    } static_elif (typeinfo is_numeric(type<TT>)) {
        // jv is non-null here (read_json_field / the from_json generator both guarantee it), so read the number straight off the variant with no null guard — inlines null_coalescing[_with_string].
        static_if (typeinfo is_float(type<TT>) || typeinfo is_double(type<TT>)) {
            if (jv.value is _number) return TT(jv.value as _number)    // nolint:PERF020 redundant for TT=double
            if (jv.value is _longint) return TT(jv.value as _longint)
        } else {
            if (jv.value is _longint) return TT(jv.value as _longint)  // nolint:PERF020 redundant for TT=int64/uint64
            if (jv.value is _number) return TT(jv.value as _number)
            static_if (typeinfo stripped_typename(type<TT>) == "int64" || typeinfo stripped_typename(type<TT>) == "uint64") {
                if (jv.value is _string) return TT(jv.value as _string)  // 64-bit ints also accept a numeric string
            }
        }
        return default<TT -const>
    } else {
        return from_JV(jv, type<TT>)   // dim / array / tuple / variant / custom handled types
    }
}

[unsafe_outside_of_for]
def from_json(jv : JsonValue const?; t : type<auto(TT)>) : iterator<TT -const> {
    //! Lazily walks the elements of a JSON array *jv*, reading each element's top-level fields **by name** into a *TT* — the JSON counterpart of ``from_xml_node``, for ``for`` loops and LINQ chains:
    return <- generator<TT -const> {
        if (jv != null && jv.value is _array) {
            for (elem in jv.value as _array) {
                yield from_json_row(elem, type<TT>)
            }
        }
        return false
    }
}

[constant_expression(key=false)]
def read_json_field(jv : JsonValue const?; key : string; defv : auto(TT)) : TT {
    //! Reads one same-named key of the JSON object *jv* into a *TT* — the field-pruned counterpart of a full by-name materialize, emitted by linq_fold's fused JSON lane. A missing key keeps *defv* (pass the struct field's declared default). The value is converted by name via ``from_json_row``, so a struct field reads *its* fields by name too — never an overridable whole-struct ``from_JV``.
    let found = unsafe((unsafe(jv ?as _object))?[key])   // value-slot pointer; null if jv is null, not an object, or key absent
    if (found != null) return from_json_row(*found, type<TT>)
    return defv
}
