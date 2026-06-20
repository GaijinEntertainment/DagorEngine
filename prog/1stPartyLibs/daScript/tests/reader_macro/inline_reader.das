options gen2
options no_aot

module inline_reader

require daslib/ast
require daslib/ast_boost
require strings

[reader_macro(name=wrap)]
class WrapReader : AstReaderMacro {
    //! Test inline (expression-level) reader macro: ``%wrap! <expr> %%`` rewrites to ``( <expr> )``.
    //! Exercises the inline rewrite path — the body text is captured and re-parsed in place as an
    //! ordinary expression. ``accept`` collects the body (eating the ``%%`` terminator); ``suffix``
    //! returns the parenthesized rewrite that the lexer splices back in.
    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        append(expr.sequence, ch)
        if (ends_with(expr.sequence, "%%")) {
            resize(expr.sequence, length(expr.sequence) - 2)
            return false
        }
        return true
    }
    def override suffix(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; info : LineInfo;
                        var outLine : int&; var outFile : FileInfo?&) : string {
        return build_string() $(var writer) {
            writer |> write("( ")
            writer |> write(expr.sequence)   // write accepts das_string directly — no string()/peek cast
            writer |> write(" )")
        }
    }
}

[reader_macro(name=at_line)]
class AtLineReader : AstReaderMacro {
    //! Regression macro for multi-line location tracking: ``%at_line! … %%`` rewrites to the source
    //! line number the lexer assigned to the macro (``info.line``). A multi-line body must report its
    //! OPENING line, not the closing ``%%`` line — see test_inline_multiline_location.
    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        append(expr.sequence, ch)
        if (ends_with(expr.sequence, "%%")) {
            resize(expr.sequence, length(expr.sequence) - 2)
            return false
        }
        return true
    }
    def override suffix(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; info : LineInfo;
                        var outLine : int&; var outFile : FileInfo?&) : string {
        return "({int(info.line)})"
    }
}
