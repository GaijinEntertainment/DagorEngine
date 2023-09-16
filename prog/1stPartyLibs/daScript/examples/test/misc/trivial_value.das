options indenting = 4
options no_aot = true

module trivial_value

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

// macro, which replaces trivial_value(VAL)
// with:
//  @@ (effects : int) // effects : ArmyEffects
//      return <- [{ auto[] val => 1.0 }]
[call_macro(name="trivial_value")]
class TrivialValueMacro : AstCallMacro
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        // check if we  get correct number of arguments
        macro_verify(call.arguments |> length==1,prog,call.at,"expecting trivalValue(constant_value)")
        // check if argument is constant value (not expression). i.e. "blah" or 1234
        macro_verify((call.arguments[0] is ExprConstString) || (call.arguments[0] is ExprConstInt),
            prog, call.at,"expecting constant value, got {call.arguments[0].__rtti}")
        // build a block which we are planning to return
        return <- qmacro() <| @@ (effects : int) // effects : ArmyEffects
            return <- [{ auto[] $e(call.arguments[0]) => 1.0 }]
