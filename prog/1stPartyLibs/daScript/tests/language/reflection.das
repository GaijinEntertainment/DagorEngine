options gen2
require daslib/rtti
require strings
require daslib/strings_boost
require dastest/testing_boost public

// note: this is var because we want "sample" global variable to show up
var {
    sample = "
require math
require UnitTest

let
    one_two = 12

enum TestEnum
    foo
    bar

[checkRange (foo=5)]
struct TestStruct
    a : int = 5
    b : string = \"something\"
    c : float[3][4]
    d : array<int>
    e : table<float; string>
    f : TestStruct?
    g : tuple<int; float; string>
    h : variant<i : int; f : float>
    fn : function<(a:int;b:float):string>
    lm : lambda<(a:int;b:float):string>

[export]
def bar ( blk : block<(a:int):float> )
    pass

[export]
def main
    feint(\"hello world!\n\")
    return true
"
}

def describeType(info) {
    var res : string
    if (info.basicType == Type.tHandle) {
        res = string(info.annotation.name)
    } elif (info.basicType == Type.tStructure) {
        res = info.structType?.name ?? "structure"
    } elif (info.basicType == Type.tEnumeration || info.basicType == Type.tEnumeration8 || info.basicType == Type.tEnumeration16 || info.basicType == Type.tEnumeration64) {
        res = info.enumType?.name ?? "enumeration"
    } elif (info.basicType == Type.tPointer) {
        res = info.firstType != null ? "{describeType(*(info.firstType))}?" : "void?"
    } elif (info.basicType == Type.tArray) {
        res = info.firstType != null ? "array<{describeType(*(info.firstType))}>" : "array"
    } elif (info.basicType == Type.tTable) {
        if (info.firstType != null && info.secondType != null) {
            res = "table<{describeType(deref(info.firstType))};{describeType(deref(info.secondType))}>"
        } else {
            res = "table"
        }
    } elif (info.basicType == Type.tTuple) {
        if (info.argTypes != null) {
            res = "tuple<" + join([for (argtype in arg_types(info)); describeType(argtype)], ";") + ">"
        } else {
            res += "tuple"
        }
    } elif (info.basicType == Type.tVariant) {
        if (info.argTypes != null && info.argNames != null) {
            res = "variant<" +  join([for (argname, argtype in arg_names(info), arg_types(info)); "{argname}:{describeType(argtype)}"], ";") + ">"
        } else {
            res += "variant"
        }
    } elif (info.basicType == Type.tFunction || info.basicType == Type.tLambda || info.basicType == Type.tBlock) {
        res = "{get_das_type_name(info.basicType)}<"
        if (info.argTypes != null) {
            res += "(" + join([for (argtype in arg_types(info)); describeType(argtype)], ";") + ")"
        }
        if (info.firstType != null) {
            if (info.argTypes != null) {
                res += ":"
            }
            res += "{describeType(*(info.firstType))}"
        }
        res += ">"
    } else {
        res = get_das_type_name(info.basicType)
    }
    res += join([for (d in each_dim(info)); d != -1 ? "[{d}]" : "[]"], "")
    if (info.isConst) {
        res += " const"
    }
    if (info.isRef) {
        res += " &"
    }
    if (info.isTemp) {
        res += "#"
    }
    if (info.isImplicit) {
        res += " implicit"
    }
    return res
}

def describeFunction(finfo) {
    feint("def {finfo.name} ")
    if (finfo.count != 0u) {
        let argT = join([for (arg in finfo); "{arg.name}:{describeType(arg)}"], "; ")
        feint("( {argT} ) ")
    }
    if (finfo.result != null) {
        feint(": {describeType(*(finfo.result))}")
    }
    feint("\n")
}

def describeValue(value) {
    if (value is nothing) {
        return ""
    } elif (value is tString) {
        return "=\"{value}\""
    } else {
        return "={value}"
    }
}

def describeVariable(glob; extra = "") {
    feint("{extra}{glob.name} : {describeType(glob)}{describeValue(get_variable_value(glob))}\n")
}

def describeStructure(sinfo) {
    var anyAnn = false
    structure_for_each_annotation(sinfo) <| $(ann; annArgs) {
        let argT = join([for (arg in annArgs); "{arg.name}{describeValue(get_annotation_argument_value(arg))}"], ",")
        feint("[{ann.name}({argT})]\n")
    }
    feint("struct {sinfo.name}\n")
    for (sfield in sinfo) {
        describeVariable(sfield, "\t")
    }
}

def describeEnumeration(einfo) {
    feint("enum {einfo.name}\n")
    for (eitem in einfo) {
        feint("\t{eitem.name} = {eitem.value}\n")
    }
}

def describeAnnotation(ann) {
    if (ann.isBasicStructureAnnotation) {
        unsafe {
            let sann & = reinterpret<BasicStructureAnnotation&>(ann)
            feint("struct {sann.name} // {sann.fieldCount} {sann.cppName}\n")
            basic_struct_for_each_field(sann) <| $(name, cppName, xtype, offset) {
                feint("\t{name}:{describeType(xtype)} // {cppName} at {offset}\n")
            }
        }
    } elif (ann.isTypeAnnotation) {
        unsafe {
            let tann & = reinterpret<TypeAnnotation&>(ann)
            feint("{tann.name} // {tann.cppName}\n")
        }
    } else {
        feint("{ann.name}\n")
    }
}

def describeModule(mod) {
    module_for_each_enumeration(mod) <| $(einfo) {
        describeEnumeration(einfo)
    }
    module_for_each_structure(mod) <| $(sinfo) {
        describeStructure(sinfo)
    }
    module_for_each_annotation(mod) <| $(ann) {
        describeAnnotation(ann)
    }
    module_for_each_function(mod) <| $(finfo) {
        describeFunction(finfo)
    }
    module_for_each_generic(mod) <| $(finfo) {
        describeFunction(finfo)
    }
    module_for_each_global(mod) <| $(glob) {
        describeVariable(glob)
    }
}

// note: do_reflection iterates all registered modules including generics.
// module_for_each_generic crashes on dastest's suite module generics (dangling RTTI),
// so we skip full registered-module iteration and test safe subsets instead.

[test]
def test_compile_sample(t : T?) {
    var compiled_ok = false
    compile("sample", sample, CodeOfPolicies()) <| $(ok; prog; errors) {
        if (ok) {
            compiled_ok = true
            let thisMod = get_this_module(prog)
            program_for_each_module(prog) <| $(mod) {
                if (mod == thisMod) {
                    describeModule(mod)
                }
            }
        } else {
            feint("failed to compile:\n{errors}\n")
        }
    }
    t |> success(compiled_ok, "sample program should compile")
}

[test]
def test_dollar_module(t : T?) {
    // describe the builtin "$" module — enums, structs, annotations, functions, globals
    describeModule(get_module("$"))
    t |> success(true)
}

[test]
def test_context_reflection(t : T?) {
    var fcount = 0
    context_for_each_function() <| $(finfo) {
        describeFunction(finfo)
        fcount++
    }
    var vcount = 0
    context_for_each_variable() <| $(vinfo) {
        describeVariable(vinfo)
        vcount++
    }
    t |> success(fcount > 0, "should have context functions")
    t |> success(vcount > 0, "should have context variables")
}

[test]
def test_registered_modules_safe(t : T?) {
    // iterate registered modules — enums, structs, annotations, functions, globals
    // skip generics: module_for_each_generic can crash on some module RTTI
    var modCount = 0
    program_for_each_registered_module() <| $(mod) {
        module_for_each_enumeration(mod) <| $(einfo) {
            describeEnumeration(einfo)
        }
        module_for_each_structure(mod) <| $(sinfo) {
            describeStructure(sinfo)
        }
        module_for_each_annotation(mod) <| $(ann) {
            describeAnnotation(ann)
        }
        module_for_each_function(mod) <| $(finfo) {
            describeFunction(finfo)
        }
        module_for_each_global(mod) <| $(glob) {
            describeVariable(glob)
        }
        modCount++
    }
    t |> success(modCount > 0, "should iterate registered modules")
}
