options gen2
options indenting = 4
options strict_smart_pointers = true

module stringify shared public

//! Long-string embedding reader macro.
//!
//! ``%stringify~ ... %%`` embeds a multi-line string literal verbatim
//! into the source code, preserving whitespace and special characters
//! without requiring escaping.

require strings
require daslib/ast_boost

[reader_macro(name="stringify")]
class private LongStringReader : AstReaderMacro {
    //! This macro embeds a long string into the source code.
    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        //! Accepts characters into the reader sequence until the closing %% delimiter is found.
        if (ch != '\r') {
            append(expr.sequence, ch)
        }
        if (ends_with(expr.sequence, "%%")) {
            let len = length(expr.sequence)
            resize(expr.sequence, len - 2)
            return false
        } else {
            return true
        }
    }
    def override visit(prog : ProgramPtr; mod : Module?; expr : ExprReader?) : ExpressionPtr {
        //! Converts the accumulated reader sequence into a constant string expression.
        return <- new ExprConstString(at = expr.at, value := expr.sequence)
    }
}
