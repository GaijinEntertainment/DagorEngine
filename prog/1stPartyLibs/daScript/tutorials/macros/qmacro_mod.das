options gen2
options no_aot

// Tutorial macro module: quasi-quotation reference (qmacro variants).
//
// This module demonstrates every qmacro variant and reification
// operator through a single [call_macro] named `build_demo`.
// When the user writes `build_demo()` the macro generates:
//
//   • A struct via qmacro_template_class  — DemoIntFloat (with method)
//   • A function via qmacro_function      — demo_run
//   • A function clone via qmacro_template_function — demo_add_int
//
// Every other qmacro variant (qmacro, qmacro_block, qmacro_block_to_array,
// qmacro_expr, qmacro_type, qmacro_variable, quote) and every reification
// splice ($v, $e, $i, $c, $f, $t, $a, $b) is exercised in the process.
//
// Every variant is exercised, including qmacro_method — used here
// to generate a `get_tuple` method on the cloned struct.

module qmacro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

// -----------------------------------------------------------------------
// Template struct — used by qmacro_template_class.
// `struct template` prevents direct instantiation.  TFirst / TSecond
// are alias names resolved when the template is cloned.
//
// The describe() method is defined inside the struct body.  When
// qmacro_template_class clones DemoPair into DemoIntFloat it also
// clones this method, renames it to DemoIntFloat`describe, and applies
// the same type substitution rules (TFirst → int, TSecond → float) to
// the method's self parameter and body.
// -----------------------------------------------------------------------
struct template DemoPair {
    first  : TFirst
    second : TSecond

    def describe() {
        print("first  = {self.first}\n")
        print("second = {self.second}\n")
    }
}

// -----------------------------------------------------------------------
// Template function — used by qmacro_template_function.
// The $t(t_value) tag types in the signature are resolved automatically
// when qmacro_template_function clones the function — it reads the tag
// and substitutes whatever TypeDeclPtr `t_value` holds at expansion time.
// -----------------------------------------------------------------------
def template demo_add(a, b : $t(t_value)) : $t(t_value) {
    return a + b
}

// -----------------------------------------------------------------------
// The main call macro.
// -----------------------------------------------------------------------

[call_macro(name="build_demo")]
class BuildDemoMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {

        // ===============================================================
        // 1. quote(expr) — plain AST capture, NO reification.
        //    Returns ExpressionPtr.  Use for simple literal constants.
        // ===============================================================
        var inscope literal_true <- quote(true)   // ExprConstBool
        var inscope literal_zero <- quote(0)      // ExprConstInt

        // ===============================================================
        // 2. qmacro(expr) — expression with reification splices.
        //    Returns ExpressionPtr.
        //
        //    Reification operators used here:
        //      $v(value) — splice compile-time VALUE as a constant
        //      $e(expr)  — splice an existing ExpressionPtr
        // ===============================================================
        let greeting = "hello from qmacro"

        // $v — compile-time string becomes a constant in the AST.
        var inscope print_greeting <- qmacro(print($v("{greeting}!\n")))

        // $e — insert an already-built ExpressionPtr into the tree.
        var inscope msg_expr <- qmacro("the literal is: ")
        var inscope print_literal <- qmacro(print($e(msg_expr) + "{$e(literal_true)}\n"))

        // ===============================================================
        // 3. qmacro_type(type<T>) — build a TypeDeclPtr.
        //    $t(typeDecl) — splice a TypeDeclPtr in type position.
        // ===============================================================
        var inscope int_type   <- qmacro_type(type<int>)
        var inscope float_type <- qmacro_type(type<float>)
        // $t — compose types: array<$t(int_type)> → array<int>
        var inscope arr_int_type <- qmacro_type(type<array<$t(int_type)>>)

        // ===============================================================
        // 4. qmacro_variable("name", type<T>) — build a VariablePtr.
        //    Used to create function arguments programmatically.
        // ===============================================================
        var inscope tag_var <- qmacro_variable("tag", type<string>)

        // ===============================================================
        // 5. qmacro_expr(${ statement; }) — single statement.
        //    Returns ExpressionPtr.  Useful for let/var declarations
        //    that need statement syntax in an expression context.
        // ===============================================================
        var inscope let_stmt <- qmacro_expr(${ let title = "items"; })

        // ===============================================================
        // 6. qmacro_block() { stmts } — multiple statements as ExprBlock.
        //    $b(stmts) — splice array<ExpressionPtr> as a block body.
        // ===============================================================
        var inscope steps : array<ExpressionPtr>
        steps |> emplace_new <| qmacro(print("  step 1\n"))
        steps |> emplace_new <| qmacro(print("  step 2\n"))
        steps |> emplace_new <| qmacro(print("  step 3\n"))

        var inscope block_stmts <- qmacro_block() {
            print("qmacro_block body:\n")
            $b(steps)
        }

        // ===============================================================
        // 7. qmacro_block_to_array() { stmts }
        //    Like qmacro_block but returns array<ExpressionPtr> —
        //    each statement becomes a separate element.
        // ===============================================================
        var inscope extra <- qmacro_block_to_array() {
            print("  extra A\n")
            print("  extra B\n")
        }

        // ===============================================================
        // 8. qmacro_function("name") $(args) : RetType { body }
        //    Generates a complete FunctionPtr.
        //
        //    Reification operators used here:
        //      $a(args) — splice array<VariablePtr> as parameter list
        //      $b(body) — splice array<ExpressionPtr> as function body
        //      $i(name) — splice a string as an IDENTIFIER reference
        //      $c(name) — splice a string as a function CALL name
        //      $f(name) — splice a string as a field access name
        // ===============================================================
        let fn_name = "demo_run"
        var inscope fn_args : array<VariablePtr>
        fn_args |> emplace_new <| clone_variable(tag_var)

        // Build the function body — each statement demonstrates
        // a different reification operator or qmacro variant.
        var inscope fn_body : array<ExpressionPtr>

        // $v — greeting is baked in as a string constant.
        fn_body |> emplace_new <| clone_expression(print_greeting)

        // $e — two sub-expressions spliced into a larger expression.
        fn_body |> emplace_new <| clone_expression(print_literal)

        // $c — string becomes a function call name.
        //      $c("print") → ExprCall { name = "print" }
        let print_fn = "print"
        fn_body |> emplace_new <| qmacro($c(print_fn)($v("called via $c\n")))

        // $i — string becomes an identifier (variable reference).
        //      $i("tag") → ExprVar { name = "tag" }
        let tag_name = "tag"
        fn_body |> emplace_new <| qmacro(print("$i resolved tag = {$i(tag_name)}\n"))

        // $f — string becomes a field access name.
        //      pair.$f("first") → pair.first
        let field_name = "first"
        fn_body |> emplace_new <| qmacro_expr(${
            var pair = DemoIntFloat(first = 42, second = 3.14);
        })
        fn_body |> emplace_new <| qmacro(print("pair.$f first = {pair.$f(field_name)}\n"))

        // qmacro_block result — three steps spliced via $b.
        fn_body |> emplace_new <| clone_expression(block_stmts)

        // qmacro_block_to_array — extra statements appended.
        for (s in extra) {
            fn_body |> emplace_new <| clone_expression(s)
        }

        // qmacro_expr — a let declaration as a single statement.
        fn_body |> emplace_new <| clone_expression(let_stmt)
        fn_body |> emplace_new <| qmacro(print("title = {title}\n"))

        // $a — splice argument list; $b — splice body statements.
        var inscope demo_fn <- qmacro_function(fn_name) $($a(fn_args)) {
            $b(fn_body)
        }
        demo_fn.flags |= FunctionFlags.generated
        add_function(compiling_module(), demo_fn)

        // ===============================================================
        // 9. qmacro_template_class("Name", type<Template>)
        //    Clones a struct template, renames it, clears the template
        //    flag, and returns a TypeDeclPtr pointing to the new struct.
        //
        //    Type aliases are resolved via add_structure_alias — this
        //    registers alias types on the cloned struct so the compiler
        //    replaces TFirst/TSecond with real types during compilation.
        //
        //    Also clones DemoPair`describe → DemoIntFloat`describe,
        //    applying the same alias substitution to the method body.
        // ===============================================================
        var inscope pair_type <- qmacro_template_class("DemoIntFloat", type<DemoPair>)
        pair_type.structType |> add_structure_alias("TFirst", int_type)
        pair_type.structType |> add_structure_alias("TSecond", float_type)

        // ===============================================================
        // 10. qmacro_template_function(@@template_fn)
        //     Clones a template function, strips the template flag.
        //     Takes a function address (@@), not a string name.
        //     The template uses $t(t_value) tag types in its signature,
        //     so qmacro_template_function auto-generates substitution
        //     rules — no manual type fixup is needed.
        // ===============================================================
        var inscope t_value <- clone_type(int_type)
        var inscope add_fn <- qmacro_template_function(@@demo_add)
        add_fn.name := "demo_add_int"
        add_fn.flags |= FunctionFlags.generated
        add_function(compiling_module(), add_fn)

        // ===============================================================
        // 11. qmacro_method("Cls`name", cls) $(var self : T) { body }
        //     Generates a FunctionPtr marked as a class method.
        //     Here we add a `get_tuple` method to DemoIntFloat that
        //     returns (first, second) as a tuple.
        // ===============================================================

        var inscope pair_struct_type <- add_ptr_ref(pair_type.structType)
        var inscope method <- qmacro_method("get_tuple", pair_struct_type) $(var self : $t(pair_type)) {
            return (self.first, self.second)
        }
        add_function(compiling_module(), method)

        // ===============================================================
        // Return value — all real work was done via add_function /
        // qmacro_template_class.  Return a trivial expression.
        // ===============================================================
        return <- quote(true)
    }
}
