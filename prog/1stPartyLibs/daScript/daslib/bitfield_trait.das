options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module bitfield_trait shared private

//! Compile-time bitfield name trait generation.
//!
//! Generates ``bitfield_trait`` functions that return the string name
//! of a bitfield bit, enabling runtime bitfield-to-string conversion.

require daslib/ast
require daslib/ast_boost
require daslib/contracts
require daslib/strings_boost
require daslib/templates_boost
require daslib/macro_boost

[expect_any_bitfield(argT), tag_function(each_bitfield_tag), unused_argument(argT)]
def public each(argT) {
    //! Iterates over the names of a bitfield type, yielding each bit as a bitfield value (1ul << bitIndex).
    pass
}

[tag_function_macro(tag="each_bitfield_tag")]
class EachBitfieldMacro : AstFunctionAnnotation {
    //! This macro converts each(bitfield) to the following code::
    //!     generator<string>() <|
    //!         yield field1
    //!         yield field2
    //!         ...
    //!         return false
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {
        macro_verify(call.arguments[0]._type != null, compiling_program(), call.at, "expecting argument type")
        macro_verify(call.arguments[0]._type.isBitfield, compiling_program(), call.at, "expecting a bitfield type")
        var inscope yields : array<ExpressionPtr>
        if (call.arguments[0]._type.baseType == Type.tBitfield8) {
            for (bit in iter_range(call.arguments[0]._type.argNames)) {
                yields |> emplace_new <| qmacro_expr(${ yield $v(bitfield8(1ul << uint64(bit))); })
            }
        } elif (call.arguments[0]._type.baseType == Type.tBitfield16) {
            for (bit in iter_range(call.arguments[0]._type.argNames)) {
                yields |> emplace_new <| qmacro_expr(${ yield $v(bitfield16(1ul << uint64(bit))); })
            }
        } elif (call.arguments[0]._type.baseType == Type.tBitfield64) {
            for (bit in iter_range(call.arguments[0]._type.argNames)) {
                yields |> emplace_new <| qmacro_expr(${ yield $v(bitfield64(1ul << uint64(bit))); })
            }
        } else {
            for (bit in iter_range(call.arguments[0]._type.argNames)) {
                yields |> emplace_new <| qmacro_expr(${ yield $v(bitfield(1ul << uint64(bit))); })
            }
        }
        let call_name = "field`values`{call.arguments[0]._type.get_mnh}"
        var inscope qfn <- qmacro_function(call_name) {
            return <- generator<$t(call.arguments[0]._type)>() <| $() {
                $b(yields)
                return false
            }
        }
        qfn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        compiling_module() |> add_function(qfn)
        let call_expr_name = "_::{call_name}"
        return <- qmacro($c(call_expr_name)())
    }
}

[expect_any_bitfield(argT), tag_function(each_bit_name_bitfield_tag), unused_argument(argT)]
def public each_bit_name(argT) {
    //! Iterates over the names of a bitfield type, yielding each bit name as a string.
    pass
}

[tag_function_macro(tag="each_bit_name_bitfield_tag")]
class EachBitNameBitfieldMacro : AstFunctionAnnotation {
    //! This macro converts each(bitfield) to the following code::
    //!     generator<string>() <|
    //!         yield "field1"
    //!         yield "field2"
    //!         ...
    //!         return false
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {
        macro_verify(call.arguments[0]._type != null, compiling_program(), call.at, "expecting argument type")
        macro_verify(call.arguments[0]._type.isBitfield, compiling_program(), call.at, "expecting a bitfield type")
        var inscope yields : array<ExpressionPtr>
        for (name in call.arguments[0]._type.argNames) {
            yields |> emplace_new <| qmacro_expr(${ yield $v(string(name)); })
        }
        let call_name = "field`names`{call.arguments[0]._type.get_mnh}"
        var inscope qfn <- qmacro_function(call_name) <| $() {
            return <- generator<string>() <| $() {
                $b(yields)
                return false
            }
        }
        qfn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        compiling_module() |> add_function(qfn)
        let call_expr_name = "_::{call_name}"
        return <- qmacro($c(call_expr_name)())
    }
}

