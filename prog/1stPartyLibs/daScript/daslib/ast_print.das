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
    // TODO: fixme

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

// program
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
// type
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
// alias
    def override preVisitAlias(typ : TypeDeclPtr; name : das_string) {
        //! Prints a type alias declaration.
        write(*writer, "typedef {name} = {describe([decl=typ,extra=extraTypeInfo])}\n\n");
    }
// enumeration
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
// structure
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
// function
    def override preVisitFunction(fun : FunctionPtr) {
        //! Prints the function declaration header with annotations and name.
        if (fun.flags.knownSideEffects) {
            if (fun.sideEffectFlags == bitfield(0)) {
                write(*writer, "// [nosideeffects]\n");
            } else {
                // // todo: describe_bitfield not supported in aot yet
                // write(*writer, "// {describe_bitfield(fun.sideEffectFlags)}\n");
            }
        }
        // let bfa : FunctionFlags = fun.flags & function_annotation_flags;
        // // todo: describe_bitfield not supported in aot yet
        // write(*writer, "{describe_bitfield(bfa,"\n")}");
        if (fun.annotations |> length != 0) {
            write(*writer, "[{describe(fun.annotations)}]\n");
        }
        write(*writer, "def {fun.name}");
        if (fun.arguments |> length != 0) {
            write(*writer, " ( ");
        }
    }
    def override preVisitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) {
        //! Prints the closing parenthesis of arguments and the return type.
        if (fun.arguments |> length != 0) {
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
        if (arg.isAccessUnused) {
            write(*writer, " /*unsued*/ ");
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
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
// block
    def override preVisitExprBlock(blk : smart_ptr<ExprBlock>) {
        //! Prints the opening of a block expression, including closure arguments.
        if (blk.blockFlags.isClosure) {
            if (blk.returnType != null || blk.arguments |> length != 0) {
                write(*writer, "$(");
                for (arg, argIndex in blk.arguments, range(blk.arguments |> length)) {
                    if (arg.annotation |> length != 0) {
                        write(*writer, "[{describe(arg.annotation)}] ");
                    }
                    if (arg._type.flags.constant) {
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
    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        //! Prints the closing brace of a block expression.
        tab --;
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident()}\}");
        }
        return <- blk;
    }
    def override preVisitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        //! Prints indentation before a block sub-expression.
        write(*writer, "{ident()}");
    }
    def override visitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        //! Prints a newline after a block sub-expression.
        self->newLine();
        return expr;
    }
    def override preVisitExprBlockFinal(blk : smart_ptr<ExprBlock>) {
        //! Prints the finally block header.
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident(tab-1)}\} finally \{\n");
        } else {
            write(*writer, "{ident(tab-1)}finally\n");
        }
    }
    def override preVisitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        //! Prints indentation before a finally-block expression.
        write(*writer, "{ident()}");
    }
    def override visitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        //! Prints a newline after a finally-block expression.
        self->newLine();
        return expr;
    }
// let
    def override preVisitExprLet(expr : smart_ptr<ExprLet>) {
        //! Prints the let or var keyword for a variable declaration.
        var isLet = true;
        for (pv in expr.variables) {
            if (pv._type != null && pv._type.flags.constant) {
                isLet = false;
                break;
            }
        }
        write(*writer, "{isLet ? "let" : "var"} ");
    }
    def override preVisitExprLetVariable(expr : smart_ptr<ExprLet>; arg : VariablePtr; lastArg : bool) {
        //! Prints a let-variable name and its type.
        if (arg.isAccessUnused) {
            write(*writer, " /*unused*/ ");
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
    }
    def override visitExprLetVariable(expr : smart_ptr<ExprLet>; arg : VariablePtr; lastArg : bool) {
        //! Prints the separator between let-variable declarations.
        if (!lastArg) {
            write(*writer, ";");
        }
        return arg;
    }
    def override preVisitExprLetVariableInit(blk : smart_ptr<ExprLet>; arg : VariablePtr; expr : ExpressionPtr) {
        //! Prints the initializer operator for a let-variable.
        if (arg.flags.init_via_move) {
            write(*writer, " <- ");
        } elif (arg.flags.init_via_clone) {
            write(*writer, " := ");
        } else {
            write(*writer, " = ");
        }
    }
// global let
    def override preVisitGlobalLetVariable(arg : VariablePtr; lastArg : bool) {
        //! Prints a global variable declaration with its type.
        write(*writer, "{arg._type.flags.constant ? "let" : "var"}{arg.flags.global_shared ? " shared" : ""}\n{ident(1)}");
        if (arg.isAccessUnused) {
            write(*writer, " /*unused*/ ");
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
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
// string builder
    def override preVisitExprStringBuilder(expr : smart_ptr<ExprStringBuilder>) {
        //! Prints the opening of a string builder expression.
        write(*writer, "string_builder(");
    }
    def override visitExprStringBuilder(var expr : smart_ptr<ExprStringBuilder>) : ExpressionPtr {
        //! Prints the closing parenthesis of a string builder expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprStringBuilderElement(expr : smart_ptr<ExprStringBuilder>; elem : ExpressionPtr; last : bool) {
        //! Prints the separator between string builder elements.
        if (!last) {
            write(*writer, ",");
        }
        return elem;
    }
// new
    def override preVisitExprNew(expr : smart_ptr<ExprNew>) {
        //! Prints the opening of a new-expression with its type.
        write(*writer, "new {describe(expr.typeexpr)}");
        if (expr.initializer) {
            write(*writer, "(");
        }
    }
    def override visitExprNew(var expr : smart_ptr<ExprNew>) : ExpressionPtr {
        //! Prints the closing parenthesis of a new-expression.
        if (expr.initializer) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override visitExprNewArgument(expr : smart_ptr<ExprNew>; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between new-expression arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// named call
    def override preVisitExprNamedCall(expr : smart_ptr<ExprNamedCall>) {
        //! Prints the opening of a named-argument call expression.
        write(*writer, "{expr.name}([");
    }
    def override visitExprNamedCall(var expr : smart_ptr<ExprNamedCall>) : ExpressionPtr {
        //! Prints the closing of a named-argument call expression.
        write(*writer, "])");
        return <- expr;
    }
    def override preVisitExprNamedCallArgument(expr : smart_ptr<ExprNamedCall>; arg : MakeFieldDeclPtr; last : bool) {
        //! Prints a named call argument name and its assignment operator.
        write(*writer, "{arg.name}{arg.flags.moveSemantics ? "<-" : "="}");
    }
    def override visitExprNamedCallArgument(expr : smart_ptr<ExprNamedCall>; arg : MakeFieldDeclPtr; last : bool) {
        //! Prints the separator between named call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// looks like call (debug,assert,verify,erase,find,key_exists,keys,values,invoke,memzero etc)
    def override preVisitExprLooksLikeCall(expr : smart_ptr<ExprLooksLikeCall>) {
        //! Prints the opening of a looks-like-call expression (debug, assert, etc.).
        write(*writer, "{expr.name}(");
    }
    def override visitExprLooksLikeCall(var expr : smart_ptr<ExprLooksLikeCall>) : ExpressionPtr {
        //! Prints the closing parenthesis of a looks-like-call expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprLooksLikeCallArgument(expr : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between looks-like-call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// call
    def override preVisitExprCall(expr : smart_ptr<ExprCall>) {
        //! Prints the opening of a function call expression.
        write(*writer, "{expr.name}(");
    }
    def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr {
        //! Prints the closing parenthesis of a function call expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprCallArgument(expr : smart_ptr<ExprCall>; arg : ExpressionPtr; last : bool) {
        //! Prints the separator between function call arguments.
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// null coaelescing
    def override preVisitExprNullCoalescingDefault(expr : smart_ptr<ExprNullCoalescing>; defval : ExpressionPtr) {
        //! Prints the null-coalescing operator (??).
        write(*writer, " ?? ");
    }
// at
    def override visitExprAt(var expr : smart_ptr<ExprAt>) : ExpressionPtr {
        //! Prints the closing bracket of an index-at expression.
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprAtIndex(expr : smart_ptr<ExprAt>; index : ExpressionPtr) {
        //! Prints the opening bracket of an index-at expression.
        write(*writer, "[");
    }
// safe at
    def override visitExprSafeAt(var expr : smart_ptr<ExprSafeAt>) : ExpressionPtr {
        //! Prints the closing bracket of a safe-index expression.
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprSafeAtIndex(expr : smart_ptr<ExprSafeAt>; index : ExpressionPtr) {
        //! Prints the opening safe-index operator (?[).
        write(*writer, "?[");
    }
// is
    def override preVisitExprIsType(expr : smart_ptr<ExprIs>; typeDecl : TypeDeclPtr) {
        //! Prints the is-type check expression.
        write(*writer, "is type<{describe([decl=typeDecl,extra=false,contracts=true])}>");
    }
// op2
    def override preVisitExprOp2(expr : smart_ptr<ExprOp2>) {
        //! Prints the opening parenthesis of a binary operator expression.
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp2(var expr : smart_ptr<ExprOp2>) : ExpressionPtr {
        //! Prints the closing parenthesis of a binary operator expression.
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp2Right(expr : smart_ptr<ExprOp2>; right : ExpressionPtr) {
        //! Prints the binary operator between left and right operands.
        write(*writer, " {expr.op} ");
    }
// op3
    def override preVisitExprOp3(expr : smart_ptr<ExprOp3>) {
        //! Prints the opening parenthesis of a ternary operator expression.
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp3(var expr : smart_ptr<ExprOp3>) : ExpressionPtr {
        //! Prints the closing parenthesis of a ternary operator expression.
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp3Left(expr : smart_ptr<ExprOp3>; left : ExpressionPtr) {
        //! Prints the ternary '?' separator before the true branch.
        write(*writer, " ? ");
    }
    def override preVisitExprOp3Right(expr : smart_ptr<ExprOp3>; right : ExpressionPtr) {
        //! Prints the ternary ':' separator before the false branch.
        write(*writer, " : ");
    }
// copy
    def override preVisitExprCopyRight(expr : smart_ptr<ExprCopy>; right : ExpressionPtr) {
        //! Prints the copy assignment operator (=).
        write(*writer, " = ");
    }
// move
    def override preVisitExprMoveRight(expr : smart_ptr<ExprMove>; right : ExpressionPtr) {
        //! Prints the move operator (<-).
        write(*writer, " <- ");
    }
// clone
    def override preVisitExprCloneRight(expr : smart_ptr<ExprClone>; right : ExpressionPtr) {
        //! Prints the clone operator (:=).
        write(*writer, " := ");
    }
// with
    def override preVisitExprWith(expr : smart_ptr<ExprWith>) {
        //! Prints the 'with' keyword of a with-expression.
        write(*writer, "with ");
    }
    def override preVisitExprWithBody(expr : smart_ptr<ExprWith>; right : ExpressionPtr) {
        //! Prints the separator before the with-expression body.
        write(*writer, " ");
    }
// while
    def override preVisitExprWhile(expr : smart_ptr<ExprWhile>) {
        //! Prints the 'while' keyword of a while-loop.
        write(*writer, "while ");
    }
    def override preVisitExprWhileBody(expr : smart_ptr<ExprWhile>; right : ExpressionPtr) {
        //! Prints the separator before the while-loop body.
        write(*writer, " ");
    }
// try-catch
    def override preVisitExprTryCatch(expr : smart_ptr<ExprTryCatch>) {
        //! Prints the 'try' keyword of a try-catch expression.
        write(*writer, "try ");
    }
    def override preVisitExprTryCatchCatch(expr : smart_ptr<ExprTryCatch>; right : ExpressionPtr) {
        //! Prints the 'recover' keyword before the catch block.
        write(*writer, " recover ");
    }
// if-then-else
    def override preVisitExprIfThenElse(expr : smart_ptr<ExprIfThenElse>) {
        //! Prints the 'if' keyword and opening parenthesis of an if-expression.
        write(*writer, "if (");
    }
    def override preVisitExprIfThenElseIfBlock(expr : smart_ptr<ExprIfThenElse>; ifBlock : ExpressionPtr) {
        //! Prints the closing parenthesis before the if-block body.
        write(*writer, ") ");
    }
    def override preVisitExprIfThenElseElseBlock(expr : smart_ptr<ExprIfThenElse>; elseBlock : ExpressionPtr) {
        //! Prints the 'else' keyword before the else block.
        if (elseBlock.__rtti == "ExprIfThenElse") {
            write(*writer, " else ");
        } else {
            write(*writer, " else ");
        }
    }
// for
    def override preVisitExprFor(expr : smart_ptr<ExprFor>) {
        //! Prints the 'for' keyword and opening parenthesis of a for-loop.
        write(*writer, "for (");
    }
    def override preVisitExprForVariable(expr : smart_ptr<ExprFor>; svar : VariablePtr; last : bool) {
        //! Prints a for-loop iteration variable name.
        write(*writer, "{svar.name}{last ? " in " : ","}");
    }
    def override visitExprForSource(var expr : smart_ptr<ExprFor>; var source : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints the separator between for-loop source expressions.
        if (!last) {
            write(*writer, ",");
        }
        return <- source;
    }
    def override preVisitExprForBody(expr : smart_ptr<ExprFor>) {
        //! Prints the closing parenthesis before the for-loop body.
        write(*writer, ") ");
    }
// make variant
    def override preVisitExprMakeVariant(expr : smart_ptr<ExprMakeVariant>) {
        //! Prints the opening of a make-variant expression.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}");
        }
        write(*writer, "(");
    }
    def override visitExprMakeVariant(var expr : smart_ptr<ExprMakeVariant>) : ExpressionPtr {
        //! Prints closing parenthesis of a make-variant expression.
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints field name and assignment operator in a make-variant expression.
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints separator between variant fields.
        if (!last) {
            write(*writer, "; ");
        }
        return decl;
    }
// make structure
    def override preVisitExprMakeStruct(expr : smart_ptr<ExprMakeStruct>) {
        //! Prints opening of a make-struct expression with type name.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}({expr.makeStructFlags.useInitializer ? "" : "uninitialized"} ");
        } else {
            write(*writer, "(")
        }
    }
    def override visitExprMakeStruct(var expr : smart_ptr<ExprMakeStruct>) : ExpressionPtr {
        //! Prints closing parenthesis of a make-struct expression.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeStructIndex(expr : smart_ptr<ExprMakeStruct>; index : int; last : bool) {
        //! Prints semicolon separator between struct initializer groups.
        if (!last) {
            write(*writer, "; ");
        }
    }
    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints field name and assignment operator in a make-struct expression.
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        //! Prints comma separator between struct fields.
        if (!last) {
            write(*writer, ", ");
        }
        return decl;
    }
// make array
    def override preVisitExprMakeArray(expr : smart_ptr<ExprMakeArray>) {
        //! Prints opening of a fixed_array literal expression.
        write(*writer, "fixed_array");
        if (expr.recordType != null) {
            write(*writer, "<{describe(expr.recordType)}>");
        }
        write(*writer, "(");
    }
    def override visitExprMakeArray(var expr : smart_ptr<ExprMakeArray>) : ExpressionPtr {
        //! Prints closing parenthesis of a fixed_array literal.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints comma separator between array literal elements.
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
// make tuple
    def override preVisitExprMakeTuple(expr : smart_ptr<ExprMakeTuple>) {
        //! Prints opening of a tuple literal expression.
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)} ");
        }
        write(*writer, "(");
    }
    def override visitExprMakeTuple(var expr : smart_ptr<ExprMakeTuple>) : ExpressionPtr {
        //! Prints closing parenthesis of a tuple literal.
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        //! Prints comma separator between tuple elements.
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
// array comprehension
    def override preVisitExprArrayComprehension(expr : smart_ptr<ExprArrayComprehension>) {
        //! Prints opening bracket of an array or table comprehension.
        if (expr.tableSyntax) {
            write(*writer, "\{");
        } else {
            write(*writer, "[");
        }
    }
    def override visitExprArrayComprehension(var expr : smart_ptr<ExprArrayComprehension>) : ExpressionPtr {
        //! Prints closing bracket of an array or table comprehension.
        if (expr.tableSyntax) {
            write(*writer, "\}");
        } else {
            write(*writer, "]");
        }
        return <- expr;
    }
    def override preVisitExprArrayComprehensionSubexpr(expr : smart_ptr<ExprArrayComprehension>; subexrp : ExpressionPtr) {
        //! Prints semicolon separator before comprehension subexpression.
        write(*writer, "; ");
    }
    def override preVisitExprArrayComprehensionWhere(expr : smart_ptr<ExprArrayComprehension>; filter : ExpressionPtr) {
        //! Prints the where clause of an array comprehension.
        write(*writer, "; where ");
    }

    def override preVisitExprTypeDecl(var expr : smart_ptr<ExprTypeDecl>) {
        //! Prints a type declaration expression.
        write(*writer, "type<{describe(expr.typeexpr)}>")
    }

// type info
    def override preVisitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>) {
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
    def override visitExprTypeInfo(var expr : smart_ptr<ExprTypeInfo>) : ExpressionPtr {
        //! Prints closing parenthesis of a typeinfo expression.
        write(*writer, ")");
        return <- expr;
    }
// ptr to ref
    def override preVisitExprPtr2Ref(expr : smart_ptr<ExprPtr2Ref>) {
        //! Prints opening of a pointer dereference (deref) expression.
        write(*writer, "deref(");
    }
    def override visitExprPtr2Ref(var expr : smart_ptr<ExprPtr2Ref>) : ExpressionPtr {
        //! Prints closing parenthesis of a deref expression.
        write(*writer, ")");
        return <- expr;
    }
// label
    def override preVisitExprLabel(expr : smart_ptr<ExprLabel>) {
        //! Prints a label statement with optional comment.
        write(*writer, "label {expr.labelName}:");
        if (!empty(expr.comment)) {
            write(*writer, "/*{expr.comment}*/");
        }
    }
// goto
    def override preVisitExprGoto(expr : smart_ptr<ExprGoto>) {
        //! Prints a goto statement with target label.
        write(*writer, "goto ");
        if (expr.subexpr == null) {
            write(*writer, "label {expr.labelName}");
        }
    }
// ref to value
    def override preVisitExprRef2Value(expr : smart_ptr<ExprRef2Value>) {
        //! Prints opening of a reference-to-value conversion.
        write(*writer, "/*r2v*/ (");
    }
    def override visitExprRef2Value(var expr : smart_ptr<ExprRef2Value>) : ExpressionPtr {
        //! Prints closing parenthesis of a reference-to-value conversion.
        write(*writer, ")");
        return <- expr;
    }
// ref 2 ptr
    def override preVisitExprRef2Ptr(expr : smart_ptr<ExprRef2Ptr>) {
        //! Prints opening of an addr (reference-to-pointer) expression.
        write(*writer, "addr(");
    }
    def override visitExprRef2Ptr(var expr : smart_ptr<ExprRef2Ptr>) : ExpressionPtr {
        //! Prints closing parenthesis of an addr expression.
        write(*writer, ")");
        return <- expr;
    }
// @@
    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) {
        //! Prints a @@ function pointer address expression.
        write(*writer, "@@");
        if (expr.funcType != null) {
            write(*writer, "<{describe(expr.funcType)}>");
        }
        write(*writer, "{expr.target}");
    }
// ascend
    def override preVisitExprAscend(expr : smart_ptr<ExprAscend>) {
        //! Prints a new (heap allocation) expression.
        if (expr.ascType != null) {
            write(*writer, "new<{describe(expr.ascType)}> ");
        } else {
            write(*writer, "new ");
        }
    }
// cast
    def override preVisitExprCast(expr : smart_ptr<ExprCast>) {
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
// delete
    def override preVisitExprDelete(expr : smart_ptr<ExprDelete>) {
        //! Prints a delete expression.
        write(*writer, "delete ");
        if (expr.native) {
            write(*writer, "/*native*/ ");
        }
    }
// var
    def override preVisitExprVar(expr : smart_ptr<ExprVar>) {
        //! Prints a variable name reference.
        write(*writer, "{expr.name}");
    }
// field
    def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr {
        //! Prints a dot-field access expression.
        write(*writer, ".{expr.name}");
        return <- expr;
    }
// safe field
    def override visitExprSafeField(var expr : smart_ptr<ExprSafeField>) : ExpressionPtr {
        //! Prints a safe-navigation (?.) field access expression.
        write(*writer, "?.{expr.name}");
        return <- expr;
    }
// swizzle
    def override visitExprSwizzle(var expr : smart_ptr<ExprSwizzle>) : ExpressionPtr {
        //! Prints a vector swizzle access (e.g. .xyz).
        write(*writer, ".");
        let f2name = fixed_array<string>("x", "y", "z", "w");
        for (fch in expr.fields) {
            let f = int(fch);
            write(*writer, "{f>=0 && f<=3 ? f2name[f] : "?"}");
        }
        return <- expr;
    }
// is variant
    def override visitExprIsVariant(var expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        //! Prints an `is` variant type check expression.
        write(*writer, " is {expr.name}");
        return <- expr;
    }
// as variant
    def override visitExprAsVariant(var expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        //! Prints an `as` variant type conversion expression.
        write(*writer, " as {expr.name}");
        return <- expr;
    }
// safe as variant
    def override visitExprSafeAsVariant(var expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        //! Prints a safe `?as` variant type conversion expression.
        write(*writer, " ?as {expr.name}");
        return <- expr;
    }
// op1
    def override preVisitExprOp1(expr : smart_ptr<ExprOp1>) : void {
        //! Prints prefix portion of a unary operator expression.
        let op = "{expr.op}";
        if (op != "+++" && op != "---") {
            write(*writer, op);
        }
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp1(var expr : smart_ptr<ExprOp1>) : ExpressionPtr {
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
// return
    def override preVisitExprReturn(expr : smart_ptr<ExprReturn>) : void {
        //! Prints a return statement with optional move semantics.
        write(*writer, "return ");
        if (expr.returnFlags.fromYield) {
            write(*writer, "/* from yield */ ");
        }
        if (expr.returnFlags.moveSemantics) {
            write(*writer, "<- ");
        }
    }
// yield
    def override preVisitExprYield(expr : smart_ptr<ExprYield>) : void {
        //! Prints a yield expression with optional move semantics.
        write(*writer, "yield ");
        if (expr.returnFlags.moveSemantics) {
            write(*writer, "<- ");
        }
    }
// break
    def override preVisitExprBreak(expr : smart_ptr<ExprBreak>) : void {
        //! Prints a break statement.
        write(*writer, "break");
    }
// continue
    def override preVisitExprContinue(expr : smart_ptr<ExprContinue>) : void {
        //! Prints a continue statement.
        write(*writer, "continue");
    }
// const ptr
    def override preVisitExprConstPtr(expr : smart_ptr<ExprConstPtr>) : void {
        //! Prints a constant pointer value or null.
        if (expr.value != null) {
            write(*writer, "{expr.value}");
        } else {
            write(*writer, "null");
        }
    }
// const int 8
    def override preVisitExprConstInt8(expr : smart_ptr<ExprConstInt8>) : void {
        //! Prints an int8 constant value.
        write(*writer, "{expr.value}");
    }
// const int 16
    def override preVisitExprConstInt16(expr : smart_ptr<ExprConstInt16>) : void {
        //! Prints an int16 constant value.
        write(*writer, "{expr.value}");
    }
// const int 64
    def override preVisitExprConstInt64(expr : smart_ptr<ExprConstInt64>) : void {
        //! Prints an int64 constant value.
        write(*writer, "{expr.value}");
    }
// const int
    def override preVisitExprConstInt(expr : smart_ptr<ExprConstInt>) : void {
        //! Prints an int constant value.
        write(*writer, "{expr.value}");
    }
// const int2
    def override preVisitExprConstInt2(expr : smart_ptr<ExprConstInt2>) : void {
        //! Prints an int2 vector constant value.
        write(*writer, "int2({expr.value})");
    }
// const int3
    def override preVisitExprConstInt3(expr : smart_ptr<ExprConstInt3>) : void {
        //! Prints an int3 vector constant value.
        write(*writer, "int3({expr.value})");
    }
// const int4
    def override preVisitExprConstInt4(expr : smart_ptr<ExprConstInt4>) : void {
        //! Prints an int4 vector constant value.
        write(*writer, "int4({expr.value})");
    }
// const uint 8
    def override preVisitExprConstUInt8(expr : smart_ptr<ExprConstUInt8>) : void {
        //! Prints a uint8 constant value.
        write(*writer, "{expr.value}");
    }
// const uint 16
    def override preVisitExprConstUInt16(expr : smart_ptr<ExprConstUInt16>) : void {
        //! Prints a uint16 constant value.
        write(*writer, "{expr.value}");
    }
// const uint 64
    def override preVisitExprConstUInt64(expr : smart_ptr<ExprConstUInt64>) : void {
        //! Prints a uint64 constant value.
        write(*writer, "{expr.value}");
    }
// const uint
    def override preVisitExprConstUInt(expr : smart_ptr<ExprConstUInt>) : void {
        //! Prints a uint constant value.
        write(*writer, "{expr.value}");
    }
// const uint2
    def override preVisitExprConstUInt2(expr : smart_ptr<ExprConstUInt2>) : void {
        //! Prints a uint2 vector constant value.
        write(*writer, "uint2({expr.value})");
    }
// const uint3
    def override preVisitExprConstUInt3(expr : smart_ptr<ExprConstUInt3>) : void {
        //! Prints a uint3 vector constant value.
        write(*writer, "uint3({expr.value})");
    }
// const uint4
    def override preVisitExprConstUInt4(expr : smart_ptr<ExprConstUInt4>) : void {
        //! Prints a uint4 vector constant value.
        write(*writer, "uint4({expr.value})");
    }
// const range
    def override preVisitExprConstRange(expr : smart_ptr<ExprConstRange>) : void {
        //! Prints a range constant value.
        write(*writer, "range({expr.value})");
    }
// const urnage
    def override preVisitExprConstURange(expr : smart_ptr<ExprConstURange>) : void {
        //! Prints a urange constant value.
        write(*writer, "urange({expr.value})");
    }
// const range
    def override preVisitExprConstRange64(expr : smart_ptr<ExprConstRange64>) : void {
        //! Prints a range64 constant value.
        write(*writer, "range64({expr.value})");
    }
// const urnage
    def override preVisitExprConstURange64(expr : smart_ptr<ExprConstURange64>) : void {
        //! Prints a urange64 constant value.
        write(*writer, "urange64({expr.value})");
    }
// const bool
    def override preVisitExprConstBool(expr : smart_ptr<ExprConstBool>) : void {
        //! Prints a boolean constant value.
        write(*writer, "{expr.value}");
    }
// const float
    def override preVisitExprConstFloat(expr : smart_ptr<ExprConstFloat>) : void {
        //! Prints a float constant value.
        write(*writer, "{expr.value}");
    }
// const float2
    def override preVisitExprConstFloat2(expr : smart_ptr<ExprConstFloat2>) : void {
        //! Prints a float2 vector constant value.
        write(*writer, "float2({expr.value})");
    }
// const float3
    def override preVisitExprConstFloat3(expr : smart_ptr<ExprConstFloat3>) : void {
        //! Prints a float3 vector constant value.
        write(*writer, "float3({expr.value})");
    }
// const float4
    def override preVisitExprConstFloat4(expr : smart_ptr<ExprConstFloat4>) : void {
        //! Prints a float4 vector constant value.
        write(*writer, "float4({expr.value})");
    }
// const double
    def override preVisitExprConstDouble(expr : smart_ptr<ExprConstDouble>) : void {
        //! Prints a double constant value with lf suffix.
        write(*writer, "{expr.value}lf");
    }
// fake context
    def override preVisitExprFakeContext(expr : smart_ptr<ExprFakeContext>) : void {
        //! Prints the __context__ intrinsic.
        write(*writer, "__context__");
    }
// fake line info
    def override preVisitExprFakeLineInfo(expr : smart_ptr<ExprFakeLineInfo>) : void {
        //! Prints the __lineinfo__ intrinsic.
        write(*writer, "__lineinfo__");
    }
// string
    def override preVisitExprConstString(expr : smart_ptr<ExprConstString>) : void {
        //! Prints a string constant with proper escaping.
        peek(expr.value) $(str) {
            write(*writer, "\"{escape(str)}\"");
        }
    }
// const enumeraiton
    def override preVisitExprConstEnumeration(expr : smart_ptr<ExprConstEnumeration>) : void {
        //! Prints a constant enumeration value with module prefix.
        let enumModule = expr.enumType._module;
        if (enumModule != null && !empty(enumModule.name)) {
            write(*writer, "{enumModule.name}::");
        }
        write(*writer, "{expr.enumType.name}.{expr.value}");
    }
// const bitfield
    def override preVisitExprConstBitfield(expr : smart_ptr<ExprConstBitfield>) : void {
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

[sideeffects, export]
def allExpr(arg : int) {
    //! Exercises all expression types for AST pretty-printer coverage.
    // ExprStringBuilder
    var sb = "arg = {arg}\n";
    sb = "";
    // ExprNamedCall
    add([a=1, b=2]);
    add(3, 4, [a=1, b=2]);
    // ExprNew
    var pFoo1 = new Foo;
    var pFoo2 = new Foo(1);
    var pFoo3 = new Foo(1, 2);
    // ExprCall
    add(1, 2);
    // ExprNullCoalescing
    var exprAt <- pFoo1 ?? Foo(a = 1);
    exprAt <- Foo(a = 2);
    // ExprAt
    pFoo1.b[0] = pFoo2;
    // ExprSafeAt
    var exprSafeAt = pFoo1.b?[0] ?? pFoo2;
    exprSafeAt = pFoo2;
    // ExprIs
    if (exprAt is type<int>) {
        print("nada\n");
    }
    // ExprOp2
    var aa = 1;
    aa = 3;
    var bb = 2;
    bb = 4;
    var cc = aa + bb;
    // ExprOp3
    cc = aa == bb ? aa : bb;
    // ExprWith
    with (exprAt) {
        aa = 3;
    }
    // ExprWhile
    while (aa != 100) {
        aa++;
    }
    // ExprTryRecover
    try {
        aa = 2;
    } recover {
        aa = 3;
    }
    // ExprIfThenElse
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
    // ExprMakeVariant
    var mkv = IorF(i = 5);
    mkv = IorF(f = 1.0);
    // ExprMakeStruct
    var mks <- [Foo(a=5, b <- array<Foo?>(null)), Foo(a = 6)];
    mks[1] <- Foo(a = 5);
    // ExprMakeArray
    var mka = fixed_array<int>(1, 2, 3, 4);
    mka = fixed_array<int>(5, 6, 7, 8);
    // ExprMakeTuple
    var mkt =  (1, 2.0, "three");
    mkt =  (2, 3.0, "four");
    // ExprArrayCompehension
    var acomp <- [for (x in range(3)); x * x; where x != 1];
    var gcomp <- [iterator for(x in range(3)); x * x; where x != 1];
    for (ta, tg in acomp, gcomp) {
        verify(ta == tg);
    }
    // ExprTypeInfo
    let tinfo = typeinfo rtti_typeinfo(type<int>).basicType;
    print("{tinfo}\n");
    // ExprPtr2Ref
    print("{*pFoo1}");
    // ExprLabel, ExprGoto
    goto label 1;
    print("nop");
    label 1:;
    // ExprRef2Ptr
    unsafe {
        let aacomp = addr(acomp);
        print("{aacomp}");
    }
    // ExprAddr
    let eaddr = @@add;
    print("{eaddr}");
    // ExprAssert, ExprStaticAssert
    assert(aa == 4, "assert here");
    verify(aa == 4, "verify here");
    static_assert(true, "static assert here");
    concept_assert(true, "concept assert here");
    // ExprDebug
    debug(aa);
    // ExprInvoke
    let atadd = @@add;
    invoke(atadd, 1, 2);
    // ExprErase
    var tab <- { 1 => "one", 2 => "two" };
    erase(tab, 1);
    // ExprFind
    get(tab, 2) $(argX) {
        print("{argX}");
    }
    // ExprKeyExists
    assert(key_exists(tab, 2));
    // ExprAscend
    let easc = new fixed_array(1, 2, 3, 4);
    print("{easc}");
    // ExprCast
    unsafe {
        let aaf = reinterpret<float> aa;
        print("{aaf}");
    }
    // ExprDelete
    unsafe {
        delete pFoo1;
    }
    // ExprSwizzle
    var fsw = float4(1, 2, 3, 4);
    fsw = fsw.yyzx;
    // ExprOp1
    var eop1 = 1;
    eop1 ++;
    -- eop1;
    // ExprMemZero
    memzero(eop1);
    // ExprConstString
    var cstr = "hello\nworld\n";
    cstr += "\{..\}";
    // ExprConstEnumeration
    var enc = Type.none;
    enc = Type.tInt;
    // ExprConstBitfield
    var bfc = ExprPrintFlags.topLevel;
    bfc = ExprPrintFlags.argLevel;
}

[export]
def test {
    //! Self-test: visits this program's AST and prints it.
    var astVisitor = new PrintVisitor();
    var astVisitorAdapter <- make_visitor(*astVisitor);
    visit(this_program(), astVisitorAdapter);

/*
    // ast_typedecl
    add_extra = 14
    let td <- typeinfo(ast_typedecl type<Foo>)
    print("td = {describe(td)}\n")
    // ast_expression
    var x = 1
    var y = 2
    x = 3
    y = 4
    let ed <- quote(x+y)
    print("ed = {describe(ed)}\n")
    // ast_function
    let fd <- typeinfo(ast_function @@add)
    print("fd = {describe(fd)}\n")
*/

    return true;
}

[export]
def printAst(var prog : ProgramPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints the entire program AST to the given writer.
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(prog, adapter)
}

[export]
def printExpr(var expr : ExpressionPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints a single expression AST node to the given writer.
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(expr, adapter)
}

[export]
def printFunc(var func : FunctionPtr; var writer : StringBuilderWriter?) {
    //! Pretty-prints a single function AST node to the given writer.
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(func, adapter)
}

[export]
def setFlags(var prog : ProgramPtr) {
    //! Visits the program AST to set printer flags before pretty-printing.
    var flags = new SetPrinterFlags();
    var inscope adapter <- make_visitor(*flags)
    visit(prog, adapter)
}
