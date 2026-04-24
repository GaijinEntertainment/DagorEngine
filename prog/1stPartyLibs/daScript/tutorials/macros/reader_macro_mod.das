options gen2
options no_aot

// Tutorial macro module: reader macros (visit + suffix patterns).
//
// Provides two AstReaderMacro implementations:
//   CsvReader    — [reader_macro(name=csv)]    visit pattern
//                  Parses CSV text at compile time, returns an array expression.
//   BasicReader  — [reader_macro(name=basic)]   suffix pattern
//                  Transpiles BASIC-style code into daScript source, which
//                  the parser re-parses as normal code.
//
// TIMING:
//   accept()  — called character-by-character during PARSING
//   suffix()  — called immediately after accept() during PARSING;
//               returns source text injected into the lexer
//   visit()   — called during TYPE INFERENCE on the ExprReader node;
//               returns an AST expression replacing the reader expression
//
// Visit pattern (CsvReader):
//   accept() collects characters → visit() parses at compile time → returns AST
//   Used as an EXPRESSION: var data = %csv~ ... %%
//
// Suffix pattern (BasicReader):
//   accept() collects characters → suffix() returns daScript source → parser re-parses
//   Used at MODULE LEVEL: %basic~ ... %%
//   The ExprReader node is discarded; only the injected text matters.

module reader_macro_mod

require daslib/ast
require strings
require daslib/ast_boost
require daslib/strings_boost

// ---- CSV reader (visit pattern) ---------------------------------------------
// %csv~ Alice,30,New York %% → array of trimmed strings at compile time.

[reader_macro(name=csv)]
class CsvReader : AstReaderMacro {
    //! Embeds a CSV literal as a string array at compile time.
    //!
    //! accept() — collects characters until the %% terminator.
    //! visit()  — splits the sequence by commas, trims whitespace,
    //!   and returns the result via convert_to_expression().

    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        //! Standard %% terminator idiom: append each character,
        //! check for the terminator, strip it, and return false.
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

    def override visit(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprReader>) : ExpressionPtr {
        //! Parse the collected CSV text at compile time and embed
        //! the resulting string array directly in the AST.
        if (is_in_completion()) {
            return <- default<ExpressionPtr>
        }
        let seq = string(expr.sequence)
        var items <- split(seq, ",")
        for (i in range(length(items))) {
            items[i] = strip(items[i])
        }
        // convert_to_expression turns any daScript value into AST nodes.
        // Here it produces an ExprMakeArray of ExprConstString values.
        return <- convert_to_expression(items, expr.at)
    }
}

// ---- BASIC reader (suffix pattern) ------------------------------------------
// %basic~ DEF hello \n 10 PRINT "Hello" \n 20 LET x = 42 \n 30 PRINT x %%
// Generates a daScript function definition that the parser re-parses.

[reader_macro(name=basic)]
class BasicReader : AstReaderMacro {
    //! Transpiles a tiny BASIC program into a daScript function.
    //!
    //! accept() — collects characters until the %% terminator.
    //! suffix() — parses numbered BASIC lines, generates a daScript
    //!   function definition, and returns it as source text that the
    //!   parser re-parses.
    //!
    //! Supported BASIC commands:
    //!   DEF name          — names the generated function (must be first)
    //!   NUMBER PRINT "s"  — print a string literal
    //!   NUMBER PRINT var  — print a variable via string interpolation
    //!   NUMBER LET v = e  — declare a variable

    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
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

    def override suffix(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; info : LineInfo; var outLine : int&; var outFile : FileInfo?&) : string {
        let seq = string(expr.sequence)
        var lines <- split(seq, "\n")
        var func_name = "basic_program"
        var stmts : array<string>
        for (line in lines) {
            let trimmed = strip(line)
            if (empty(trimmed)) {
                continue
            }
            // DEF name — must appear first
            if (starts_with(trimmed, "DEF ")) {
                func_name = strip(slice(trimmed, 4))
                continue
            }
            // Numbered lines: skip the number, parse COMMAND args
            // Find first space (after the line number)
            let sp1 = find(trimmed, " ")
            if (sp1 < 0) {
                continue
            }
            let after_num = strip(slice(trimmed, sp1 + 1))
            if (starts_with(after_num, "PRINT ")) {
                let arg = strip(slice(after_num, 6))
                if (starts_with(arg, "\"")) {
                    // String literal: PRINT "text"
                    // Strip quotes, wrap in print("...\n")
                    let inner = slice(arg, 1, length(arg) - 1)
                    stmts |> push("print(\"{inner}\\n\")")
                } else {
                    // Variable: PRINT x → print("{x}\n")
                    // In the suffix output we need literal {x} for the parser,
                    // so we use escaped braces \{ and \} around the interpolation.
                    stmts |> push("print(\"\{{arg}\}\\n\")")
                }
            } elif (starts_with(after_num, "LET ")) {
                // LET x = 42 → var x = 42
                let assignment = strip(slice(after_num, 4))
                stmts |> push("var {assignment}")
            }
        }
        // Build the daScript function definition (gen2 syntax — braces required).
        // \{ and \} produce literal braces in the output, avoiding interpolation.
        return build_string() <| $(var w) {
            w |> write("def {func_name}() \{\n")
            for (stmt in stmts) {
                w |> write("    {stmt}\n")
            }
            w |> write("\}\n")
        }
    }
}
