options rtti = true
// options log_infer_passes = true

// options log = true

require ast
require rtti
require daslib/ast_boost
require daslib/strings_boost

//////////
// example

let
    function_annotation_flags = (
        (FunctionFlags fastCall) |
        (FunctionFlags exports) |
        (FunctionFlags privateFunction) |
        (FunctionFlags unsafeDeref) |
        (FunctionFlags unsafeOperation) |
        (FunctionFlags _generator) )

// TODO: fixme
def noBracket(expr)
    return false

class PrintVisitor : AstVisitor
    extraTypeInfo : bool = true
    printCStyle : bool = true
    tab : int = 0
    // TODO: fixme
    def newLine()
        print("\n")
// program
    def override preVisitProgram(prog:ProgramPtr)
        print("// program\n")
    def override visitProgram(prog:ProgramPtr)
        print("// end program\n")
    def override preVisitProgramBody(prog:ProgramPtr;mod:Module?)
        print("// program body\n")
// type
    ET : bool = false
    def override preVisitTypeDecl(typ:TypeDeclPtr)
        ET = typ.isExprType
        if ET
            print("/*[")
    def override visitTypeDecl(typ:TypeDeclPtr)
        if ET
            print("]*/")
        return typ
// alias
    def override preVisitAlias(typ:TypeDeclPtr;name:das_string)
        print("typedef\n\t{name} = {describe([decl=typ,extra=extraTypeInfo])}\n\n")
// enumeration
    def override preVisitEnumeration(enu:EnumerationPtr)
        print("enum {enu.name} : {das_to_string(enu.baseType)}\n")
    def override preVisitEnumerationValue(enu:EnumerationPtr;name:das_string;value:ExpressionPtr;last:bool)
        print("\t{name} =")
    def override visitEnumerationValue(enu:EnumerationPtr;name:das_string;value:ExpressionPtr;last:bool)
        print("\n")
        return value
    def override visitEnumeration(enu:EnumerationPtr)
        print("\n")
        return enu
// structure
    def override preVisitStructure(str:StructurePtr)
        if str.annotations |> length != 0
            print("[{describe(str.annotations)}]\n")
        print("{str.flags.isClass ? "class" : "struct"} {str.name}\n")
    def override preVisitStructureField(str:StructurePtr;decl:FieldDeclaration;last:bool)
        print("\t")
        if decl.annotation |> length != 0
            print("[{describe(decl.annotation)}] ")
        print("{decl.name} : {describe([decl=decl._type,extra=extraTypeInfo])}")
        if decl.flags.parentType
            print(" /* from {str.parent.name} */")
        if decl.init != null
            print("{decl.flags.moveSemantics ? " <- " : " = "}")
    def override visitStructureField(str:StructurePtr;decl:FieldDeclaration;last:bool)
        print("\n")
    def override visitStructure(str:StructurePtr) : StructurePtr
        print("\n")
        return str
// function
    def override preVisitFunction(fun:FunctionPtr)
        if fun.flags.knownSideEffects
            if fun.sideEffectFlags == bitfield(0)
                print("// [nosideeffects]\n")
            else
                print("// {describe_bitfield(fun.sideEffectFlags)}\n")
        let bfa : FunctionFlags = fun.flags & function_annotation_flags
        print("{describe_bitfield(bfa,"\n")}")
        if fun.annotations |> length != 0
            print("[{describe(fun.annotations)}]\n")
        print("def {fun.name}")
        if fun.arguments |> length != 0
            print(" ( ")
    def override preVisitFunctionBody(fun:FunctionPtr; expr:ExpressionPtr)
        if fun.arguments |> length != 0
            print(" ) ")
        if fun.result != null && !fun.result.isVoid
            print(" : {describe([decl=fun.result,extra=extraTypeInfo])}")
        print("\n")
    def override visitFunction(fun:FunctionPtr) : FunctionPtr
        print("\n")
        return fun
    def override preVisitFunctionArgument(fun:FunctionPtr;arg:VariablePtr;last:bool)
        if arg.annotation |> length != 0
            print("[{describe(arg.annotation)}] ")
        if !arg._type.flags.constant
            print("var ")
        if arg.isAccessUnused
            print(" /*unsued*/ ")
        print("{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}")
    def override visitFunctionArgument(fun:FunctionPtr;arg:VariablePtr;last:bool)
        if !last
            print("; ")
        return arg
    def override preVisitFunctionArgumentInit(fun:FunctionPtr;arg:VariablePtr;value:ExpressionPtr)
        print(" = ")
// block
    def override preVisitExprBlock(blk:smart_ptr<ExprBlock>)
        if blk.blockFlags.isClosure
            if blk.returnType!=null || blk.arguments |> length!=0
                print("$(")
                for arg,argIndex in blk.arguments,range(blk.arguments |> length)
                    if arg.annotation |> length != 0
                        print("[{describe(arg.annotation)}] ")
                    if arg._type.flags.constant
                        print("var ")
                    print("{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}")
                    if argIndex != blk.arguments |> length-1
                        print("; ")
                print(")")
                if blk.returnType!=null
                    print(":{describe([decl=blk.returnType,extra=extraTypeInfo])}")
                print("\n")
        if printCStyle || blk.blockFlags.isClosure
            print("{repeat("\t",tab)}\{\n")
        tab ++
    def override visitExprBlock(blk:smart_ptr<ExprBlock>) : ExpressionPtr
        tab --
        if printCStyle || blk.blockFlags.isClosure
            print("{repeat("\t",tab)}\}\n")
        return blk
    def override preVisitExprBlockExpression(blk:smart_ptr<ExprBlock>;expr:ExpressionPtr)
        print("{repeat("\t",tab)}")
    def override visitExprBlockExpression(blk:smart_ptr<ExprBlock>;expr:ExpressionPtr)
        if printCStyle || blk.blockFlags.isClosure
            print(";")
        self->newLine()
        return expr
    def override visitExprBlockFinal(blk:smart_ptr<ExprBlock>)
        if printCStyle || blk.blockFlags.isClosure
            print("{repeat("\t",tab-1)}\} finally \{\n")
        else
            print("{repeat("\t",tab-1)}finally\n")
    def override preVisitExprBlockFinalExpression(blk:smart_ptr<ExprBlock>;expr:ExpressionPtr)
        print("{repeat("\t",tab)}")
    def override visitExprBlockFinalExpression(blk:smart_ptr<ExprBlock>;expr:ExpressionPtr)
        if printCStyle || blk.blockFlags.isClosure
            print(";")
        self->newLine()
        return expr
// let
    def override preVisitExprLet(expr:smart_ptr<ExprLet>)
        var isLet = true
        for pv in expr.variables
            if pv._type!=null && pv._type.flags.constant
                isLet = false
                break
        print("{isLet ? "let" : "var"} ")
    def override preVisitExprLetVariable(expr:smart_ptr<ExprLet>;arg:VariablePtr;lastArg:bool)
        if arg.isAccessUnused
            print(" /*unused*/ ")
        print("{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}")
    def override visitExprLetVariable(expr:smart_ptr<ExprLet>;arg:VariablePtr;lastArg:bool)
        if !lastArg
            print(";")
        return arg
    def override preVisitExprLetVariableInit(blk:smart_ptr<ExprLet>;arg:VariablePtr;expr:ExpressionPtr)
        if arg.flags.init_via_move
            print(" <- ")
        elif arg.flags.init_via_clone
            print(" := ")
        else
            print(" = ")
// global let
    def override preVisitGlobalLetVariable(arg:VariablePtr;lastArg:bool)
        print("{arg._type.flags.constant ? "let" : "var"}{arg.flags.global_shared ? " shared" : ""}\n\t")
        if arg.isAccessUnused
            print(" /*unused*/ ")
        print("{arg.name} : {describe([decl=arg._type,extra=extraTypeInfo])}")
    def override visitGlobalLetVariable(arg:VariablePtr;lastArg:bool)
        print("\n\n")
        return arg
    def override preVisitGlobalLetVariableInit(arg:VariablePtr;expr:ExpressionPtr)
        if arg.flags.init_via_move
            print(" <- ")
        elif arg.flags.init_via_clone
            print(" := ")
        else
            print(" = ")
// string builder
    def override preVisitExprStringBuilder(expr:smart_ptr<ExprStringBuilder>)
        print("string_builder(")
    def override visitExprStringBuilder(expr:smart_ptr<ExprStringBuilder>) : ExpressionPtr
        print(")")
        return expr
    def override visitExprStringBuilderElement(expr:smart_ptr<ExprStringBuilder>;elem:ExpressionPtr;last:bool)
        if !last
            print(",")
        return elem
// new
    def override preVisitExprNew(expr:smart_ptr<ExprNew>)
        print("new {describe(expr.typeexpr)}")
        if expr.initializer
            print("(")
    def override visitExprNew(expr:smart_ptr<ExprNew>) : ExpressionPtr
        if expr.initializer
            print(")")
        return expr
    def override visitExprNewArgument(expr:smart_ptr<ExprNew>;arg:ExpressionPtr;last:bool)
        if !last
            print(",")
        return arg
// named call
    def override preVisitExprNamedCall(expr:smart_ptr<ExprNamedCall>)
        print("{expr.name}([")
    def override visitExprNamedCall(expr:smart_ptr<ExprNamedCall>) : ExpressionPtr
        print("])")
        return expr
    def override preVisitExprNamedCallArgument(expr:smart_ptr<ExprNamedCall>;arg:MakeFieldDeclPtr;last:bool)
        print("{arg.name}{arg.flags.moveSemantics ? "<-" : "="}")
    def override visitExprNamedCallArgument(expr:smart_ptr<ExprNamedCall>;arg:MakeFieldDeclPtr;last:bool)
        if !last
            print(",")
        return arg
// looks like call (debug,assert,verify,erase,find,key_exists,keys,values,invoke,memzero etc)
    def override preVisitExprLooksLikeCall(expr:smart_ptr<ExprLooksLikeCall>)
        print("{expr.name}(")
    def override visitExprLooksLikeCall(expr:smart_ptr<ExprLooksLikeCall>) : ExpressionPtr
        print(")")
        return expr
    def override visitExprLooksLikeCallArgument(expr:smart_ptr<ExprLooksLikeCall>;arg:ExpressionPtr;last:bool)
        if !last
            print(",")
        return arg
// call
    def override preVisitExprCall(expr:smart_ptr<ExprCall>)
        print("{expr.name}(")
    def override visitExprCall(expr:smart_ptr<ExprCall>) : ExpressionPtr
        print(")")
        return expr
    def override visitExprCallArgument(expr:smart_ptr<ExprCall>;arg:ExpressionPtr;last:bool)
        if !last
            print(",")
        return arg
// null coaelescing
    def override preVisitExprNullCoalescingDefault(expr:smart_ptr<ExprNullCoalescing>;defval:ExpressionPtr)
        print(" ?? ")
// at
    def override visitExprAt(expr:smart_ptr<ExprAt>) : ExpressionPtr
        print("]")
        return expr
    def override preVisitExprAtIndex(expr:smart_ptr<ExprAt>;index:ExpressionPtr)
        print("[")
// safe at
    def override visitExprSafeAt(expr:smart_ptr<ExprSafeAt>) : ExpressionPtr
        print("]")
        return expr
    def override preVisitExprSafeAtIndex(expr:smart_ptr<ExprAt>;index:ExpressionPtr)
        print("?[")
// is
    def override preVisitExprIsType(expr:smart_ptr<ExprAt>;typeDecl:TypeDeclPtr)
        print("is type<{describe([decl=typeDecl,extra=false,contracts=true])}>")
// op2
    def override preVisitExprOp2(expr:smart_ptr<ExprOp2>)
        if !noBracket(expr)
            print("(")
    def override visitExprOp2(expr:smart_ptr<ExprOp2>) : ExpressionPtr
        if !noBracket(expr)
            print(")")
        return expr
    def override preVisitExprOp2Right(expr:smart_ptr<ExprOp2>;right:ExpressionPtr)
        print(" {expr.op} ")
// op3
    def override preVisitExprOp3(expr:smart_ptr<ExprOp3>)
        if !noBracket(expr)
            print("(")
    def override visitExprOp3(expr:smart_ptr<ExprOp3>) : ExpressionPtr
        if !noBracket(expr)
            print(")")
        return expr
    def override preVisitExprOp3Left(expr:smart_ptr<ExprOp3>;left:ExpressionPtr)
        print(" ? ")
    def override preVisitExprOp3Right(expr:smart_ptr<ExprOp3>;right:ExpressionPtr)
        print(" : ")
// copy
    def override preVisitExprCopyRight(expr:smart_ptr<ExprCopy>;right:ExpressionPtr)
        print(" = ")
// move
    def override preVisitExprMoveRight(expr:smart_ptr<ExprMove>;right:ExpressionPtr)
        print(" <- ")
// clone
    def override preVisitExprCloneRight(expr:smart_ptr<ExprClone>;right:ExpressionPtr)
        print(" := ")
// with
    def override preVisitExprWith(expr:smart_ptr<ExprWith>)
        print("with ")
    def override preVisitExprWithBody(expr:smart_ptr<ExprWith>;right:ExpressionPtr)
        print("\n")
// while
    def override preVisitExprWhile(expr:smart_ptr<ExprWhile>)
        print("while ")
    def override preVisitExprWhileBody(expr:smart_ptr<ExprWhile>;right:ExpressionPtr)
        print("\n")
// try-catch
    def override preVisitExprTryCatch(expr:smart_ptr<ExprTryCatch>)
        print("try\n")
    def override preVisitExprTryCatchCatch(expr:smart_ptr<ExprTryCatch>;right:ExpressionPtr)
        print("{repeat("\t",tab)}recover\n")
// if-then-else
    def override preVisitExprIfThenElse(expr:smart_ptr<ExprIfThenElse>)
        print("if ")
    def override preVisitExprIfThenElseIfBlock(expr:smart_ptr<ExprIfThenElse>;ifBlock:ExpressionPtr)
        print("\n")
    def override preVisitExprIfThenElseElseBlock(expr:smart_ptr<ExprIfThenElse>;elseBlock:ExpressionPtr)
        print("{repeat("\t",tab)}")
        if elseBlock.__rtti=="ExprIfThenElse"
            print("else ")
        else
            print("else\n")
// for
    def override preVisitExprFor(expr:smart_ptr<ExprFor>)
        print("for ")
    def override preVisitExprForVariable(expr:smart_ptr<ExprFor>;svar:VariablePtr;last:bool)
        print("{svar.name}{last ? " in " : ","}")
    def override visitExprForSource(expr:smart_ptr<ExprFor>;source:ExpressionPtr;last:bool) : ExpressionPtr
        if !last
            print(",")
        return source
    def override preVisitExprForBody(expr:smart_ptr<ExprFor>)
        print("\n")
// make variant
    def override preVisitExprMakeVariant(expr:smart_ptr<ExprMakeVariant>)
        print("[[")
        if expr._type != null
            print("{describe(expr._type)} ")
    def override visitExprMakeVariant(expr:smart_ptr<ExprMakeVariant>) : ExpressionPtr
        print("]]")
        return expr
    def override preVisitExprMakeVariantField(expr:smart_ptr<ExprMakeVariant>;index:int;decl:MakeFieldDeclPtr;last:bool)
        print("{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ")
    def override visitExprMakeVariantField(expr:smart_ptr<ExprMakeVariant>;index:int;decl:MakeFieldDeclPtr;last:bool)
        if !last
            print("; ")
        return decl
// make structure
    def override preVisitExprMakeStruct(expr:smart_ptr<ExprMakeStruct>)
        print("[[")
        if expr._type != null
            print("{describe(expr._type)}{expr.makeStructFlags.useInitializer ? "()" : ""} ")
    def override visitExprMakeStruct(expr:smart_ptr<ExprMakeStruct>) : ExpressionPtr
        print("]]")
        return expr
    def override visitExprMakeStructIndex(expr:smart_ptr<ExprMakeStruct>;index:int;last:bool)
        if !last
            print("; ")
    def override preVisitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;decl:MakeFieldDeclPtr;last:bool)
        print("{decl.name} {decl.flags.moveSemantics ? "<-" : "=" } ")
    def override visitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;decl:MakeFieldDeclPtr;last:bool)
        if !last
            print(", ")
        return decl
// make array
    def override preVisitExprMakeArray(expr:smart_ptr<ExprMakeArray>)
        print("[[")
        if expr._type != null
            print("{describe(expr._type)} ")
    def override visitExprMakeArray(expr:smart_ptr<ExprMakeArray>) : ExpressionPtr
        print("]]")
        return expr
    def override visitExprMakeArrayIndex(expr:smart_ptr<ExprMakeArray>;index:int;init:ExpressionPtr;last:bool) : ExpressionPtr
        if !last
            print("; ")
        return init
// make tuple
    def override preVisitExprMakeTuple(expr:smart_ptr<ExprMakeTuple>)
        print("[[")
        if expr._type != null
            print("{describe(expr._type)} ")
    def override visitExprMakeTuple(expr:smart_ptr<ExprMakeTuple>) : ExpressionPtr
        print("]]")
        return expr
    def override visitExprMakeTupleIndex(expr:smart_ptr<ExprMakeTuple>;index:int;init:ExpressionPtr;last:bool) : ExpressionPtr
        if !last
            print(", ")
        return init
// array comprehension
    def override preVisitExprArrayComprehension(expr:smart_ptr<ExprArrayComprehension>)
        print("{expr.generatorSyntax ? "[[" : "[\{"}")
    def override visitExprArrayComprehension(expr:smart_ptr<ExprArrayComprehension>) : ExpressionPtr
        print("{expr.generatorSyntax ? "]]" : "\}]"}")
        return expr
    def override preVisitExprArrayComprehensionSubexpr(expr:smart_ptr<ExprArrayComprehension>;subexrp:ExpressionPtr)
        print("; ")
    def override preVisitExprArrayComprehensionWhere(expr:smart_ptr<ExprArrayComprehension>;filter:ExpressionPtr)
        print("; where ")
// type info
    def override preVisitExprTypeInfo(expr:smart_ptr<ExprTypeInfo>)
        print("typeinfo({expr.trait}")
        if !empty(expr.subtrait)
            print("<{expr.subtrait}")
            if !empty(expr.extratrait)
                print(";{expr.extratrait}")
        print(">")
        if expr.subexpr == null
            print("type<{describe(expr.typeexpr)}>")
    def override visitExprTypeInfo(expr:smart_ptr<ExprTypeInfo>) : ExpressionPtr
        print(")")
        return expr
// ptr to ref
    def override preVisitExprPtr2Ref(expr:smart_ptr<ExprPtr2Ref>)
        print("deref(")
    def override visitExprPtr2Ref(expr:smart_ptr<ExprPtr2Ref>) : ExpressionPtr
        print(")")
        return expr
// label
    def override preVisitExprLabel(expr:smart_ptr<ExprLabel>)
        print("label {expr.labelName}:")
        if !empty(expr.comment)
            print("/*{expr.comment}*/")
// goto
    def override preVisitExprGoto(expr:smart_ptr<ExprGoto>)
        print("goto ")
        if expr.subexpr==null
            print("label {expr.labelName}")
// ref to value
    def override preVisitExprRef2Value(expr:smart_ptr<ExprRef2Value>)
        print("/*r2v*/ (")
    def override visitExprRef2Value(expr:smart_ptr<ExprRef2Value>) : ExpressionPtr
        print(")")
        return expr
// ref 2 ptr
    def override preVisitExprRef2Ptr(expr:smart_ptr<ExprRef2Ptr>)
        print("addr(")
    def override visitExprRef2Ptr(expr:smart_ptr<ExprRef2Ptr>) : ExpressionPtr
        print(")")
        return expr
// @@
    def override preVisitExprAddr(expr:smart_ptr<ExprAddr>)
        print("@@")
        if expr.funcType != null
            print("<{describe(expr.funcType)}>")
        print("{expr.target}")
// ascend
    def override preVisitExprAscend(expr:smart_ptr<ExprAscend>)
        if expr.ascType != null
            print("new<{describe(expr.ascType)}> ")
        else
            print("new ")
// cast
    def override preVisitExprCast(expr:smart_ptr<ExprCast>)
        if expr.castFlags.reinterpretCast
            print("reinterpret")
        elif expr.castFlags.upcastCast
            print("upcast")
        else
            print("cast")
        print("<{describe(expr.castType)}> ")
// delete
    def override preVisitExprDelete(expr:smart_ptr<ExprDelete>)
        print("delete ")
        if expr.native
            print("/*native*/ ")
// var
    def override preVisitExprVar(expr:smart_ptr<ExprVar>)
        print("{expr.name}")
// field
    def override visitExprField(expr:smart_ptr<ExprField>) : ExpressionPtr
        print(".{expr.name}")
        return expr
// safe field
    def override visitExprSafeField(expr:smart_ptr<ExprSafeField>) : ExpressionPtr
        print("?.{expr.name}")
        return expr
// swizzle
    def override visitExprSwizzle(expr:smart_ptr<ExprSwizzle>) : ExpressionPtr
        print(".")
        let f2name = [[string "x";"y";"z";"w"]]
        for fch in expr.fields
            let f = int(fch)
            print("{f>=0 && f<=3 ? f2name[f] : "?"}")
        return expr
// is variant
    def override visitExprIsVariant(expr:smart_ptr<ExprIsVariant>) : ExpressionPtr
        print(" is {expr.name}")
        return expr
// as variant
    def override visitExprAsVariant(expr:smart_ptr<ExprAsVariant>) : ExpressionPtr
        print(" as {expr.name}")
        return expr
// safe as variant
    def override visitExprSafeAsVariant(expr:smart_ptr<ExprSafeAsVariant>) : ExpressionPtr
        print(" ?as {expr.name}")
        return expr
// op1
    def override preVisitExprOp1(expr:smart_ptr<ExprOp1>) : void
        let op = "{expr.op}"
        if op!="+++" && op!="---"
            print(op)
        if !noBracket(expr)
            print("(")
    def override visitExprOp1(expr:smart_ptr<ExprOp1>) : ExpressionPtr
        let op = "{expr.op}"
        if !noBracket(expr)
            print(")")
        if op=="+++" || op=="---"
            print("{slice(op,0,-1)}")
        return expr
// return
    def override preVisitExprReturn(expr:smart_ptr<ExprReturn>) : void
        print("return ")
        if expr.returnFlags.fromYield
            print("/* from yield */ ")
        if expr.returnFlags.moveSemantics
            print("<- ")
// yield
    def override preVisitExprYield(expr:smart_ptr<ExprYield>) : void
        print("yield ")
        if expr.returnFlags.moveSemantics
            print("<- ")
// break
    def override preVisitExprBreak(expr:smart_ptr<ExprBreak>) : void
        print("break")
// continue
    def override preVisitExprContinue(expr:smart_ptr<ExprContinue>) : void
        print("continue")
// const ptr
    def override preVisitExprConstPtr(expr:smart_ptr<ExprConstPtr>) : void
        print("{expr.value}")
// const int 8
    def override preVisitExprConstInt8(expr:smart_ptr<ExprConstInt8>) : void
        print("{expr.value}")
// const int 16
    def override preVisitExprConstInt16(expr:smart_ptr<ExprConstInt16>) : void
        print("{expr.value}")
// const int 64
    def override preVisitExprConstInt64(expr:smart_ptr<ExprConstInt64>) : void
        print("{expr.value}")
// const int
    def override preVisitExprConstInt(expr:smart_ptr<ExprConstInt>) : void
        print("{expr.value}")
// const int2
    def override preVisitExprConstInt2(expr:smart_ptr<ExprConstInt2>) : void
        print("int2({expr.value})")
// const int3
    def override preVisitExprConstInt3(expr:smart_ptr<ExprConstInt3>) : void
        print("int3({expr.value})")
// const int4
    def override preVisitExprConstInt4(expr:smart_ptr<ExprConstInt4>) : void
        print("int4({expr.value})")
// const uint 8
    def override preVisitExprConstUInt8(expr:smart_ptr<ExprConstUInt8>) : void
        print("{expr.value}")
// const uint 16
    def override preVisitExprConstUInt16(expr:smart_ptr<ExprConstUInt16>) : void
        print("{expr.value}")
// const uint 64
    def override preVisitExprConstUInt64(expr:smart_ptr<ExprConstUInt64>) : void
        print("{expr.value}")
// const uint
    def override preVisitExprConstUInt(expr:smart_ptr<ExprConstUInt>) : void
        print("{expr.value}")
// const uint2
    def override preVisitExprConstUInt2(expr:smart_ptr<ExprConstUInt2>) : void
        print("uint2({expr.value})")
// const uint3
    def override preVisitExprConstUInt3(expr:smart_ptr<ExprConstUInt3>) : void
        print("uint3({expr.value})")
// const uint4
    def override preVisitExprConstUInt4(expr:smart_ptr<ExprConstUInt4>) : void
        print("uint4({expr.value})")
// const range
    def override preVisitExprConstRange(expr:smart_ptr<ExprConstRange>) : void
        print("range({expr.value})")
// const urnage
    def override preVisitExprConstURange(expr:smart_ptr<ExprConstURange>) : void
        print("urange({expr.value})")
// const range
    def override preVisitExprConstRange64(expr:smart_ptr<ExprConstRange64>) : void
        print("range64({expr.value})")
// const urnage
    def override preVisitExprConstURange64(expr:smart_ptr<ExprConstURange64>) : void
        print("urange64({expr.value})")
// const bool
    def override preVisitExprConstBool(expr:smart_ptr<ExprConstBool>) : void
        print("{expr.value}")
// const float
    def override preVisitExprConstFloat(expr:smart_ptr<ExprConstFloat>) : void
        print("{expr.value}")
// const float2
    def override preVisitExprConstFloat2(expr:smart_ptr<ExprConstFloat2>) : void
        print("float2({expr.value})")
// const float3
    def override preVisitExprConstFloat3(expr:smart_ptr<ExprConstFloat3>) : void
        print("float3({expr.value})")
// const float4
    def override preVisitExprConstFloat4(expr:smart_ptr<ExprConstFloat4>) : void
        print("float4({expr.value})")
// const double
    def override preVisitExprConstDouble(expr:smart_ptr<ExprConstDouble>) : void
        print("{expr.value}lf")
// fake context
    def override preVisitExprFakeContext(expr:smart_ptr<ExprFakeContext>) : void
        print("__context__")
// fake line info
    def override preVisitExprFakeLineInfo(expr:smart_ptr<ExprFakeLineInfo>) : void
        print("__lineinfo__")
// string
    def override preVisitExprConstString(expr:smart_ptr<ExprConstString>) : void
        peek(expr.value) <| $(str)
            print("\"{escape(str)}\"")
// const enumeraiton
    def override preVisitExprConstEnumeration(expr:smart_ptr<ExprConstEnumeration>) : void
        let enumModule = expr.enumType._module
        if enumModule!=null && !empty(enumModule.name)
            print("{enumModule.name}::")
        print("{expr.enumType.name} {expr.value}")
// const bitfield
    def override preVisitExprConstBitfield(expr:smart_ptr<ExprConstBitfield>) : void
        var name : string
        if expr.bitfieldType!=null && !empty(expr.bitfieldType.alias)
            name = find_bitfield_name(expr.bitfieldType,expr.value)
        if !empty(name)
            print("{expr.bitfieldType.alias} {name}")
        else
            print("bitfield({expr.value})")

typedef
    Integer = int
    IorF = variant<i:int;f:float>

enum FooBar
    Foo
    Bar = 13


[comment(x=5,msg="haha")]
struct Foo
    [[do_not_delete,temp=13]] a : int = 11
    b : array<Foo?>

def Foo(x:int)
    return <- [[Foo a=x]]

def Foo(x,y:int)
    return <- [[Foo a=x+y]]

var
    add_extra = 13

[sideeffects]
def add(a,b:int)
    print("a={a} b={b}")
    return a + b + add_extra

[sideeffects,export]
def allExpr(arg:int)
    // ExprStringBuilder
    var sb = "arg = {arg}\n"
    sb = ""
    // ExprNamedCall
    add([a=1,b=2])
    add(3,4,[a=1,b=2])
    // ExprNew
    var pFoo1 = new Foo
    var pFoo2 = new Foo(1)
    var pFoo3 = new Foo(1,2)
    // ExprCall
    add(1,2)
    // ExprNullCoalescing
    var exprAt <- pFoo1 ?? [[Foo a=1]]
    exprAt <- [[Foo a=2]]
    // ExprAt
    pFoo1.b[0] = pFoo2
    // ExprSafeAt
    var exprSafeAt = pFoo1.b?[0] ?? pFoo2
    exprSafeAt = pFoo2
    // ExprIs
    if exprAt is type<int>
        print("nada\n")
    // ExprOp2
    var aa = 1
    aa = 3
    var bb = 2
    bb = 4
    var cc = aa + bb
    // ExprOp3
    cc = aa==bb ? aa : bb
    // ExprWith
    with exprAt
        aa = 3
    // ExprWhile
    while aa!=100
        aa++
    // ExprTryRecover
    try
        aa = 2
    recover
        aa = 3
    // ExprIfThenElse
    if aa==2
        aa = 2
    if aa==2
        aa = 2
    else
        aa = 3
    if aa==2
        aa = 2
    elif aa==3
        aa = 3
    else
        aa = 4
    // ExprMakeVariant
    var mkv = [[IorF i=5]]
    mkv = [[IorF f=1.0]]
    // ExprMakeStruct
    var mks <- [[Foo() a=5, b<-[{Foo? [1] null}]; a=6]]
    mks[1] <- [[Foo a=5]]
    // ExprMakeArray
    var mka = [[int 1;2;3;4]]
    mka = [[int 5;6;7;8]]
    // ExprMakeTuple
    var mkt = [[auto 1,2.0,"three"]]
    mkt = [[auto 2,3.0,"four"]]
    // ExprArrayCompehension
    var acomp <- [{for x in range(3); x*x; where x!=1}]
    var gcomp <- [[for x in range(3); x*x; where x!=1]]
    for ta,tg in acomp,gcomp
        verify(ta==tg)
    // ExprTypeInfo
    var tinfo = typeinfo(rtti_typeinfo type<int>).basicType
    print("{tinfo}\n")
    // ExprPtr2Ref
    print("{*pFoo1}")
    // ExprLabel, ExprGoto
    goto label 1
    print("nop")
    label 1:
    // ExprRef2Ptr
    unsafe
        let aacomp = addr(acomp)
        print("{aacomp}")
    // ExprAddr
    let eaddr = @@add
    print("{eaddr}")
    // ExprAssert, ExprStaticAssert
    assert(aa==4,"assert here")
    verify(aa==4,"verify here")
    static_assert(true,"static assert here")
    concept_assert(true,"concept assert here")
    // ExprDebug
    debug(aa)
    // ExprInvoke
    let atadd = @@add
    invoke(atadd,1,2)
    // ExprErase
    var tab <- {{ 1 => "one"; 2 => "two" }}
    erase(tab,1)
    // ExprFind
    find(tab, 2) <| $(argX)
        print("{argX}")
    // ExprKeyExists
    assert(key_exists(tab,2))
    // ExprAscend
    let easc = new [[auto 1;2;3;4]]
    print("{easc}")
    // ExprCast
    unsafe
        let aaf = reinterpret<float> aa
        print("{aaf}")
    // ExprDelete
    unsafe
        delete pFoo1
    // ExprSwizzle
    var fsw = float4(1,2,3,4)
    fsw = fsw.yyzx
    // ExprOp1
    var eop1 = 1
    eop1 ++
    -- eop1
    // ExprMemZero
    memzero(eop1)
    // ExprConstString
    var cstr = "hello\nworld\n"
    cstr += "\{..\}"
    // ExprConstEnumeration
    var enc = Type none
    enc = Type tInt
    // ExprConstBitfield
    var bfc = ExprPrintFlags topLevel
    bfc = ExprPrintFlags argLevel

[export]
def test
    var astVisitor = new PrintVisitor()
    var astVisitorAdapter <- make_visitor(*astVisitor)
    visit(this_program(), astVisitorAdapter)

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

    return true
