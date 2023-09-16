options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module stringify shared public

require strings
require daslib/ast_boost

[reader_macro(name="stringify")]
class private LongStringReader : AstReaderMacro
    //! This macro implements embedding of the long string into the source code.
    //!   var st = %stringify~
    //!  This is a long string
    //!  with multiple lines
    //!  and special charactes like { this } and "that"
    //!  %%
    def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
        append(expr.sequence,ch)
        if ends_with(expr.sequence,"%%")
            let len = length(expr.sequence)
            resize(expr.sequence,len-2)
            return false
        else
            return true
    def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
        return <- new [[ExprConstString() at=expr.at, value:=expr.sequence]]
