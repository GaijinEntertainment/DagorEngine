options gen2

module ast_print

options rtti = true
// options log_infer_passes = true
options strict_smart_pointers = false
// options log = true

require rtti
require ast
require daslib/ast_boost
require daslib/strings_boost


require printer_flags_visitor

//////////
// example

// TODO: fixme
def noBracket(expr) {
    return false;
}


class PrintVisitor : AstVisitor {
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
        write(*writer, "\n");
    }
    def ident(tabs : int = -1) {
        if (tabs == -1) {
            return repeat("    ", tab)
        } else {
            return repeat("    ", tabs)
        }
    }

// program
    def override preVisitProgram(prog : ProgramPtr) {
        write(*writer, "// program\n");
    }
    def override visitProgram(prog : ProgramPtr) {
        write(*writer, "// end program\n");
    }
    def override preVisitProgramBody(prog : ProgramPtr; mod : Module?) {
        write(*writer, "// program body\n");
    }
// type
    ET : bool = false;
    def override preVisitTypeDecl(typ : TypeDeclPtr) {
        ET = typ.isExprType;
        if (ET) {
            write(*writer, "/*[");
        }
    }
    def override visitTypeDecl(typ : TypeDeclPtr) {
        if (ET) {
            write(*writer, "]*/");
        }
        return typ;
    }
// alias
    def override preVisitAlias(typ : TypeDeclPtr; name : das_string) {
        write(*writer, "typedef {name} = {describe([decl=typ,extra=extraTypeInfo])}\n\n");
    }
// enumeration
    def override preVisitEnumeration(enu : EnumerationPtr) {
        write(*writer, "enum {enu.name} : {get_das_type_name(enu.baseType)} \{\n");
    }
    def override preVisitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*writer, "{ident(1)}{name} = ");
    }
    def override visitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*writer, "\n");
        return value;
    }
    def override visitEnumeration(enu : EnumerationPtr) {
        write(*writer, "\}\n\n");
        return enu;
    }
// structure
    def override preVisitStructure(str : StructurePtr) {
        if (str.annotations |> length != 0) {
            write(*writer, "[{describe(str.annotations)}]\n");
        }
        write(*writer, "{str.flags.isClass ? "class" : "struct"} {str.name} \{\n");
    }
    def override preVisitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) {
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
        write(*writer, "\n");
    }
    def override visitStructure(var str : StructurePtr) : StructurePtr {
        write(*writer, "\}\n\n");
        return <- str;
    }
// function
    def override preVisitFunction(fun : FunctionPtr) {
        if (fun.flags.knownSideEffects) {
            if (fun.sideEffectFlags == bitfield(0)) {
                write(*writer, "// [nosideeffects]\n");
            } else {
                // // todo: describe_bitfield not supported in aot yet
                // write(*writer, "// {describe_bitfield(fun.sideEffectFlags)}\n");
            }
        }
        let bfa : FunctionFlags = fun.flags & function_annotation_flags;
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
        if (fun.arguments |> length != 0) {
            write(*writer, " ) ");
        }
        if (fun.result != null && !fun.result.isVoid) {
            write(*writer, " : {describe([decl=fun.result,extra=extraTypeInfo])}");
        }
        write(*writer, " ");
    }
    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        write(*writer, "\n\n");
        return <- fun;
    }
    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; last : bool) {
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
        if (!last) {
            write(*writer, "; ");
        }
        return arg;
    }
    def override preVisitFunctionArgumentInit(fun : FunctionPtr; arg : VariablePtr; value : ExpressionPtr) {
        write(*writer, " = ");
    }
// block
    def override preVisitExprBlock(blk : smart_ptr<ExprBlock>) {
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
        tab --;
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident()}\}");
        }
        return <- blk;
    }
    def override preVisitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        write(*writer, "{ident()}");
    }
    def override visitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        self->newLine();
        return expr;
    }
    def override preVisitExprBlockFinal(blk : smart_ptr<ExprBlock>) {
        if (printCStyle || blk.blockFlags.isClosure) {
            write(*writer, "{ident(tab-1)}\} finally \{\n");
        } else {
            write(*writer, "{ident(tab-1)}finally\n");
        }
    }
    def override preVisitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        write(*writer, "{ident()}");
    }
    def override visitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        self->newLine();
        return expr;
    }
// let
    def override preVisitExprLet(expr : smart_ptr<ExprLet>) {
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
        if (arg.isAccessUnused) {
            write(*writer, " /*unused*/ ");
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
    }
    def override visitExprLetVariable(expr : smart_ptr<ExprLet>; arg : VariablePtr; lastArg : bool) {
        if (!lastArg) {
            write(*writer, ";");
        }
        return arg;
    }
    def override preVisitExprLetVariableInit(blk : smart_ptr<ExprLet>; arg : VariablePtr; expr : ExpressionPtr) {
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
        write(*writer, "{arg._type.flags.constant ? "let" : "var"}{arg.flags.global_shared ? " shared" : ""}\n{ident(1)}");
        if (arg.isAccessUnused) {
            write(*writer, " /*unused*/ ");
        }
        write(*writer, "{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}");
    }
    def override visitGlobalLetVariable(arg : VariablePtr; lastArg : bool) {
        write(*writer, "\n\n");
        return arg;
    }
    def override preVisitGlobalLetVariableInit(arg : VariablePtr; expr : ExpressionPtr) {
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
        write(*writer, "string_builder(");
    }
    def override visitExprStringBuilder(var expr : smart_ptr<ExprStringBuilder>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprStringBuilderElement(expr : smart_ptr<ExprStringBuilder>; elem : ExpressionPtr; last : bool) {
        if (!last) {
            write(*writer, ",");
        }
        return elem;
    }
// new
    def override preVisitExprNew(expr : smart_ptr<ExprNew>) {
        write(*writer, "new {describe(expr.typeexpr)}");
        if (expr.initializer) {
            write(*writer, "(");
        }
    }
    def override visitExprNew(var expr : smart_ptr<ExprNew>) : ExpressionPtr {
        if (expr.initializer) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override visitExprNewArgument(expr : smart_ptr<ExprNew>; arg : ExpressionPtr; last : bool) {
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// named call
    def override preVisitExprNamedCall(expr : smart_ptr<ExprNamedCall>) {
        write(*writer, "{expr.name}([");
    }
    def override visitExprNamedCall(var expr : smart_ptr<ExprNamedCall>) : ExpressionPtr {
        write(*writer, "])");
        return <- expr;
    }
    def override preVisitExprNamedCallArgument(expr : smart_ptr<ExprNamedCall>; arg : MakeFieldDeclPtr; last : bool) {
        write(*writer, "{arg.name}{arg.flags.moveSemantics ? "<-" : "="}");
    }
    def override visitExprNamedCallArgument(expr : smart_ptr<ExprNamedCall>; arg : MakeFieldDeclPtr; last : bool) {
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// looks like call (debug,assert,verify,erase,find,key_exists,keys,values,invoke,memzero etc)
    def override preVisitExprLooksLikeCall(expr : smart_ptr<ExprLooksLikeCall>) {
        write(*writer, "{expr.name}(");
    }
    def override visitExprLooksLikeCall(var expr : smart_ptr<ExprLooksLikeCall>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprLooksLikeCallArgument(expr : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) {
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// call
    def override preVisitExprCall(expr : smart_ptr<ExprCall>) {
        write(*writer, "{expr.name}(");
    }
    def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprCallArgument(expr : smart_ptr<ExprCall>; arg : ExpressionPtr; last : bool) {
        if (!last) {
            write(*writer, ",");
        }
        return arg;
    }
// null coaelescing
    def override preVisitExprNullCoalescingDefault(expr : smart_ptr<ExprNullCoalescing>; defval : ExpressionPtr) {
        write(*writer, " ?? ");
    }
// at
    def override visitExprAt(var expr : smart_ptr<ExprAt>) : ExpressionPtr {
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprAtIndex(expr : smart_ptr<ExprAt>; index : ExpressionPtr) {
        write(*writer, "[");
    }
// safe at
    def override visitExprSafeAt(var expr : smart_ptr<ExprSafeAt>) : ExpressionPtr {
        write(*writer, "]");
        return <- expr;
    }
    def override preVisitExprSafeAtIndex(expr : smart_ptr<ExprSafeAt>; index : ExpressionPtr) {
        write(*writer, "?[");
    }
// is
    def override preVisitExprIsType(expr : smart_ptr<ExprIs>; typeDecl : TypeDeclPtr) {
        write(*writer, "is type<{describe([decl=typeDecl,extra=false,contracts=true])}>");
    }
// op2
    def override preVisitExprOp2(expr : smart_ptr<ExprOp2>) {
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp2(var expr : smart_ptr<ExprOp2>) : ExpressionPtr {
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp2Right(expr : smart_ptr<ExprOp2>; right : ExpressionPtr) {
        write(*writer, " {expr.op} ");
    }
// op3
    def override preVisitExprOp3(expr : smart_ptr<ExprOp3>) {
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp3(var expr : smart_ptr<ExprOp3>) : ExpressionPtr {
        if (!noBracket(expr)) {
            write(*writer, ")");
        }
        return <- expr;
    }
    def override preVisitExprOp3Left(expr : smart_ptr<ExprOp3>; left : ExpressionPtr) {
        write(*writer, " ? ");
    }
    def override preVisitExprOp3Right(expr : smart_ptr<ExprOp3>; right : ExpressionPtr) {
        write(*writer, " : ");
    }
// copy
    def override preVisitExprCopyRight(expr : smart_ptr<ExprCopy>; right : ExpressionPtr) {
        write(*writer, " = ");
    }
// move
    def override preVisitExprMoveRight(expr : smart_ptr<ExprMove>; right : ExpressionPtr) {
        write(*writer, " <- ");
    }
// clone
    def override preVisitExprCloneRight(expr : smart_ptr<ExprClone>; right : ExpressionPtr) {
        write(*writer, " := ");
    }
// with
    def override preVisitExprWith(expr : smart_ptr<ExprWith>) {
        write(*writer, "with ");
    }
    def override preVisitExprWithBody(expr : smart_ptr<ExprWith>; right : ExpressionPtr) {
        write(*writer, " ");
    }
// while
    def override preVisitExprWhile(expr : smart_ptr<ExprWhile>) {
        write(*writer, "while ");
    }
    def override preVisitExprWhileBody(expr : smart_ptr<ExprWhile>; right : ExpressionPtr) {
        write(*writer, " ");
    }
// try-catch
    def override preVisitExprTryCatch(expr : smart_ptr<ExprTryCatch>) {
        write(*writer, "try ");
    }
    def override preVisitExprTryCatchCatch(expr : smart_ptr<ExprTryCatch>; right : ExpressionPtr) {
        write(*writer, " recover ");
    }
// if-then-else
    def override preVisitExprIfThenElse(expr : smart_ptr<ExprIfThenElse>) {
        write(*writer, "if (");
    }
    def override preVisitExprIfThenElseIfBlock(expr : smart_ptr<ExprIfThenElse>; ifBlock : ExpressionPtr) {
        write(*writer, ") ");
    }
    def override preVisitExprIfThenElseElseBlock(expr : smart_ptr<ExprIfThenElse>; elseBlock : ExpressionPtr) {
        if (elseBlock.__rtti == "ExprIfThenElse") {
            write(*writer, " else ");
        } else {
            write(*writer, " else ");
        }
    }
// for
    def override preVisitExprFor(expr : smart_ptr<ExprFor>) {
        write(*writer, "for (");
    }
    def override preVisitExprForVariable(expr : smart_ptr<ExprFor>; svar : VariablePtr; last : bool) {
        write(*writer, "{svar.name}{last ? " in " : ","}");
    }
    def override visitExprForSource(var expr : smart_ptr<ExprFor>; var source : ExpressionPtr; last : bool) : ExpressionPtr {
        if (!last) {
            write(*writer, ",");
        }
        return <- source;
    }
    def override preVisitExprForBody(expr : smart_ptr<ExprFor>) {
        write(*writer, ") ");
    }
// make variant
    def override preVisitExprMakeVariant(expr : smart_ptr<ExprMakeVariant>) {
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}");
        }
        write(*writer, "(");
    }
    def override visitExprMakeVariant(var expr : smart_ptr<ExprMakeVariant>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override preVisitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        if (!last) {
            write(*writer, "; ");
        }
        return decl;
    }
// make structure
    def override preVisitExprMakeStruct(expr : smart_ptr<ExprMakeStruct>) {
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)}({expr.makeStructFlags.useInitializer ? "" : "uninitialized"} ");
        } else {
            write(*writer, "(")
        }
    }
    def override visitExprMakeStruct(var expr : smart_ptr<ExprMakeStruct>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeStructIndex(expr : smart_ptr<ExprMakeStruct>; index : int; last : bool) {
        if (!last) {
            write(*writer, "; ");
        }
    }
    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        write(*writer, "{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ");
    }
    def override visitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        if (!last) {
            write(*writer, ", ");
        }
        return decl;
    }
// make array
    def override preVisitExprMakeArray(expr : smart_ptr<ExprMakeArray>) {
        write(*writer, "fixed_array");
        if (expr.recordType != null) {
            write(*writer, "<{describe(expr.recordType)}>");
        }
        write(*writer, "(");
    }
    def override visitExprMakeArray(var expr : smart_ptr<ExprMakeArray>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
// make tuple
    def override preVisitExprMakeTuple(expr : smart_ptr<ExprMakeTuple>) {
        if (expr._type != null) {
            write(*writer, "{describe(expr._type)} ");
        }
        write(*writer, "(");
    }
    def override visitExprMakeTuple(var expr : smart_ptr<ExprMakeTuple>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
    def override visitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; var init : ExpressionPtr; last : bool) : ExpressionPtr {
        if (!last) {
            write(*writer, ", ");
        }
        return <- init;
    }
// array comprehension
    def override preVisitExprArrayComprehension(expr : smart_ptr<ExprArrayComprehension>) {
        if (expr.tableSyntax) {
            write(*writer, "\{");
        } else {
            write(*writer, "[");
        }
    }
    def override visitExprArrayComprehension(var expr : smart_ptr<ExprArrayComprehension>) : ExpressionPtr {
        if (expr.tableSyntax) {
            write(*writer, "\}");
        } else {
            write(*writer, "]");
        }
        return <- expr;
    }
    def override preVisitExprArrayComprehensionSubexpr(expr : smart_ptr<ExprArrayComprehension>; subexrp : ExpressionPtr) {
        write(*writer, "; ");
    }
    def override preVisitExprArrayComprehensionWhere(expr : smart_ptr<ExprArrayComprehension>; filter : ExpressionPtr) {
        write(*writer, "; where ");
    }

    def override preVisitExprTypeDecl(var expr : smart_ptr<ExprTypeDecl>) {
        write(*writer, "type<{describe(expr.typeexpr)}>")
    }

// type info
    def override preVisitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>) {
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
        write(*writer, ")");
        return <- expr;
    }
// ptr to ref
    def override preVisitExprPtr2Ref(expr : smart_ptr<ExprPtr2Ref>) {
        write(*writer, "deref(");
    }
    def override visitExprPtr2Ref(var expr : smart_ptr<ExprPtr2Ref>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
// label
    def override preVisitExprLabel(expr : smart_ptr<ExprLabel>) {
        write(*writer, "label {expr.labelName}:");
        if (!empty(expr.comment)) {
            write(*writer, "/*{expr.comment}*/");
        }
    }
// goto
    def override preVisitExprGoto(expr : smart_ptr<ExprGoto>) {
        write(*writer, "goto ");
        if (expr.subexpr == null) {
            write(*writer, "label {expr.labelName}");
        }
    }
// ref to value
    def override preVisitExprRef2Value(expr : smart_ptr<ExprRef2Value>) {
        write(*writer, "/*r2v*/ (");
    }
    def override visitExprRef2Value(var expr : smart_ptr<ExprRef2Value>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
// ref 2 ptr
    def override preVisitExprRef2Ptr(expr : smart_ptr<ExprRef2Ptr>) {
        write(*writer, "addr(");
    }
    def override visitExprRef2Ptr(var expr : smart_ptr<ExprRef2Ptr>) : ExpressionPtr {
        write(*writer, ")");
        return <- expr;
    }
// @@
    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) {
        write(*writer, "@@");
        if (expr.funcType != null) {
            write(*writer, "<{describe(expr.funcType)}>");
        }
        write(*writer, "{expr.target}");
    }
// ascend
    def override preVisitExprAscend(expr : smart_ptr<ExprAscend>) {
        if (expr.ascType != null) {
            write(*writer, "new<{describe(expr.ascType)}> ");
        } else {
            write(*writer, "new ");
        }
    }
// cast
    def override preVisitExprCast(expr : smart_ptr<ExprCast>) {
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
        write(*writer, "delete ");
        if (expr.native) {
            write(*writer, "/*native*/ ");
        }
    }
// var
    def override preVisitExprVar(expr : smart_ptr<ExprVar>) {
        write(*writer, "{expr.name}");
    }
// field
    def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr {
        write(*writer, ".{expr.name}");
        return <- expr;
    }
// safe field
    def override visitExprSafeField(var expr : smart_ptr<ExprSafeField>) : ExpressionPtr {
        write(*writer, "?.{expr.name}");
        return <- expr;
    }
// swizzle
    def override visitExprSwizzle(var expr : smart_ptr<ExprSwizzle>) : ExpressionPtr {
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
        write(*writer, " is {expr.name}");
        return <- expr;
    }
// as variant
    def override visitExprAsVariant(var expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        write(*writer, " as {expr.name}");
        return <- expr;
    }
// safe as variant
    def override visitExprSafeAsVariant(var expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        write(*writer, " ?as {expr.name}");
        return <- expr;
    }
// op1
    def override preVisitExprOp1(expr : smart_ptr<ExprOp1>) : void {
        let op = "{expr.op}";
        if (op != "+++" && op != "---") {
            write(*writer, op);
        }
        if (!noBracket(expr)) {
            write(*writer, "(");
        }
    }
    def override visitExprOp1(var expr : smart_ptr<ExprOp1>) : ExpressionPtr {
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
        write(*writer, "yield ");
        if (expr.returnFlags.moveSemantics) {
            write(*writer, "<- ");
        }
    }
// break
    def override preVisitExprBreak(expr : smart_ptr<ExprBreak>) : void {
        write(*writer, "break");
    }
// continue
    def override preVisitExprContinue(expr : smart_ptr<ExprContinue>) : void {
        write(*writer, "continue");
    }
// const ptr
    def override preVisitExprConstPtr(expr : smart_ptr<ExprConstPtr>) : void {
        if (expr.value != null) {
            write(*writer, "{expr.value}");
        } else {
            write(*writer, "null");
        }
    }
// const int 8
    def override preVisitExprConstInt8(expr : smart_ptr<ExprConstInt8>) : void {
        write(*writer, "{expr.value}");
    }
// const int 16
    def override preVisitExprConstInt16(expr : smart_ptr<ExprConstInt16>) : void {
        write(*writer, "{expr.value}");
    }
// const int 64
    def override preVisitExprConstInt64(expr : smart_ptr<ExprConstInt64>) : void {
        write(*writer, "{expr.value}");
    }
// const int
    def override preVisitExprConstInt(expr : smart_ptr<ExprConstInt>) : void {
        write(*writer, "{expr.value}");
    }
// const int2
    def override preVisitExprConstInt2(expr : smart_ptr<ExprConstInt2>) : void {
        write(*writer, "int2({expr.value})");
    }
// const int3
    def override preVisitExprConstInt3(expr : smart_ptr<ExprConstInt3>) : void {
        write(*writer, "int3({expr.value})");
    }
// const int4
    def override preVisitExprConstInt4(expr : smart_ptr<ExprConstInt4>) : void {
        write(*writer, "int4({expr.value})");
    }
// const uint 8
    def override preVisitExprConstUInt8(expr : smart_ptr<ExprConstUInt8>) : void {
        write(*writer, "{expr.value}");
    }
// const uint 16
    def override preVisitExprConstUInt16(expr : smart_ptr<ExprConstUInt16>) : void {
        write(*writer, "{expr.value}");
    }
// const uint 64
    def override preVisitExprConstUInt64(expr : smart_ptr<ExprConstUInt64>) : void {
        write(*writer, "{expr.value}");
    }
// const uint
    def override preVisitExprConstUInt(expr : smart_ptr<ExprConstUInt>) : void {
        write(*writer, "{expr.value}");
    }
// const uint2
    def override preVisitExprConstUInt2(expr : smart_ptr<ExprConstUInt2>) : void {
        write(*writer, "uint2({expr.value})");
    }
// const uint3
    def override preVisitExprConstUInt3(expr : smart_ptr<ExprConstUInt3>) : void {
        write(*writer, "uint3({expr.value})");
    }
// const uint4
    def override preVisitExprConstUInt4(expr : smart_ptr<ExprConstUInt4>) : void {
        write(*writer, "uint4({expr.value})");
    }
// const range
    def override preVisitExprConstRange(expr : smart_ptr<ExprConstRange>) : void {
        write(*writer, "range({expr.value})");
    }
// const urnage
    def override preVisitExprConstURange(expr : smart_ptr<ExprConstURange>) : void {
        write(*writer, "urange({expr.value})");
    }
// const range
    def override preVisitExprConstRange64(expr : smart_ptr<ExprConstRange64>) : void {
        write(*writer, "range64({expr.value})");
    }
// const urnage
    def override preVisitExprConstURange64(expr : smart_ptr<ExprConstURange64>) : void {
        write(*writer, "urange64({expr.value})");
    }
// const bool
    def override preVisitExprConstBool(expr : smart_ptr<ExprConstBool>) : void {
        write(*writer, "{expr.value}");
    }
// const float
    def override preVisitExprConstFloat(expr : smart_ptr<ExprConstFloat>) : void {
        write(*writer, "{expr.value}");
    }
// const float2
    def override preVisitExprConstFloat2(expr : smart_ptr<ExprConstFloat2>) : void {
        write(*writer, "float2({expr.value})");
    }
// const float3
    def override preVisitExprConstFloat3(expr : smart_ptr<ExprConstFloat3>) : void {
        write(*writer, "float3({expr.value})");
    }
// const float4
    def override preVisitExprConstFloat4(expr : smart_ptr<ExprConstFloat4>) : void {
        write(*writer, "float4({expr.value})");
    }
// const double
    def override preVisitExprConstDouble(expr : smart_ptr<ExprConstDouble>) : void {
        write(*writer, "{expr.value}lf");
    }
// fake context
    def override preVisitExprFakeContext(expr : smart_ptr<ExprFakeContext>) : void {
        write(*writer, "__context__");
    }
// fake line info
    def override preVisitExprFakeLineInfo(expr : smart_ptr<ExprFakeLineInfo>) : void {
        write(*writer, "__lineinfo__");
    }
// string
    def override preVisitExprConstString(expr : smart_ptr<ExprConstString>) : void {
        peek(expr.value) <| $(str) {
            write(*writer, "\"{escape(str)}\"");
        }
    }
// const enumeraiton
    def override preVisitExprConstEnumeration(expr : smart_ptr<ExprConstEnumeration>) : void {
        let enumModule = expr.enumType._module;
        if (enumModule != null && !empty(enumModule.name)) {
            write(*writer, "{enumModule.name}::");
        }
        write(*writer, "{expr.enumType.name}.{expr.value}");
    }
// const bitfield
    def override preVisitExprConstBitfield(expr : smart_ptr<ExprConstBitfield>) : void {
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

typedef Integer = int
typedef IorF = variant<i : int; f : float>


enum FooBar {
    Foo
    Bar = 13
}


[comment(x=5, msg="haha")]
struct Foo {
    @do_not_delete@temp = 13 a : int = 11;
    b : array<Foo?>;
}

def Foo(x : int) {
    return <- Foo(a = x);
}

def Foo(x, y : int) {
    return <- Foo(a = x + y);
}

[sideeffects]
def add(a, b : int) {
    // todo make global, after globals supported in standalone ctx
    var add_extra = 13;
    print("a={a} b={b}");
    return a + b + add_extra;
}

[sideeffects, export]
def allExpr(arg : int) {
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
    var tinfo = typeinfo rtti_typeinfo(type<int>).basicType;
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
    get(tab, 2) <| $(argX) {
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
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(prog, adapter)
}

[export]
def printExpr(var expr : ExpressionPtr; var writer : StringBuilderWriter?) {
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(expr, adapter)
}

[export]
def printFunc(var func : FunctionPtr; var writer : StringBuilderWriter?) {
    var flags = new PrintVisitor(writer = writer);
    var inscope adapter <- make_visitor(*flags)
    visit(func, adapter)
}

[export]
def setFlags(var prog : ProgramPtr) {
    var flags = new SetPrinterFlags();
    var inscope adapter <- make_visitor(*flags)
    visit(prog, adapter)
}
