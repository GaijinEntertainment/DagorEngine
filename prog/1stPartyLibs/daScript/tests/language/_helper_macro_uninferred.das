// helper module for failed_macro_added_function_must_infer test
// the [bad_emitter] structure_macro adds a function to the module
// during patch() but emits a call with an unresolvable identifier and
// does NOT set astChanged - exercising the path where a function
// is created post-infer and never visited by type inference
options gen2

module _helper_macro_uninferred public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

def some_callee(x : int) : void {
}

[structure_macro(name = "bad_emitter")]
class BadEmitterMacro : AstStructureAnnotation {
    def override patch(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        let badname = "id|{st.name}"
        var regblk <- setup_call_list("emitted_calls|{st.name}", st.at, true, true)
        regblk.list |> emplace_new <| qmacro(some_callee($i(badname)))
        return true
    }
}
