module _array_reader

require rtti
require ast
require strings
require daslib/ast_boost

[reader_macro(name="arr")]
class ArrayReader : AstReaderMacro
    def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
        append(expr.sequence,ch)
        if ends_with(expr.sequence,"%%")
            let len = length(expr.sequence)
            resize(expr.sequence,len-2)
            return false
        else
            return true
    def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
        let seqStr = string(expr.sequence)
        var arrT <- new [[TypeDecl() baseType=Type tInt]]
        push(arrT.dim,length(seqStr))
        var mkArr <- new [[ExprMakeArray() at = expr.at, makeType <- arrT]]
        for x in seqStr
            var mkC <- new [[ExprConstInt() at=expr.at, value=x]]
            emplace(mkArr.values,mkC)
        return mkArr
