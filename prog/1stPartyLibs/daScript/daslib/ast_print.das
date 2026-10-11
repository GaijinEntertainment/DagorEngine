options gen2

module ast_print

//! AST pretty-printer.
//!
//! Walks a compiled daslang AST and produces human-readable source-like
//! output. Useful for debugging macros and inspecting transformed code.

options rtti = true
// options log_infer_passes = true
options strict_smart_pointers = false
// options log = true

require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/strings_boost


require daslib/ast_print_flags

//////////
// example

// TODO: fixme
def noBracket(expr) {
    //! Checks whether an expression requires surrounding brackets.
    return false;
}


class PrintVisitor : AstVisitor {
    //! AST visitor that pretty-prints daslang code back to source text.
    writer : StringBuilderWriter?
    extraTypeInfo : bool = true;
    printCStyle : bool = true;
    tab : int = 0;

    function_annotation_flags = (
            (FunctionFlags.fastCall) |
            (FunctionFlags.exports) |
            (FunctionFlags.privateFunction) |
            (FunctionFlags.unsafeDeref) |
            (FunctionFlags.unsafeOperation) |
            (FunctionFlags._generator));

    def newLine() {
        //! Writes a newline character to the output.
        write(*writer, "\n");
    }
    def ident(tabs : int = -1) {
        //! Returns an indentation string for the current or specified tab level.
        if (tabs == -1) {
            return repeat("    ", tab)
        } else {
            return repeat("    ", tabs)
        }
    }

    def override preVisitProgram(prog : ProgramPtr) {
        //! Prints the program header comment.
        write(*writer, "// program\n");
    }
    def override visitProgram(prog : ProgramPtr) {
        //! Prints the end-of-program comment.
        write(*writer, "// end program\n");
    }
    def override preVisitProgramBody(prog : ProgramPtr; mod : Module?) {
        //! Prints the program body marker comment.
        write(*writer, "// program body\n");
    }
    ET : bool = false;
    def override preVisitTypeDecl(typ : TypeDeclPtr) {
        //! Prints the opening marker for an expression-type annotation.
        ET = typ.isExprType;
        if (ET) {
            write(*writer, "/*[");
        }
    }
    def override visitTypeDecl(typ : TypeDeclPtr) {
        //! Prints the closing marker for an expression-type annotation.
        if (ET) {
            write(*writer, "]*/");
        }
        return typ;
    }
    def override preVisitAlias(typ : TypeDeclPtr; name : das_string) {
        //! Prints a type alias declaration.
        write(*writer, "typedef {name} = {describe([decl=typ,extra=extraTypeInfo])}\n\n");
    }
    def override preVisitEnumeration(enu : EnumerationPtr) {
        //! Prints the opening of an enumeration declaration.
        write(*writer, "enum {enu.name} : {get_das_type_name(enu.baseType)} \{\n");
    }
    def override preVisitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        //! Prints an enumeration value name and assignment operator.
        write(*writer, "{ident(1)}{name} = ");
    }
    def override visitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        //! Prints a newline after an enumeration value.
        write(*writer, "\n");
        return value;
    }
    def override visitEnumeration(enu : EnumerationPtr) {
        //! Prints the closing brace of an enumeration.
        write(*writer, "\}\n\n");
        return enu;
    }
    def override preVisitStructure(str : StructurePtr) {
        //! Prints the opening of a struct or class declaration.
        if (str.annotations |> length != 0) {
            write(*writer, "[{describe(str.annotations)}]\n");
        }
        write(*writer, "{str.flags.isClass ? "class" : "struct"} {str.name} \{\n");
    }
    def override preVisitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) {
        //! Prints a structure field with its type, annotations, and optional initializer.
        write(*writer, "{ident(1)}");
        if (decl.annotation |> length != 0) {
            write(*writer, "[{describe(decl.annotation)}] ");
        }
        write(*writer, "{decl.name} : {describe([decl=decl._type,extra=extraTypeInfo])}");
        if (decl.flags.parentType) {
            write(*writer, " /* from {str.parent.name} */");
        }
        if (decl.init != null) {
            write(*writer, "{decl.flags.moveSemantics ? " <- " : " = "}");
        }
    }
    def override visitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) {
        //! Prints a newline after a structure field.
        write(*writer, "\n");
    }
    def override visitStructure(var str : StructurePtr) : StructurePtr {
        //! Prints the closing brace of a structure declaration.
        write(*writer, "\}\n\n");
        return <- str;
    }
    def override preVisitFunction(fun : FunctionPtr) {
        //! Prints the function declaration header with annotations and name.
        if (fun.flags.knownSideEffects && fun.sideEffectFlags == bitfield(0)) {
            write(*writer, "// [nosideeffects]\n");
        }
        if (!empty(fun.annotations)) {
            write(*writer, "[{describe(fun.annotations)}]\n");
        }
        write(*writer, "def {fun.name}");
        if (!empty(fun.arguments)) {
            write(*writer, " ( ");
        }
    }
    def override preVisitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) {
        //! Prints the closing parenthesis of arguments and the return type.
        if (!empty(fun.arguments)) {
            write(*writer, " ) ");
        }
        if (fun.result != null && !fun.result.isVoid) {
            write(*writer, " : {describe([decl=fun.result,extra=extraTypeInfo])}");
        }
        write(*writer, " ");
    }
    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        //! Prints trailing newlines after a function definition.
        write(*writer, "\n\n");
        return <- fun;
    }
    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; last : bool) {
        //! Prints a function argument with its type and annotations.
        if (arg.annotation |> length != 0) {
            write(*writer, "[{describe(arg.annotation)}] ");
        }
        if (!arg._type.flags.constant) {
            write(*writer, "var ");
        }
        write_variable_decl(arg);
    }
    def override visitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; last : bool) {
        //! Prints the separator between function arguments.
        if (!last) {
            write(*writer, "; ");
        }
        return arg;
    }
    def override preVisitFunctionArgumentInit(fun : FunctionPtr; arg : VariablePtr; value : ExpressionPtr) {
        //! Prints the equals sign before an argument default value.
        write(*writer, " = ");
    }
    def override preVisitExprBlock(blk : ExprBlock?) {
        //! Prints the opening of a block expression, including closure arguments.
        if (blk.blockFlags.isClosure) {
            if (blk.returnType != null || !empty(blk.arguments)) {
                write(*writer, "$(");
                for (arg, argIndex in blk.arguments, range(blk.arguments |> length)) {
                    if (arg.annotation |> length != 0) {
                        write(*writer, "[{describe(arg.annotation)}] ");
                    }
                    if (!arg._type.flags.constant) {
                        write(*writer, "var ");
                    }
                    write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
                    if (argIndex != blk.arguments |> length - 1) {
                        write(*writer, "; ");
                    }
                }
                write(*writer, ")");
                if (blk.returnType != null) {
                    write(*writer, ":{describe([decl=blk.returnType,extra=extraTypeInfo])}");
                }
                write(*writer, " ");
            }
        }
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "\{\n");
        }
        tab ++;
    }
    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        //! Prints the closing brace of a block expression.
        tab --;
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident()}\}");
        }
        return <- blk;
    }
    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        //! Prints indentation before a block sub-expression.
        write(*writer, "{ident()}");
    }
    def override visitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        //! Prints a newline after a block sub-expression.
        self.newLine();
        return expr;
    }
    def override preVisitExprBlockFinal(blk : ExprBlock?) {
        //! Prints the finally block header.
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident(tab-1)}\} finally \{\n");
        } else {
            write(*writer, "{ident(tab-1)}finally\n");
        }
    }
    def override preVisitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        //! Prints indentation before a finally-block expression.
        write(*writer, "{ident()}");
    }
    def override visitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        //! Prints a newline after a finally-block expression.
        self.newLine();
        return expr;
    }
    def override preVisitExprLet(expr : ExprLet?) {
        //! Prints the let or var keyword for a variable declaration.
        var isLet = true;
        for (pv in expr.variables) {
            if (pv._type != null && !pv._type.flags.constant) {
                isLet = false;
                break;
            }
        }
        write(*writer, "{isLet ? "let" : "var"} ");
    }
    def write_variable_decl(arg : VariablePtr) : void {
        if (arg.isAccessUnused) {
            write(*writer, " /*unused*/ ")
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}")
    }

    def override preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) {
        //! Prints a let-variable name and its type.
        write_variable_decl(arg)
    }
    def override visitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) {
        //! Prints the separator between let-variable declarations.
        if (!lastArg) {
            write(*writer, ";");
        }
        return arg;
    }
    def override preVisitExprLetVariableInit(blk : ExprLet?; arg : VariablePtr; expr : ExpressionPtr) {
        //! Prints the initializer operator for a let-variable.
        if (arg.flags.init_via_move) {
            write(*writer, " <- ");
        } elif (arg.flags.init_via_clone) {
            write(*writer, " := ");
        } else {
            write(*writer, " = ");
        }
    }
    def override preVisitGlobalLetVariable(arg : VariablePtr; lastArg : bool) {
        //! Prints a global variable declaration with its type.
        write(*writer, "{arg._type.flags.constant ? "let" : "var"}{arg.flags.global_shared ? " shared" : ""}\n{ident(1)}");
        write_variable_decl(arg)
    }
    def override visitGlobalLetVariable(arg : VariablePtr; lastArg : bool) {
        //! Prints trailing newlines after a global variable.
        write(*writer, "\n\n");
        return arg;
    }
    def override preVisitGlobalLetVariableInit(arg : VariablePtr; expr : ExpressionPtr) {
        //! Prints the initializer operator for a global variable.
        if (arg.flags.init_via_move) {
            write(*writer, " <- ");
        } elif (arg.flags.init_via_clone) {
            write(*writer, " := ");
        } else {
            write(*writer, " = ");
        }
    }
    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) {
        //! Prints the opening of a string builder expression.
        write(*writer, "string_builder(");
    }
    def override visitExprStringBuilder(var expr : ExprStringBuilder?) : ExpressionPtr {
        //! Prints the closing parenthesis of a string builder expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprStringBuilderElement(expr : ExprStringBuilder?; elem : ExpressionPtr; last : bool) {
        //! Prints the separator between string builder elements.
        if (!last) {
            write(*writer, ",");
        }
        return elem;
    }
    def override preVisitExprNew(expr : ExprNew?) {
        //! Prints the opening of a new-expression with its type.
        write(*writer, "new {describe(expr.typeexpr)}");
        if (expr.initializer) {
            write(*writer, "(");
        }
    }
    def override visitExprNew(var expr : ExprNew?) : ExpressionPtr {
        //! Prints the closing parenthesis of a new-expression.
        if (expr.initializer) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override visitExprNewArgument(expr : ExprNew?; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between new-expression arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
    def override preVisitExprNamedCall(expr : ExprNamedCall?) {
        //! Prints the opening of a named-argument call expression.
        write(*writer, "{expr.name}([");
    }
    def override visitExprNamedCall(var expr : ExprNamedCall?) : ExpressionPtr {
        //! Prints the closing of a named-argument call expression.
        write(*writer, "])");
        return <- expr;
    }
    def override preVisitExprNamedCallArgument(expr : ExprNamedCall?; arg : MakeFieldDeclPtr; last : bool) {
        //! Prints a named call argument name and its assignment operator.
        write(*writer, "{arg.name}{arg.flags.moveSemantics ? "<-" : "="}");
    }
    def override visitExprNamedCallArgument(expr : ExprNamedCall?; arg : MakeFieldDeclPtr; last : bool) {
        //! Prints the separator between named call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
    def override preVisitExprLooksLikeCall(expr : ExprLooksLikeCall?) {
        //! Prints the opening of a looks-like-call expression (debug, assert, etc.).
        write(*writer, "{expr.name}(");
    }
    def override visitExprLooksLikeCall(var expr : ExprLooksLikeCall?) : ExpressionPtr {
        //! Prints the closing parenthesis of a looks-like-call expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprLooksLikeCallArgument(expr : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between looks-like-call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
    def override preVisitExprCall(expr : ExprCall?) {
        //! Prints the opening of a function call expression.
        write(*writer, "{expr.name}(");
    }
    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        //! Prints the closing parenthesis of a function call expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between function call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
    def override preVisitExprNullCoalescingDefault(expr : ExprNullCoalescing?; defval : ExpressionPtr) {
        //! Prints the null-coalescing operator (??).
        write(*writer, expr.no_promotion ? " !?? " : " ?? ");
    }
    def override visitExprAt(var expr : ExprAt?) : ExpressionPtr {
        //! Prints the closing bracket of an index-at expression.
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprAtIndex(expr : ExprAt?; index : ExpressionPtr) {
        //! Prints the opening bracket of an index-at expression.
        write(*writer, expr.atFlags.no_promotion ? "![" : "[");
    }
    def override visitExprSafeAt(var expr : ExprSafeAt?) : ExpressionPtr {
        //! Prints the closing bracket of a safe-index expression.
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprSafeAtIndex(expr : ExprSafeAt?; index : ExpressionPtr) {
        //! Prints the opening safe-index operator (?[).
        write(*writer, expr.atFlags.no_promotion ? "!?[" : "?[");
    }
    def override preVisitExprIsType(expr : ExprIs?; typeDecl : TypeDeclPtr) {
        //! Prints the is-type check expression.
        write(*writer, " {expr.no_promotion ? "!is" : "is"} type<{describe([decl=typeDecl,extra=false,contracts=true])}>");
    }
    def override preVisitExprOp2(expr : ExprOp2?) {
        //! Prints the opening parenthesis of a binary operator expression.
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp2(var expr : ExprOp2?) : ExpressionPtr {
        //! Prints the closing parenthesis of a binary operator expression.
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp2Right(expr : ExprOp2?; right : ExpressionPtr) {
        //! Prints the binary operator between left and right operands.
        write(*writer, " {expr.op} ");
    }
    def override preVisitExprOp3(expr : ExprOp3?) {
        //! Prints the opening parenthesis of a ternary operator expression.
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp3(var expr : ExprOp3?) : ExpressionPtr {
        //! Prints the closing parenthesis of a ternary operator expression.
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp3Left(expr : ExprOp3?; left : ExpressionPtr) {
        //! Prints the ternary '?' separator before the true branch.
        write(*writer, " ? ");
    }
    def override preVisitExprOp3Right(expr : ExprOp3?; right : ExpressionPtr) {
        //! Prints the ternary ':' separator before the false branch.
        write(*writer, " : ");
    }
    def override preVisitExprCopyRight(expr : ExprCopy?; right : ExpressionPtr) {
        //! Prints the copy assignment operator (=), or its raw form (!==).
        write(*writer, expr.copy_flags.no_promotion ? " !== " : " = ");
    }
    def override preVisitExprMoveRight(expr : ExprMove?; right : ExpressionPtr) {
        //! Prints the move operator (<-), or its raw form (!<-).
        write(*writer, expr.move_flags.no_promotion ? " !<- " : " <- ");
    }
    def override preVisitExprCloneRight(expr : ExprClone?; right : ExpressionPtr) {
        //! Prints the clone operator (:=), or its raw form (!:=).
        write(*writer, expr.clone_flags.no_promotion ? " !:= " : " := ");
    }
    def override preVisitExprWith(expr : ExprWith?) {
        //! Prints the 'with' keyword of a with-expression.
        write(*writer, "with ");
        if (!empty(expr.moduleName)) {
            write(*writer, "(module {expr.moduleName})");
        }
    }
    def override preVisitExprWithBody(expr : ExprWith?; right : ExpressionPtr) {
        //! Prints the separator before the with-expression body.
        write(*writer, " ");
    }
    def override preVisitExprWhile(expr : ExprWhile?) {
        //! Prints the 'while' keyword of a while-loop.
        write(*writer, "while ");
    }
    def override preVisitExprWhileBody(expr : ExprWhile?; right : ExpressionPtr) {
        //! Prints the separator before the while-loop body.
        write(*writer, " ");
    }
    def override preVisitExprTryCatch(expr : ExprTryCatch?) {
        //! Prints the 'try' keyword of a try-catch expression.
        write(*writer, "try ");
    }
    def override preVisitExprTryCatchCatch(expr : ExprTryCatch?; right : ExpressionPtr) {
        //! Prints the 'recover' keyword before the catch block.
        write(*writer, " recover ");
    }
    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) {
        //! Prints the 'if' keyword and opening parenthesis of an if-expression.
        write(*writer, "if (");
    }
    def override preVisitExprIfThenElseIfBlock(expr : ExprIfThenElse?; ifBlock : ExpressionPtr) {
        //! Prints the closing parenthesis before the if-block body.
        write(*writer, ") ");
    }
    def override preVisitExprIfThenElseElseBlock(expr : ExprIfThenElse?; elseBlock : ExpressionPtr) {
        //! Prints the 'else' keyword before the else block.
        write(*writer, " else ");
    }
    def override preVisitExprFor(expr : ExprFor?) {
        //! Prints the 'for' keyword and opening parenthesis of a for-loop.
        write(*writer, "for (");
    }
    def override preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) {
        //! Prints a for-loop iteration variable name.
        write(*writer, "{svar.name}{last ? " in " : ","}");
    }
    def override visitExprForSource(var expr : ExprFor?; var source : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints the separator between for-loop source expressions.
        if (!last) {
            write(*writer, ",");
        }
        return <- source;
    }
    def override preVisitExprForBody(expr : ExprFor?) {
        //! Prints the closing parenthesis before the for-loop body.
        write(*writer, ") ");
    }
    def override preVisitExprMakeVariant(expr : ExprMakeVariant?) {
        //! Prints the opening of a make-variant expression.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}");
        }
        write(*writer, "(");
    }
    def override visitExprMakeVariant(var expr : ExprMakeVariant?) : ExpressionPtr {
        //! Prints closing parenthesis of a make-variant expression.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints field name and assignment operator in a make-variant expression.
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints separator between variant fields.
        if (!last) {
            write(*writer, "; ");
        }
        return decl;
    }
    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        //! Prints opening of a make-struct expression with type name.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}({expr.makeStructFlags.useInitializer ? "" : "uninitialized"} ");
        } else {
            write(*writer, "(")
        }
    }
    def override visitExprMakeStruct(var expr : ExprMakeStruct?) : ExpressionPtr {
        //! Prints closing parenthesis of a make-struct expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeStructIndex(expr : ExprMakeStruct?; index : int; last : bool) {
        //! Prints semicolon separator between struct initializer groups.
        if (!last) {
            write(*writer, "; ");
        }
    }
    def override preVisitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints field name and assignment operator in a make-struct expression.
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints comma separator between struct fields.
        if (!last) {
            write(*writer, ", ");
        }
        return decl;
    }
    def override preVisitExprMakeArray(expr : ExprMakeArray?) {
        //! Prints opening of a fixed_array literal expression.
        write(*writer, "fixed_array");
        if (expr.recordType != null) {
            write(*writer, "<{describe(expr.recordType)}>");
        }
        write(*writer, "(");
    }
    def override visitExprMakeArray(var expr : ExprMakeArray?) : ExpressionPtr {
        //! Prints closing parenthesis of a fixed_array literal.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints comma separator between array literal elements.
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
    def override preVisitExprMakeTuple(expr : ExprMakeTuple?) {
        //! Prints opening of a tuple literal expression.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)} ");
        }
        write(*writer, "(");
    }
    def override visitExprMakeTuple(var expr : ExprMakeTuple?) : ExpressionPtr {
        //! Prints closing parenthesis of a tuple literal.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeTupleIndex(expr : ExprMakeTuple?; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints comma separator between tuple elements.
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
    def override preVisitExprArrayComprehension(expr : ExprArrayComprehension?) {
        //! Prints opening bracket of an array or table comprehension.
        if (expr.tableSyntax) {
            write(*writer, "\{");
        } else {
            write(*writer, "[");
        }
    }
    def override visitExprArrayComprehension(var expr : ExprArrayComprehension?) : ExpressionPtr {
        //! Prints closing bracket of an array or table comprehension.
        if (expr.tableSyntax) {
            write(*writer, "\}");
        } else {
            write(*writer, "]");
        }
        return <- expr;
    }
    def override preVisitExprArrayComprehensionSubexpr(expr : ExprArrayComprehension?; subexrp : ExpressionPtr) {
        //! Prints semicolon separator before comprehension subexpression.
        write(*writer, "; ");
    }
    def override preVisitExprArrayComprehensionWhere(expr : ExprArrayComprehension?; filter : ExpressionPtr) {
        //! Prints the where clause of an array comprehension.
        write(*writer, "; where ");
    }

    def override preVisitExprTypeDecl(var expr : ExprTypeDecl?) {
        //! Prints a type declaration expression.
        write(*writer, "type<{describe(expr.typeexpr)}>")
    }

    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
        //! Prints opening of a typeinfo expression with trait and subtrait.
        write(*writer, "typeinfo({expr.trait}");
        if (!empty(expr.subtrait)) {
            write(*writer, "<{expr.subtrait}");
            if (!empty(expr.extratrait)) {
                write(*writer, ";{expr.extratrait}");
            }
        }
        write(*writer, ">");
        if (expr.subexpr == null) {
            write(*writer, "type<{describe(expr.typeexpr)}>");
        }
    }
    def override visitExprTypeInfo(var expr : ExprTypeInfo?) : ExpressionPtr {
        //! Prints closing parenthesis of a typeinfo expression.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprPtr2Ref(expr : ExprPtr2Ref?) {
        //! Prints opening of a pointer dereference (deref) expression.
        write(*writer, "deref(");
    }
    def override visitExprPtr2Ref(var expr : ExprPtr2Ref?) : ExpressionPtr {
        //! Prints closing parenthesis of a deref expression.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprLabel(expr : ExprLabel?) {
        //! Prints a label statement with optional comment.
        write(*writer, "label {expr.labelName}:");
        if (!empty(expr.comment)) {
            write(*writer, "/*{expr.comment}*/");
        }
    }
    def override preVisitExprGoto(expr : ExprGoto?) {
        //! Prints a goto statement with target label.
        write(*writer, "goto ");
        if (expr.subexpr == null) {
            write(*writer, "label {expr.labelName}");
        }
    }
    def override preVisitExprRef2Value(expr : ExprRef2Value?) {
        //! Prints opening of a reference-to-value conversion.
        write(*writer, "/*r2v*/ (");
    }
    def override visitExprRef2Value(var expr : ExprRef2Value?) : ExpressionPtr {
        //! Prints closing parenthesis of a reference-to-value conversion.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprRef2Ptr(expr : ExprRef2Ptr?) {
        //! Prints opening of an addr (reference-to-pointer) expression.
        write(*writer, "addr(");
    }
    def override visitExprRef2Ptr(var expr : ExprRef2Ptr?) : ExpressionPtr {
        //! Prints closing parenthesis of an addr expression.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprAddr(expr : ExprAddr?) {
        //! Prints a @@ function pointer address expression.
        write(*writer, "@@");
        if (expr.funcType != null) {
            write(*writer, "<{describe(expr.funcType)}>");
        }
        write(*writer, "{expr.target}");
    }
    def override preVisitExprAscend(expr : ExprAscend?) {
        //! Prints a new (heap allocation) expression.
        if (expr.ascType != null) {
            write(*writer, "new<{describe(expr.ascType)}> ");
        } else {
            write(*writer, "new ");
        }
    }
    def override preVisitExprCast(expr : ExprCast?) {
        //! Prints a cast, upcast, or reinterpret expression.
        if (expr.castFlags.reinterpretCast) {
            write(*writer, "reinterpret");
        } elif (expr.castFlags.upcastCast) {
            write(*writer, "upcast");
        } else {
            write(*writer, "cast");
        }
        write(*writer, "<{describe(expr.castType)}> ");
    }
    def override preVisitExprDelete(expr : ExprDelete?) {
        //! Prints a delete expression.
        write(*writer, "delete ");
        if (expr.native) {
            write(*writer, "/*native*/ ");
        }
    }
    def override preVisitExprVar(expr : ExprVar?) {
        //! Prints a variable name reference.
        write(*writer, "{expr.name}");
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        //! Prints a dot-field access expression.
        write(*writer, "{expr.fieldFlags.no_promotion ? "!." : "."}{expr.name}");
        return <- expr;
    }
    def override visitExprSafeField(var expr : ExprSafeField?) : ExpressionPtr {
        //! Prints a safe-navigation (?.) field access expression.
        write(*writer, "{expr.fieldFlags.no_promotion ? "!?." : "?."}{expr.name}");
        return <- expr;
    }
    def override visitExprSwizzle(var expr : ExprSwizzle?) : ExpressionPtr {
        //! Prints a vector swizzle access (e.g. .xyz).
        write(*writer, expr.fieldFlags.no_promotion ? "!." : ".");
        let f2name = fixed_array<string>("x", "y", "z", "w");
        for (fch in expr.fields) {
            let f = int(fch);
            write(*writer, "{f>=0 && f<=3 ? f2name[f] : "?"}");
        }
        return <- expr;
    }
    def override visitExprIsVariant(var expr : ExprIsVariant?) : ExpressionPtr {
        //! Prints an `is` variant type check expression.
        write(*writer, " {expr.fieldFlags.no_promotion ? "!is" : "is"} {expr.name}");
        return <- expr;
    }
    def override visitExprAsVariant(var expr : ExprAsVariant?) : ExpressionPtr {
        //! Prints an `as` variant type conversion expression.
        write(*writer, " {expr.fieldFlags.no_promotion ? "!as" : "as"} {expr.name}");
        return <- expr;
    }
    def override visitExprSafeAsVariant(var expr : ExprSafeAsVariant?) : ExpressionPtr {
        //! Prints a safe `?as` variant type conversion expression.
        write(*writer, " {expr.fieldFlags.no_promotion ? "!?as" : "?as"} {expr.name}");
        return <- expr;
    }
    def override preVisitExprOp1(expr : ExprOp1?) : void {
        //! Prints prefix portion of a unary operator expression.
        let op = "{expr.op}";
        if (op != "+++" && op != "---") {
            write(*writer, op);
        }
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp1(var expr : ExprOp1?) : ExpressionPtr {
        //! Prints postfix portion of a unary operator (closing bracket or postfix op).
        let op = "{expr.op}";
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        if (op == "+++" || op == "---") {
            write(*writer, "{slice(op,0,-1)}");
        }
        return <- expr;
    }
    def override preVisitExprReturn(expr : ExprReturn?) : void {
        //! Prints a return statement with optional move semantics.
        write(*writer, "return ");
        if (expr.returnFlags.fromYield) {
            write(*writer, "/* from yield */ ");
        }
        if (expr.returnFlags.moveSemantics) {
            write(*writer, "<- ");
        }
    }
    def override preVisitExprYield(expr : ExprYield?) : void {
        //! Prints a yield expression with optional move semantics.
        write(*writer, "yield ");
        if (expr.returnFlags.moveSemantics) {
            write(*writer, "<- ");
        }
    }
    def override preVisitExprBreak(expr : ExprBreak?) : void {
        //! Prints a break statement.
        write(*writer, "break");
    }
    def override preVisitExprContinue(expr : ExprContinue?) : void {
        //! Prints a continue statement.
        write(*writer, "continue");
    }
    def override preVisitExprConstPtr(expr : ExprConstPtr?) : void {
        //! Prints a constant pointer value or null.
        if (expr.value != null) {
            write(*writer, "{expr.value}");
        } else {
            write(*writer, "null");
        }
    }
    def override preVisitExprConstInt8(expr : ExprConstInt8?) : void {
        //! Prints an int8 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstInt16(expr : ExprConstInt16?) : void {
        //! Prints an int16 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstFloat16(expr : ExprConstFloat16?) : void {
        //! Prints a float16 constant value with its literal suffix.
        write(*writer, "{expr.value}h");
    }
    def override preVisitExprConstInt64(expr : ExprConstInt64?) : void {
        //! Prints an int64 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstInt(expr : ExprConstInt?) : void {
        //! Prints an int constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstInt2(expr : ExprConstInt2?) : void {
        //! Prints an int2 vector constant value.
        write(*writer, "int2({expr.value})");
    }
    def override preVisitExprConstInt3(expr : ExprConstInt3?) : void {
        //! Prints an int3 vector constant value.
        write(*writer, "int3({expr.value})");
    }
    def override preVisitExprConstInt4(expr : ExprConstInt4?) : void {
        //! Prints an int4 vector constant value.
        write(*writer, "int4({expr.value})");
    }
    def override preVisitExprConstUInt8(expr : ExprConstUInt8?) : void {
        //! Prints a uint8 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstUInt16(expr : ExprConstUInt16?) : void {
        //! Prints a uint16 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstUInt64(expr : ExprConstUInt64?) : void {
        //! Prints a uint64 constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstUInt(expr : ExprConstUInt?) : void {
        //! Prints a uint constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstUInt2(expr : ExprConstUInt2?) : void {
        //! Prints a uint2 vector constant value.
        write(*writer, "uint2({expr.value})");
    }
    def override preVisitExprConstUInt3(expr : ExprConstUInt3?) : void {
        //! Prints a uint3 vector constant value.
        write(*writer, "uint3({expr.value})");
    }
    def override preVisitExprConstUInt4(expr : ExprConstUInt4?) : void {
        //! Prints a uint4 vector constant value.
        write(*writer, "uint4({expr.value})");
    }
    def override preVisitExprConstRange(expr : ExprConstRange?) : void {
        //! Prints a range constant value.
        write(*writer, "range({expr.value})");
    }
    def override preVisitExprConstURange(expr : ExprConstURange?) : void {
        //! Prints a urange constant value.
        write(*writer, "urange({expr.value})");
    }
    def override preVisitExprConstRange64(expr : ExprConstRange64?) : void {
        //! Prints a range64 constant value.
        write(*writer, "range64({expr.value})");
    }
    def override preVisitExprConstURange64(expr : ExprConstURange64?) : void {
        //! Prints a urange64 constant value.
        write(*writer, "urange64({expr.value})");
    }
    def override preVisitExprConstBool(expr : ExprConstBool?) : void {
        //! Prints a boolean constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstFloat(expr : ExprConstFloat?) : void {
        //! Prints a float constant value.
        write(*writer, "{expr.value}");
    }
    def override preVisitExprConstFloat2(expr : ExprConstFloat2?) : void {
        //! Prints a float2 vector constant value.
        write(*writer, "float2({expr.value})");
    }
    def override preVisitExprConstFloat3(expr : ExprConstFloat3?) : void {
        //! Prints a float3 vector constant value.
        write(*writer, "float3({expr.value})");
    }
    def override preVisitExprConstFloat4(expr : ExprConstFloat4?) : void {
        //! Prints a float4 vector constant value.
        write(*writer, "float4({expr.value})");
    }
    def override preVisitExprConstDouble(expr : ExprConstDouble?) : void {
        //! Prints a double constant value with lf suffix.
        write(*writer, "{expr.value}lf");
    }
    def override preVisitExprFakeContext(expr : ExprFakeContext?) : void {
        //! Prints the __context__ intrinsic.
        write(*writer, "__context__");
    }
    def override preVisitExprFakeLineInfo(expr : ExprFakeLineInfo?) : void {
        //! Prints the __lineinfo__ intrinsic.
        write(*writer, "__lineinfo__");
    }
    def override preVisitExprConstString(expr : ExprConstString?) : void {
        //! Prints a string constant with proper escaping.
        write(*writer, "\"{escape(expr.value)}\"");
    }
    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) : void {
        //! Prints a constant enumeration value with module prefix.
        let enumModule = expr.enumType._module;
        if (enumModule != null && !empty(enumModule.name)) {
            write(*writer, "{enumModule.name}::");
        }
        write(*writer, "{expr.enumType.name}.{expr.value}");
    }
    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) : void {
        //! Prints a constant bitfield value by name or numeric fallback.
        var name : string;
        if (expr.bitfieldType != null && !empty(expr.bitfieldType.alias)) {
            name = find_bitfield_name(expr.bitfieldType, expr.value);
        }
        if (!empty(name)) {
            write(*writer, "{expr.bitfieldType.alias}.{name}");
        } else {
            write(*writer, "bitfield({expr.value})");
        }
    }
}

//! Type alias for int, used in self-test pretty-printing.
typedef Integer = int
//! Variant type alias holding either an int or a float, used in self-test pretty-printing.
typedef IorF = variant<i : int; f : float>


enum FooBar {
    //! Test enumeration used for self-test pretty-printing.
    Foo
    Bar = 13
}


//! Test structure used for self-test pretty-printing.
[comment(x=5, msg="haha")]
struct Foo {
    //! Test structure used for self-test pretty-printing.
    @do_not_delete@temp = 13 a : int = 11;
    b : array<Foo?>;
}

def Foo(x : int) {
    //! Constructs a Foo with the given int assigned to field a.
    return <- Foo(a = x);
}

def Foo(x, y : int) {
    //! Constructs a Foo with field a set to x + y.
    return <- Foo(a = x + y);
}

[sideeffects]
def add(a, b : int) {
    //! Adds two integers with an extra constant, used for AST print testing.
    // todo make global, after globals supported in standalone ctx
    let add_extra = 13;
    print("a={a} b={b}");
    return a + b + add_extra;
}

[sideeffects, export, no_lint]
def allExpr(arg : int) { // nolint:STYLE038 — coverage fixture: one of every expression node, flat by design
    //! Exercises all expression types for AST pretty-printer coverage.
    // ExprStringBuilder
    var sb = "arg = {arg}\n";
    sb = "";
    add([a=1, b=2]);
    add(3, 4, [a=1, b=2]);
    var pFoo1 = new Foo;
    var pFoo2 = new Foo(1);
    var pFoo3 = new Foo(1, 2);
    add(1, 2);
    var exprAt <- pFoo1 ?? Foo(a = 1);
    exprAt <- Foo(a = 2); // nolint:PERF030 — the move-assign shape itself is the printer coverage
    pFoo1.b[0] = pFoo2;
    var exprSafeAt = pFoo1.b?[0] ?? pFoo2;
    exprSafeAt = pFoo2;
    if (exprAt is type<int>) {
        print("nada\n");
    }
    var aa = 1;
    aa = 3;
    var bb = 2;
    bb = 4;
    var cc = aa + bb;
    cc = aa == bb ? aa : bb;
    with (exprAt) {
        aa = 3;
    }
    while (aa != 100) {
        aa++;
    }
    try {
        aa = 2;
    } recover {
        aa = 3;
    }
    if (aa == 2) {
        aa = 2;
    }
    if (aa == 2) {
        aa = 2;
    } else {
        aa = 3;
    }
    if (aa == 2) {
        aa = 2;
    } elif (aa == 3) {
        aa = 3;
    } else {
        aa = 4;
    }
    var mkv = IorF(i = 5);
    mkv = IorF(f = 1.0);
    var mks <- [Foo(a=5, b <- array<Foo?>(null)), Foo(a = 6)];
    mks[1] <- Foo(a = 5);
    var mka = fixed_array<int>(1, 2, 3, 4);
    mka = fixed_array<int>(5, 6, 7, 8);
    var mkt =  (1, 2.0, "three");
    mkt =  (2, 3.0, "four");
    var acomp <- [for (x in range(3)); x * x; where x != 1];
    var gcomp <- [iterator for(x in range(3)); x * x; where x != 1];
    for (ta, tg in acomp, gcomp) {
        verify(ta == tg);
    }
    let tinfo = typeinfo rtti_typeinfo(type<int>).basicType;
    print("{tinfo}\n");
    print("{*pFoo1}");
    goto label 1;
    print("nop");
    label 1:;
    let aacomp = unsafe(addr(acomp));
    print("{aacomp}");
    let eaddr = @@add;
    print("{eaddr}");
    assert(aa == 4, "assert here");
    verify(aa == 4, "verify here");
    static_assert(true, "static assert here");
    concept_assert(true, "concept assert here");
    debug(aa);
    let atadd = @@add;
    invoke(atadd, 1, 2);
    var tab <- { 1 => "one", 2 => "two" };
    erase(tab, 1);
    get(tab, 2) $(argX) {
        print("{argX}");
    }
    assert(key_exists(tab, 2));
    let easc = new fixed_array(1, 2, 3, 4);
    print("{easc}");
    let aaf = unsafe(reinterpret<float>(aa));
    print("{aaf}");
    unsafe {
        delete pFoo1;
    }
    var fsw = float4(1, 2, 3, 4);
    fsw = fsw.yyzx;
    var eop1 = 1;
    eop1 ++;
    -- eop1;
    memzero(eop1);
    var cstr = "hello\nworld\n";
    cstr += "\{..\}";
    var enc = Type.none;
    enc = Type.tInt;
    var bfc = ExprPrintFlags.topLevel;
    bfc = ExprPrintFlags.argLevel;
}

[export]
def test {
    //! Self-test: visits this program's AST and prints it.
    var astVisitor = new PrintVisitor();
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit(this_program(), astVisitorAdapter);
    }

    return true;
}

[export]
def printAst(var prog : ProgramPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints the entire program AST to the given writer.
    var flags = new PrintVisitor(writer = writer);
    make_visitor(*flags) $(adapter) {
        visit(prog, adapter)
    }
}

[export]
def printExpr(var expr : ExpressionPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints a single expression AST node to the given writer.
    var flags = new PrintVisitor(writer = writer);
    make_visitor(*flags) $(adapter) {
        visit(expr, adapter)
    }
}

[export]
def printFunc(var func : FunctionPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints a single function AST node to the given writer.
    var flags = new PrintVisitor(writer = writer);
    make_visitor(*flags) $(adapter) {
        visit(func, adapter)
    }
}

[export]
def setFlags(var prog : ProgramPtr) {
    //! Visits the program AST to set printer flags before pretty-printing.
    var flags = new SetPrinterFlags();
    make_visitor(*flags) $(adapter) {
        visit(prog, adapter)
    }
}
