options gen2

module _template_struct_instance_mod shared public

require daslib/typemacro_boost public
require daslib/strings_boost

[ |> template_struct_instance]
class template public XModKern {
    x : KT
    @template_constant BIAS : int = 0

    def value : int {
        return x + BIAS
    }
}

[structure_macro(name = "tsi_test_bind_kt")]
class TsiTestBindKt : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        var td = new TypeDecl(at = st.at, baseType = Type.tInt)
        st |> add_structure_alias("KT", td)
        return true
    }
}

[ |> template_struct_instance]
class template public LateKern {
    x : KT

    def take(v : KT) : KT {
        return v + x
    }
}

// apply adds an int global named for the function — the stamped-clone annotation arm asserts
// the instance-named global exists (i.e. the reifier ran applies on the clone, exactly once)
[function_macro(name = "tsi_test_fn_tag")]
class TsiTestFnTag : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let gname = "{func.name}_tag" |> replace("`", "_")
        if (!add_global_var(compiling_module(), gname, new TypeDecl(at = func.at, baseType = Type.tInt), func.at, false)) {
            errors := "can't add global {gname}"
            return false
        }
        return true
    }
}
