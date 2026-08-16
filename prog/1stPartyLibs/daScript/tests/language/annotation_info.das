// Annotation reflection through context debug info (AnnotationInfo / AnnotationArgumentInfo).
// Deliberately compiled WITHOUT `options rtti` — annotation data is deep-copied into the
// context debug heap and must not depend on the Program staying alive.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/rtti
require strings

[comment(v_int = 13, v_float = 1.5, v_bool = true, v_str = "hello", v_esc = "q\"uote\\back\nline")]
struct AnnotatedStruct {
    @v_int = 13 a : int
    @v_float = 1.5 b : float
    @v_bool = true c : bool
    @v_str = "hello" d : string
    e : int     // no annotations
}

enum PlainEnum {
    one
    two
}

[export]
def annotated_fn() {
    feint("annotated\n")
}

[test]
def test_struct_annotations(t : T?) {
    let s = AnnotatedStruct()
    let info = typeinfo rtti_typeinfo(s)
    let sinfo = info.structType
    t |> success(sinfo != null, "AnnotatedStruct should have StructInfo")
    t |> equal(int(sinfo.annotation_count), 1)
    var seen_args = 0
    for (ann in each_annotation(*sinfo)) {
        // direct const char*-backed field pass — regression for the AOT (char*)(const char*) cast
        t |> equal(ann.name, "comment")
        t |> equal(int(ann.count), 5)
        for (arg in each_annotation_argument(ann)) {
            let value = get_annotation_argument_value(arg)
            if (arg.name == "v_int") {
                t |> success(value is tInt, "v_int should be int")
                t |> equal(value as tInt, 13)
            } elif (arg.name == "v_float") {
                t |> success(value is tFloat, "v_float should be float")
                t |> equal(value as tFloat, 1.5)
            } elif (arg.name == "v_bool") {
                t |> success(value is tBool, "v_bool should be bool")
                t |> equal(value as tBool, true)
            } elif (arg.name == "v_str") {
                t |> success(value is tString, "v_str should be string")
                t |> equal(value as tString, "hello")
            } elif (arg.name == "v_esc") {
                t |> success(value is tString, "v_esc should be string")
                // annotation string_constant unescapes only \" — backslashes stay
                // literal; build the expected value from chars to stay independent
                // of expression-string escape rules
                let expected = "q" + to_char(34) + "uote" + to_char(92) + to_char(92) + "back" + to_char(92) + "nline"
                t |> equal(value as tString, expected)
            }
            seen_args ++
        }
    }
    t |> equal(seen_args, 5)
}

[test]
def test_field_annotations(t : T?) {
    let s = AnnotatedStruct()
    let sinfo = (typeinfo rtti_typeinfo(s)).structType
    var checked = 0
    unsafe {
        for (fld in *sinfo) {
            if (fld.name == "a") {
                t |> equal(int(fld.annotation_argument_count), 1)
                for (arg in each_annotation_argument(fld)) {
                    t |> equal(arg.name, "v_int")
                    t |> equal(get_annotation_argument_value(arg) as tInt, 13)
                }
                checked ++
            } elif (fld.name == "b") {
                for (arg in each_annotation_argument(fld)) {
                    t |> equal(get_annotation_argument_value(arg) as tFloat, 1.5)
                }
                checked ++
            } elif (fld.name == "c") {
                for (arg in each_annotation_argument(fld)) {
                    t |> equal(get_annotation_argument_value(arg) as tBool, true)
                }
                checked ++
            } elif (fld.name == "d") {
                for (arg in each_annotation_argument(fld)) {
                    t |> equal(get_annotation_argument_value(arg) as tString, "hello")
                }
                checked ++
            } elif (fld.name == "e") {
                t |> equal(int(fld.annotation_argument_count), 0)
                checked ++
            }
        }
    }
    t |> equal(checked, 5)
}

[test]
def test_function_annotations(t : T?) {
    var found_fn = false
    var found_export = false
    context_for_each_function() $(finfo) {
        if (finfo.name == "annotated_fn") {
            found_fn = true
            for (ann in each_annotation(finfo)) {
                if (ann.name == "export") {
                    found_export = true
                }
            }
        }
    }
    t |> success(found_fn, "annotated_fn should be in context debug info")
    t |> success(found_export, "annotated_fn should carry the [export] annotation")
}

[test]
def test_enum_annotations(t : T?) {
    let info = typeinfo rtti_typeinfo(PlainEnum.one)
    let einfo = info.enumType
    t |> success(einfo != null, "PlainEnum should have EnumInfo")
    t |> equal(int(einfo.annotation_count), 0)
}

[test]
def test_resolve_annotation(t : T?) {
    let s = AnnotatedStruct()
    let sinfo = (typeinfo rtti_typeinfo(s)).structType
    for (ann in each_annotation(*sinfo)) {
        let live = resolve_annotation(ann)
        t |> success(live != null, "comment annotation should resolve to the live object")
        if (live != null) {
            t |> equal("{live.name}", "comment")
            t |> success(live.isStructureAnnotation, "comment is a structure annotation")
        }
    }
}

// the deprecated API keeps working: Annotation* is env-resolved, args are reconstructed
[test]
def test_deprecated_structure_for_each_annotation(t : T?) {
    let s = AnnotatedStruct()
    let sinfo = (typeinfo rtti_typeinfo(s)).structType
    var seen = 0
    unsafe {
        structure_for_each_annotation(*sinfo) $(ann; args) {
            t |> equal("{ann.name}", "comment")
            t |> equal(length(args), 5)
            for (arg in args) {
                if (arg.name == "v_int") {
                    t |> equal(arg.iValue, 13)
                }
            }
            seen ++
        }
    }
    t |> equal(seen, 1)
}
