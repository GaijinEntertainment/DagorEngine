options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module class_boost shared private

//! Class method macro.
//!
//! Provides ``[class_method]`` and ``[explicit_const_class_method]`` annotations
//! that transform static functions into class methods by automatically injecting
//! a ``self`` argument and wrapping the body in ``with (self)``.

require daslib/ast_boost

require daslib/templates_boost

[function_macro(name="class_method")]
class ClassMethodMacro : AstFunctionAnnotation {
    //! Turns a static method into a class method by adding a ``self`` argument
    //! of the class type as the first argument, and wrapping the function body
    //! in ``with (self) { ... }``. Applied via ``[class_method]`` annotation.
    explicitConstClassMethod : bool = false
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (func.classParent == null) {
            errors := "class_method can be applied only to class methods";
            return false
        }
        var inscope tt <- new TypeDecl(baseType = Type.tStructure, structType = func.classParent)
        if (explicitConstClassMethod) {
            tt.flags |= TypeDeclFlags.explicitConst
        }
        if (func.moreFlags.isConstClassMethod) {
            tt.flags |= TypeDeclFlags.constant
        }
        var inscope vv <- qmacro_variable("self", type<$t(tt)>)
        emplace(func.arguments, vv, 0)
        func.body |> move_new <| qmacro_block() {
            with (self) {
                $e(func.body)
            }
        }
        return true
    }
}

[function_macro(name="explicit_const_class_method")]
class ExplicitClassMethodMacro : ClassMethodMacro {
    //! Same as ``[class_method]`` but marks the ``self`` parameter with ``explicitConst``,
    //! allowing overloading of const and non-const class methods.
    override explicitConstClassMethod : bool = true
}
