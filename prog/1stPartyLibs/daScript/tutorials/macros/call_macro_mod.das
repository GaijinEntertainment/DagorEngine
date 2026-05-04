// Macro Tutorial 1: Call Macros — Module
//
// This module defines three progressively complex call macros:
//   Section 1: hello()       — simplest possible call macro
//   Section 2: greet("name") — call macro with argument validation
//   Section 3: printf(fmt, args...) — format-string macro with reordering
//
// Call macros intercept function-call-like syntax at compile time and
// replace it with arbitrary AST. They cannot be used in the module
// that defines them — see 01_call_macro.das for usage.

options gen2

module call_macro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require daslib/macro_boost

// ============================================================================
// Section 1: Minimal call macro — hello()
//
// A call macro is a class that extends AstCallMacro, annotated with
// [call_macro(name="...")]. When the compiler sees a call to that name,
// it invokes the macro's `visit` method instead of looking for a function.
//
// The visit method receives:
//   prog — the program being compiled (for error reporting)
//   mod  — the module where the call appears
//   expr — the call expression (arguments, source location, etc.)
//
// It must return an ExpressionPtr — the AST that replaces the call.
// Returning an empty ExpressionPtr signals an error.
//
// qmacro(...) is a reification helper that builds AST from daslang syntax.
// ============================================================================

[call_macro(name="hello")]
class HelloMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        // The simplest macro: ignore arguments, return print("hello\n")
        macro_verify(length(expr.arguments) == 0, prog, expr.at,
            "hello() takes no arguments")
        return <- qmacro(print("hello, call macro!\n"))
    }
}

// ============================================================================
// Section 2: Call macro with arguments — greet("name")
//
// expr.arguments is an array of ExpressionPtr — one per argument passed
// to the macro call. We can inspect their type at compile time using `is`:
//   expr.arguments[0] is ExprConstString  — checks if it's a string literal
//
// macro_verify(condition, prog, at, message) is a helper that emits
// a compile error and returns an empty ExpressionPtr if condition is false.
//
// ExprStringBuilder is the AST node for string interpolation ("{...}").
// We build one manually by filling its `elements` array with:
//   - ExprConstString nodes for literal text
//   - clone_expression(arg) for interpolated values
//
// The $e(expr) escape inside qmacro inserts an expression node.
// ============================================================================

[call_macro(name="greet")]
class GreetMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        // Validate: exactly one argument, must be a string literal
        macro_verify(length(expr.arguments) == 1, prog, expr.at,
            "greet() requires exactly one string argument")
        macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at,
            "greet() argument must be a string literal")
        // Build: print("hello, {name}!\n") using ExprStringBuilder
        var inscope sbuilder <- new ExprStringBuilder(at = expr.at)
        sbuilder.elements |> emplace_new <| new ExprConstString(value := "hello, ", at = expr.at)
        sbuilder.elements |> emplace_new <| clone_expression(expr.arguments[0])
        sbuilder.elements |> emplace_new <| new ExprConstString(value := "!\n", at = expr.at)
        return <- qmacro(print($e(sbuilder)))
    }
}

// ============================================================================
// Section 3: Format-string macro — printf(fmt, arg1, arg2, ...)
//
// This macro parses a format string containing (N) placeholders where N
// is a 1-based argument index. It builds an ExprStringBuilder with
// literal text segments interleaved with argument expressions.
//
// Example: printf("(2) said (1)\n", name, greeting)
//   → print("{greeting} said {name}\n")
//
// Key techniques:
//   - String parsing at compile time (the format string is a constant)
//   - to_int() to convert placeholder numbers
//   - Bounds checking with macro_verify
//   - clone_expression() to duplicate argument AST nodes (since an
//     argument might be referenced multiple times in the format string)
// ============================================================================

[call_macro(name="printf")]
class PrintfMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        // Validate: at least a format string
        macro_verify(length(expr.arguments) >= 1, prog, expr.at,
            "printf requires at least a format string argument")
        macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at,
            "first argument to printf must be a constant string")
        let totalArgs = length(expr.arguments)
        var inscope sbuilder <- new ExprStringBuilder(at = expr.at)
        let format = string((expr.arguments[0] as ExprConstString).value)
        var pos = 0
        let format_len = length(format)
        while (pos < format_len) {
            // Find the next (N) placeholder
            var open = find(format, '(', pos)
            if (open == -1) {
                // No more placeholders — add remaining text
                let tail = format.chop(pos, format_len - pos)
                sbuilder.elements |> emplace_new <| new ExprConstString(value := tail, at = expr.at)
                break
            }
            // Add literal text before the placeholder
            if (open > pos) {
                let text = format.chop(pos, open - pos)
                sbuilder.elements |> emplace_new <| new ExprConstString(value := text, at = expr.at)
            }
            // Find matching closing paren
            var close = find(format, ')', open + 1)
            macro_verify(close != -1, prog, expr.at,
                "unmatched '(' in format string")
            // Parse the argument number
            var argNumStr = format.chop(open + 1, close - open - 1)
            var argNum = to_int(argNumStr)
            macro_verify(argNum >= 1, prog, expr.at,
                "argument number must be >= 1, got '({argNumStr})'")
            macro_verify(argNum < totalArgs, prog, expr.at,
                "argument index ({argNum}) out of range, printf has {totalArgs - 1} argument(s)")
            // Insert a clone of the referenced argument expression
            sbuilder.elements |> emplace_new <| clone_expression(expr.arguments[argNum])
            pos = close + 1
        }
        return <- qmacro(print($e(sbuilder)))
    }
}
