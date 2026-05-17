options gen2
options indenting = 4
options no_aot = true
options no_global_variables = false
options remove_unused_symbols = false
options strict_smart_pointers

module ast shared public

require ast_core public
require daslib/rtti public
require strings

typedef ExpressionPtr = Expression?
typedef ProgramPtr = smart_ptr<Program>
typedef TypeDeclPtr = TypeDecl?
typedef VectorTypeDeclPtr = dasvector`ptr`TypeDecl
typedef EnumerationPtr = Enumeration?
typedef StructurePtr = Structure?
typedef FunctionPtr = Function?
typedef VariablePtr = Variable?
typedef MakeFieldDeclPtr = MakeFieldDecl?
typedef ExprMakeBlockPtr = ExprMakeBlock?
typedef FunctionAnnotationPtr = FunctionAnnotation?
typedef StructureAnnotationPtr = StructureAnnotation?
typedef EnumerationAnnotationPtr = EnumerationAnnotation?
typedef PassMacroPtr = PassMacro?
typedef VariantMacroPtr = VariantMacro?
typedef ReaderMacroPtr = ReaderMacro?
typedef CommentReaderPtr = CommentReader?
typedef CallMacroPtr = CallMacro?
typedef TypeInfoMacroPtr = TypeInfoMacro?
typedef ForLoopMacroPtr = ForLoopMacro?
typedef CaptureMacroPtr = CaptureMacro?
typedef TypeMacroPtr = TypeMacro?
typedef SimulateMacroPtr = SimulateMacro?

// proxy wrappers for ast_core functions
[generic]
def for_each_function(mod : Module?; name : string; blk : block<(var func : FunctionPtr) : void>) {
    ast_core::for_each_function(mod, name, blk)
}

[generic]
def for_each_structure(mod : Module?; blk : block<(var st : StructurePtr) : void>) {
    ast_core::for_each_structure(mod, blk)
}

[generic]
def for_each_enumeration(mod : Module?; blk : block<(var en : EnumerationPtr) : void>) {
    ast_core::for_each_enumeration(mod, blk)
}

[generic]
def for_each_typedef(mod : Module?; blk : block<(name : string#; var value : TypeDeclPtr) : void>) {
    ast_core::for_each_typedef(mod, blk)
}

[generic]
def for_each_global(mod : Module?; blk : block<(var value : VariablePtr) : void>) {
    ast_core::for_each_global(mod, blk)
}


[generic]
def describe(decl : TypeDecl?; extra, contracts, modules : bool = true) {
    return describe_typedecl(decl, extra, contracts, modules)
}

[generic]
def describe_cpp(decl : TypeDecl?; substitureRef, skipRef, skipConst : bool = false; redundantConst : bool = true; chooseSmartPtr : bool = true) {
    return describe_typedecl_cpp(decl, substitureRef, skipRef, skipConst, redundantConst, chooseSmartPtr)
}

[generic]
def describe(expr : Expression?) {
    return describe_expression(expr)
}

// add_ptr_ref for Expression? is registered as C++ binding in module_builtin_ast.cpp

[generic]
def describe(expr : FunctionPtr) {
    return describe_function(expr)
}

[generic]
def describe(prog : smart_ptr<Program>) {
    return describe_program(prog)
}


[generic]
def get_mangled_name(decl : ExprBlock?) {
    return get_mangled_name(unsafe(reinterpret<ExprBlock?> decl))
}

[macro_interface]
class AstFunctionAnnotation {
    def abstract transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr
    def abstract verifyCall(var call : ExprCallFunc?; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool
    def abstract apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    def abstract generic_apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    def abstract finish(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool
    def abstract patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool&) : bool
    def abstract fixup(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool
    def abstract lint(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool
    def abstract complete(var func : FunctionPtr; var ctx : smart_ptr<Context>) : void
    def abstract isCompatible(var func : FunctionPtr; var types : VectorTypeDeclPtr; decl : AnnotationDeclaration; var errors : das_string) : bool
    def abstract isSpecialized : bool
    def abstract appendToMangledName(func : FunctionPtr; decl : AnnotationDeclaration; var mangledName : das_string) : void
    //! Returns true if this annotation's ``apply`` should also run on generic
    //! function templates at parse time. Default is false. Annotations that
    //! perform purely structural rewrites (no type information needed) override
    //! this to ``true`` so the rewrite happens once on the template and every
    //! instantiation inherits it. See ``[class_method]`` for an example.
    def abstract isAppliedToGeneric : bool
}

[macro_interface]
class AstBlockAnnotation {
    def abstract apply(var blk : ExprBlock?; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    def abstract finish(var blk : ExprBlock?; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool
}

[macro_interface]
class AstStructureAnnotation {
    def abstract apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    def abstract finish(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    def abstract patch(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string; var astChanged : bool&) : bool
    def abstract complete(var st : StructurePtr; var ctx : smart_ptr<Context>) : void
    def abstract aotPrefix(var st : StructurePtr; args : AnnotationArgumentList; var writer : StringBuilderWriter) : void
    def abstract aotBody(var st : StructurePtr; args : AnnotationArgumentList; var writer : StringBuilderWriter) : void
    def abstract aotSuffix(var st : StructurePtr; args : AnnotationArgumentList; var writer : StringBuilderWriter) : void
}

[macro_interface]
class AstPassMacro {
    def abstract apply(prog : ProgramPtr; mod : Module?) : bool
}

[macro_interface]
class AstVariantMacro {
    def abstract visitExprIsVariant(prog : ProgramPtr; mod : Module?; expr : ExprIsVariant?) : ExpressionPtr
    def abstract visitExprAsVariant(prog : ProgramPtr; mod : Module?; expr : ExprAsVariant?) : ExpressionPtr
    def abstract visitExprSafeAsVariant(prog : ProgramPtr; mod : Module?; expr : ExprSafeAsVariant?) : ExpressionPtr
}

[macro_interface]
class AstForLoopMacro {
    def abstract visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr
}

[macro_interface]
class AstCaptureMacro {
    def abstract captureExpression(prog : Program?; mod : Module?; expr : ExpressionPtr; etype : TypeDeclPtr) : ExpressionPtr
    def abstract captureFunction(prog : Program?; mod : Module?; var lcs : Structure?; var fun : FunctionPtr) : void
    def abstract releaseFunction(prog : Program?; mod : Module?; var lcs : Structure?; var fun : FunctionPtr) : void
}

[macro_interface]
class AstTypeMacro {
    def abstract visit(prog : ProgramPtr; mod : Module?; td, passT : TypeDeclPtr) : TypeDeclPtr
}

[macro_interface]
class AstSimulateMacro {
    def abstract preSimulate(prog : Program?; ctx : Context?) : bool
    def abstract simulate(prog : Program?; ctx : Context?) : bool
}

[macro_interface]
class AstReaderMacro {
    def abstract accept(prog : ProgramPtr; mod : Module?; expr : ExprReader?; ch : int; info : LineInfo) : bool
    def abstract suffix(prog : ProgramPtr; mod : Module?; expr : ExprReader?; info : LineInfo; var outLine : int&; var outFile : FileInfo?&) : string
    def abstract visit(prog : ProgramPtr; mod : Module?; expr : ExprReader?) : ExpressionPtr
}

[macro_interface]
class AstCommentReader {
    def abstract open(prog : ProgramPtr; mod : Module?; cpp : bool; info : LineInfo) : void
    def abstract accept(prog : ProgramPtr; mod : Module?; ch : int; info : LineInfo) : void
    def abstract close(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeStructure(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterStructure(st : StructurePtr; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeStructureFields(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterStructureField(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterStructureFields(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeFunction(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterFunction(fn : FunctionPtr; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeGlobalVariables(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterGlobalVariable(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterGlobalVariables(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeVariant(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeVariantEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterVariantEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterVariantEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterVariant(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeTuple(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeTupleEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterTupleEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterTupleEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterTuple(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeBitfield(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeBitfieldEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterBitfieldEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterBitfieldEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterBitfield(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeEnumeration(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeEnumerationEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterEnumerationEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterEnumerationEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterEnumeration(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract beforeAlias(prog : ProgramPtr; mod : Module?; info : LineInfo) : void
    def abstract afterAlias(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void
}

[macro_interface]
class AstCallMacro {
    def abstract preVisit(prog : ProgramPtr; mod : Module?; expr : ExprCallMacro?) : void
    def abstract visit(prog : ProgramPtr; mod : Module?; expr : ExprCallMacro?) : ExpressionPtr
    def abstract canVisitArgument(expr : ExprCallMacro?; argIndex : int) : bool
    def abstract canFoldReturnResult(expr : ExprCallMacro?) : bool
}

[macro_interface]
class AstTypeInfoMacro {
    def abstract getAstChange(expr : ExprTypeInfo?; var errors : das_string) : ExpressionPtr
    def abstract getAstType(var lib : ModuleLibrary; expr : ExprTypeInfo?; var errors : das_string) : TypeDeclPtr
}

[macro_interface]
class AstEnumerationAnnotation {
    def abstract apply(var st : EnumerationPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
}

[macro_interface]
class AstVisitor {
// program
    def abstract preVisitProgram(prog : ProgramPtr) : void
    def abstract visitProgram(porg : ProgramPtr) : void
    def abstract preVisitProgramBody(prog : ProgramPtr; mod : Module?) : void
// module
    def abstract preVisitModule(mod : Module?) : void
    def abstract visitModule(mod : Module?) : void
// type decl
    def abstract preVisitExprTypeDecl(expr : ExprTypeDecl?) : void
    def abstract visitExprTypeDecl(expr : ExprTypeDecl?) : ExpressionPtr
// type
    def abstract preVisitTypeDecl(typ : TypeDeclPtr) : void
    def abstract visitTypeDecl(typ : TypeDeclPtr) : TypeDeclPtr
// type alias
    def abstract preVisitAlias(typ : TypeDeclPtr; name : das_string) : void
    def abstract visitAlias(typ : TypeDeclPtr; name : das_string) : TypeDeclPtr
// enumeration
    def abstract canVisitEnumeration(arg : Enumeration?) : bool
    def abstract preVisitEnumeration(enu : EnumerationPtr) : void
    def abstract preVisitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) : void
    def abstract visitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) : ExpressionPtr
    def abstract visitEnumeration(enu : EnumerationPtr) : EnumerationPtr
// structure
    def abstract canVisitStructure(arg : Structure?) : bool
    def abstract preVisitStructure(str : StructurePtr) : void
    def abstract preVisitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) : void
    def abstract canVisitStructureFieldInit(st : StructurePtr) : bool
    def abstract visitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) : void
    def abstract visitStructure(str : StructurePtr) : StructurePtr
// function
    def abstract canVisitFunction(fun : Function?) : bool
    def abstract canVisitFunctionArgumentInit(fun : Function?; arg : VariablePtr; value : ExpressionPtr) : bool
    def abstract preVisitFunction(fun : FunctionPtr) : void
    def abstract visitFunction(fun : FunctionPtr) : FunctionPtr
    def abstract preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; lastArg : bool) : void
    def abstract visitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; lastArg : bool) : VariablePtr
    def abstract preVisitFunctionArgumentInit(fun : FunctionPtr; arg : VariablePtr; value : ExpressionPtr) : void
    def abstract visitFunctionArgumentInit(fun : FunctionPtr; arg : VariablePtr; value : ExpressionPtr) : ExpressionPtr
    def abstract preVisitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) : void
    def abstract visitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) : ExpressionPtr
// any expression there is
    def abstract preVisitExpression(expr : ExpressionPtr) : void
    def abstract visitExpression(expr : ExpressionPtr) : ExpressionPtr
// block
    def abstract preVisitExprBlock(blk : ExprBlock?) : void
    def abstract visitExprBlock(blk : ExprBlock?) : ExpressionPtr
    def abstract preVisitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) : void
    def abstract visitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) : VariablePtr
    def abstract preVisitExprBlockArgumentInit(blk : ExprBlock?; arg : VariablePtr; expr : ExpressionPtr) : void
    def abstract visitExprBlockArgumentInit(blk : ExprBlock?; arg : VariablePtr; expr : ExpressionPtr) : ExpressionPtr
    def abstract preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) : void
    def abstract visitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) : ExpressionPtr
    def abstract preVisitExprBlockFinal(blk : ExprBlock?) : void
    def abstract visitExprBlockFinal(blk : ExprBlock?) : void
    def abstract preVisitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) : void
    def abstract visitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) : ExpressionPtr
// let
    def abstract preVisitExprLet(expr : ExprLet?) : void
    def abstract visitExprLet(expr : ExprLet?) : ExpressionPtr
    def abstract preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) : void
    def abstract visitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) : VariablePtr
    def abstract preVisitExprLetVariableInit(blk : ExprLet?; arg : VariablePtr; expr : ExpressionPtr) : void
    def abstract visitExprLetVariableInit(blk : ExprLet?; arg : VariablePtr; expr : ExpressionPtr) : ExpressionPtr
// global let
    def abstract canVisitGlobalVariable(arg : Variable?) : bool
    def abstract preVisitGlobalLet(prog : ProgramPtr) : void
    def abstract visitGlobalLet(prog : ProgramPtr) : void
    def abstract preVisitGlobalLetVariable(arg : VariablePtr; lastArg : bool) : void
    def abstract visitGlobalLetVariable(arg : VariablePtr; lastArg : bool) : VariablePtr
    def abstract preVisitGlobalLetVariableInit(arg : VariablePtr; expr : ExpressionPtr) : void
    def abstract visitGlobalLetVariableInit(arg : VariablePtr; expr : ExpressionPtr) : ExpressionPtr
// string builder
    def abstract preVisitExprStringBuilder(expr : ExprStringBuilder?) : void
    def abstract visitExprStringBuilder(expr : ExprStringBuilder?) : ExpressionPtr
    def abstract preVisitExprStringBuilderElement(expr : ExprStringBuilder?; elem : ExpressionPtr; last : bool) : void
    def abstract visitExprStringBuilderElement(expr : ExprStringBuilder?; elem : ExpressionPtr; last : bool) : ExpressionPtr
// new
    def abstract preVisitExprNew(expr : ExprNew?) : void
    def abstract visitExprNew(expr : ExprNew?) : ExpressionPtr
    def abstract preVisitExprNewArgument(expr : ExprNew?; arg : ExpressionPtr; last : bool) : void
    def abstract visitExprNewArgument(expr : ExprNew?; arg : ExpressionPtr; last : bool) : ExpressionPtr
// named call (this one only exists before or during the infer)
    def abstract preVisitExprNamedCall(expr : ExprNamedCall?) : void
    def abstract visitExprNamedCall(expr : ExprNamedCall?) : ExpressionPtr
    def abstract preVisitExprNamedCallArgument(expr : ExprNamedCall?; arg : MakeFieldDeclPtr; last : bool) : void
    def abstract visitExprNamedCallArgument(expr : ExprNamedCall?; arg : MakeFieldDeclPtr; last : bool) : MakeFieldDeclPtr
// looks like call (debug,assert,verify,erase,find,key_exists,keys,values,invoke,memzero etc)
    def abstract preVisitExprLooksLikeCall(expr : ExprLooksLikeCall?) : void
    def abstract visitExprLooksLikeCall(expr : ExprLooksLikeCall?) : ExpressionPtr
    def abstract canVisitExprLooksLikeCallArgument(expr : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) : bool
    def abstract preVisitExprLooksLikeCallArgument(expr : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) : void
    def abstract visitExprLooksLikeCallArgument(expr : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) : ExpressionPtr
// call
    def abstract canVisitCall(expr : ExprCall?) : bool
    def abstract canVisitNamedCall(expr : ExprNamedCall?) : bool
    def abstract canVisitLooksLikeCall(expr : ExprLooksLikeCall?) : bool
    def abstract preVisitExprCall(expr : ExprCall?) : void
    def abstract visitExprCall(expr : ExprCall?) : ExpressionPtr
    def abstract preVisitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; last : bool) : void
    def abstract visitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; last : bool) : ExpressionPtr
// null coaelescing
    def abstract preVisitExprNullCoalescing(expr : ExprNullCoalescing?) : void
    def abstract visitExprNullCoalescing(expr : ExprNullCoalescing?) : ExpressionPtr
    def abstract preVisitExprNullCoalescingDefault(expr : ExprNullCoalescing?; defval : ExpressionPtr) : void
// at
    def abstract preVisitExprAt(expr : ExprAt?) : void
    def abstract visitExprAt(expr : ExprAt?) : ExpressionPtr
    def abstract preVisitExprAtIndex(expr : ExprAt?; index : ExpressionPtr) : void
// safe at
    def abstract preVisitExprSafeAt(expr : ExprSafeAt?) : void
    def abstract visitExprSafeAt(expr : ExprSafeAt?) : ExpressionPtr
    def abstract preVisitExprSafeAtIndex(expr : ExprSafeAt?; index : ExpressionPtr) : void
// is
    def abstract preVisitExprIs(expr : ExprIs?) : void
    def abstract visitExprIs(expr : ExprIs?) : ExpressionPtr
    def abstract preVisitExprIsType(expr : ExprIs?; typeDecl : TypeDeclPtr) : void
// op2
    def abstract preVisitExprOp2(expr : ExprOp2?) : void
    def abstract visitExprOp2(expr : ExprOp2?) : ExpressionPtr
    def abstract preVisitExprOp2Right(expr : ExprOp2?; right : ExpressionPtr) : void
// op3
    def abstract preVisitExprOp3(expr : ExprOp3?) : void
    def abstract visitExprOp3(expr : ExprOp3?) : ExpressionPtr
    def abstract preVisitExprOp3Left(expr : ExprOp3?; left : ExpressionPtr) : void
    def abstract preVisitExprOp3Right(expr : ExprOp3?; right : ExpressionPtr) : void
// copy
    def abstract isRightFirstExprCopy(expr : ExprCopy?) : bool
    def abstract preVisitExprCopy(expr : ExprCopy?) : void
    def abstract visitExprCopy(expr : ExprCopy?) : ExpressionPtr
    def abstract preVisitExprCopyRight(expr : ExprCopy?; right : ExpressionPtr) : void
// move
    def abstract isRightFirstExprMove(expr : ExprMove?) : bool
    def abstract preVisitExprMove(expr : ExprMove?) : void
    def abstract visitExprMove(expr : ExprMove?) : ExpressionPtr
    def abstract preVisitExprMoveRight(expr : ExprMove?; right : ExpressionPtr) : void
// clone
    def abstract isRightFirstExprClone(expr : ExprClone?) : bool
    def abstract preVisitExprClone(expr : ExprClone?) : void
    def abstract visitExprClone(expr : ExprClone?) : ExpressionPtr
    def abstract preVisitExprCloneRight(expr : ExprClone?; right : ExpressionPtr) : void
// assume
    def abstract canVisitWithAliasSubexpression(expr : ExprAssume?) : bool
    def abstract preVisitExprAssume(expr : ExprAssume?) : void
    def abstract visitExprAssume(expr : ExprAssume?) : ExpressionPtr
// with
    def abstract preVisitExprWith(expr : ExprWith?) : void
    def abstract visitExprWith(expr : ExprWith?) : ExpressionPtr
    def abstract preVisitExprWithBody(expr : ExprWith?; right : ExpressionPtr) : void
// while
    def abstract preVisitExprWhile(expr : ExprWhile?) : void
    def abstract visitExprWhile(expr : ExprWhile?) : ExpressionPtr
    def abstract preVisitExprWhileBody(expr : ExprWhile?; right : ExpressionPtr) : void
// try-catch
    def abstract preVisitExprTryCatch(expr : ExprTryCatch?) : void
    def abstract visitExprTryCatch(expr : ExprTryCatch?) : ExpressionPtr
    def abstract preVisitExprTryCatchCatch(expr : ExprTryCatch?; right : ExpressionPtr) : void
// if-then-else
    def abstract preVisitExprIfThenElse(expr : ExprIfThenElse?) : void
    def abstract visitExprIfThenElse(expr : ExprIfThenElse?) : ExpressionPtr
    def abstract preVisitExprIfThenElseIfBlock(expr : ExprIfThenElse?; ifBlock : ExpressionPtr) : void
    def abstract preVisitExprIfThenElseElseBlock(expr : ExprIfThenElse?; elseBlock : ExpressionPtr) : void
// for
    def abstract preVisitExprFor(expr : ExprFor?) : void
    def abstract visitExprFor(expr : ExprFor?) : ExpressionPtr
    def abstract preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) : void
    def abstract visitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) : VariablePtr
    def abstract preVisitExprForSource(expr : ExprFor?; source : ExpressionPtr; last : bool) : void
    def abstract visitExprForSource(expr : ExprFor?; source : ExpressionPtr; last : bool) : ExpressionPtr
    def abstract preVisitExprForStack(expr : ExprFor?) : void
    def abstract preVisitExprForBody(expr : ExprFor?) : void
// make variant
    def abstract preVisitExprMakeVariant(expr : ExprMakeVariant?) : void
    def abstract visitExprMakeVariant(expr : ExprMakeVariant?) : ExpressionPtr
    def abstract preVisitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) : void
    def abstract visitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) : MakeFieldDeclPtr
// make structure
    def abstract canVisitExprMakeStructBody(expr : ExprMakeStruct?) : bool
    def abstract canVisitExprMakeStructBlock(expr : ExprMakeStruct?; blk : ExpressionPtr) : bool
    def abstract preVisitExprMakeStruct(expr : ExprMakeStruct?) : void
    def abstract visitExprMakeStruct(expr : ExprMakeStruct?) : ExpressionPtr
    def abstract preVisitStructureAlias(str : StructurePtr; name : das_string; at : TypeDeclPtr) : void
    def abstract visitStructureAlias(str : StructurePtr; name : das_string; at : TypeDeclPtr) : TypeDeclPtr
    def abstract preVisitExprMakeStructIndex(expr : ExprMakeStruct?; index : int; last : bool) : void
    def abstract visitExprMakeStructIndex(expr : ExprMakeStruct?; index : int; last : bool) : void
    def abstract preVisitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) : void
    def abstract visitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) : MakeFieldDeclPtr
    def abstract preVisitMakeStructureBlock(expr : ExprMakeStruct?; blk : Expression?) : void
    def abstract visitMakeStructureBlock(expr : ExprMakeStruct?; blk : Expression?) : ExpressionPtr
// make array
    def abstract preVisitExprMakeArray(expr : ExprMakeArray?) : void
    def abstract visitExprMakeArray(expr : ExprMakeArray?) : ExpressionPtr
    def abstract preVisitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) : void
    def abstract visitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) : ExpressionPtr
// make tuple
    def abstract preVisitExprMakeTuple(expr : ExprMakeTuple?) : void
    def abstract visitExprMakeTuple(expr : ExprMakeTuple?) : ExpressionPtr
    def abstract preVisitExprMakeTupleIndex(expr : ExprMakeTuple?; index : int; init : ExpressionPtr; last : bool) : void
    def abstract visitExprMakeTupleIndex(expr : ExprMakeTuple?; index : int; init : ExpressionPtr; last : bool) : ExpressionPtr
// array comprehension
    def abstract preVisitExprArrayComprehension(expr : ExprArrayComprehension?) : void
    def abstract visitExprArrayComprehension(expr : ExprArrayComprehension?) : ExpressionPtr
    def abstract preVisitExprArrayComprehensionSubexpr(expr : ExprArrayComprehension?; subexrp : ExpressionPtr) : void
    def abstract preVisitExprArrayComprehensionWhere(expr : ExprArrayComprehension?; filter : ExpressionPtr) : void
// type info
    def abstract canVisitExprTypeInfo(expr : ExprTypeInfo?; expr_ : ExpressionPtr) : bool
    def abstract preVisitExprTypeInfo(expr : ExprTypeInfo?) : void
    def abstract visitExprTypeInfo(expr : ExprTypeInfo?) : ExpressionPtr
// ptr to ref
    def abstract preVisitExprPtr2Ref(expr : ExprPtr2Ref?) : void
    def abstract visitExprPtr2Ref(expr : ExprPtr2Ref?) : ExpressionPtr
// label
    def abstract preVisitExprLabel(expr : ExprLabel?) : void
    def abstract visitExprLabel(expr : ExprLabel?) : ExpressionPtr
// goto
    def abstract preVisitExprGoto(expr : ExprGoto?) : void
    def abstract visitExprGoto(expr : ExprGoto?) : ExpressionPtr
// ref to value
    def abstract preVisitExprRef2Value(expr : ExprRef2Value?) : void
    def abstract visitExprRef2Value(expr : ExprRef2Value?) : ExpressionPtr
// ref to ptr
    def abstract preVisitExprRef2Ptr(expr : ExprRef2Ptr?) : void
    def abstract visitExprRef2Ptr(expr : ExprRef2Ptr?) : ExpressionPtr
// @@
    def abstract preVisitExprAddr(expr : ExprAddr?) : void
    def abstract visitExprAddr(expr : ExprAddr?) : ExpressionPtr
// assert / verify
    def abstract preVisitExprAssert(expr : ExprAssert?) : void
    def abstract visitExprAssert(expr : ExprAssert?) : ExpressionPtr
// static_assert
    def abstract preVisitExprStaticAssert(expr : ExprStaticAssert?) : void
    def abstract visitExprStaticAssert(expr : ExprStaticAssert?) : ExpressionPtr
// quote
    def abstract preVisitExprQuote(expr : ExprQuote?) : void
    def abstract visitExprQuote(expr : ExprQuote?) : ExpressionPtr
// debug
    def abstract preVisitExprDebug(expr : ExprDebug?) : void
    def abstract visitExprDebug(expr : ExprDebug?) : ExpressionPtr
// invoke
    def abstract preVisitExprInvoke(expr : ExprInvoke?) : void
    def abstract visitExprInvoke(expr : ExprInvoke?) : ExpressionPtr
// erase
    def abstract preVisitExprErase(expr : ExprErase?) : void
    def abstract visitExprErase(expr : ExprErase?) : ExpressionPtr
// insert
    def abstract preVisitExprSetInsert(expr : ExprSetInsert?) : void
    def abstract visitExprSetInsert(expr : ExprSetInsert?) : ExpressionPtr
// find
    def abstract preVisitExprFind(expr : ExprFind?) : void
    def abstract visitExprFind(expr : ExprFind?) : ExpressionPtr
// key exists
    def abstract preVisitExprKeyExists(expr : ExprKeyExists?) : void
    def abstract visitExprKeyExists(expr : ExprKeyExists?) : ExpressionPtr
// ascend
    def abstract preVisitExprAscend(expr : ExprAscend?) : void
    def abstract visitExprAscend(expr : ExprAscend?) : ExpressionPtr
// cast
    def abstract preVisitExprCast(expr : ExprCast?) : void
    def abstract visitExprCast(expr : ExprCast?) : ExpressionPtr
// delete
    def abstract preVisitExprDeleteSizeExpression(del : ExprDelete?; expr : ExpressionPtr) : void
    def abstract preVisitExprDelete(expr : ExprDelete?) : void
    def abstract visitExprDelete(expr : ExprDelete?) : ExpressionPtr
// var
    def abstract preVisitExprVar(expr : ExprVar?) : void
    def abstract visitExprVar(expr : ExprVar?) : ExpressionPtr
// tag
    def abstract preVisitExprTag(expr : ExprTag?) : void
    def abstract preVisitExprTagValue(expr : ExprTag?; value : ExpressionPtr) : void
    def abstract visitExprTag(expr : ExprTag?) : ExpressionPtr
// field
    def abstract preVisitExprField(expr : ExprField?) : void
    def abstract visitExprField(expr : ExprField?) : ExpressionPtr
// safe field
    def abstract preVisitExprSafeField(expr : ExprSafeField?) : void
    def abstract visitExprSafeField(expr : ExprSafeField?) : ExpressionPtr
// swizzle
    def abstract preVisitExprSwizzle(expr : ExprSwizzle?) : void
    def abstract visitExprSwizzle(expr : ExprSwizzle?) : ExpressionPtr
// is variant
    def abstract preVisitExprIsVariant(expr : ExprIsVariant?) : void
    def abstract visitExprIsVariant(expr : ExprIsVariant?) : ExpressionPtr
// as variant
    def abstract preVisitExprAsVariant(expr : ExprAsVariant?) : void
    def abstract visitExprAsVariant(expr : ExprAsVariant?) : ExpressionPtr
// safe as variant
    def abstract preVisitExprSafeAsVariant(expr : ExprSafeAsVariant?) : void
    def abstract visitExprSafeAsVariant(expr : ExprSafeAsVariant?) : ExpressionPtr
// op1
    def abstract preVisitExprOp1(expr : ExprOp1?) : void
    def abstract visitExprOp1(expr : ExprOp1?) : ExpressionPtr
// return
    def abstract preVisitExprReturn(expr : ExprReturn?) : void
    def abstract visitExprReturn(expr : ExprReturn?) : ExpressionPtr
// yield
    def abstract preVisitExprYield(expr : ExprYield?) : void
    def abstract visitExprYield(expr : ExprYield?) : ExpressionPtr
// break
    def abstract preVisitExprBreak(expr : ExprBreak?) : void
    def abstract visitExprBreak(expr : ExprBreak?) : ExpressionPtr
// continue
    def abstract preVisitExprContinue(expr : ExprContinue?) : void
    def abstract visitExprContinue(expr : ExprContinue?) : ExpressionPtr
// make block
    def abstract canVisitMakeBlockBody(expr : ExprMakeBlock?) : bool
    def abstract preVisitExprMakeBlock(expr : ExprMakeBlock?) : void
    def abstract visitExprMakeBlock(expr : ExprMakeBlock?) : ExpressionPtr
// make generator
    def abstract preVisitExprMakeGenerator(expr : ExprMakeGenerator?) : void
    def abstract visitExprMakeGenerator(expr : ExprMakeGenerator?) : ExpressionPtr
// memzero
    def abstract preVisitExprMemZero(expr : ExprMemZero?) : void
    def abstract visitExprMemZero(expr : ExprMemZero?) : ExpressionPtr
// const
    def abstract preVisitExprConst(expr : ExprConst?) : void
    def abstract visitExprConst(expr : ExprConst?) : ExpressionPtr
// const ptr
    def abstract preVisitExprConstPtr(expr : ExprConstPtr?) : void
    def abstract visitExprConstPtr(expr : ExprConstPtr?) : ExpressionPtr
// const enumeration
    def abstract preVisitExprConstEnumeration(expr : ExprConstEnumeration?) : void
    def abstract visitExprConstEnumeration(expr : ExprConstEnumeration?) : ExpressionPtr
// cont bitfield
    def abstract preVisitExprConstBitfield(expr : ExprConstBitfield?) : void
    def abstract visitExprConstBitfield(expr : ExprConstBitfield?) : ExpressionPtr
// const int 8
    def abstract preVisitExprConstInt8(expr : ExprConstInt8?) : void
    def abstract visitExprConstInt8(expr : ExprConstInt8?) : ExpressionPtr
// const int 16
    def abstract preVisitExprConstInt16(expr : ExprConstInt16?) : void
    def abstract visitExprConstInt16(expr : ExprConstInt16?) : ExpressionPtr
// const int 64
    def abstract preVisitExprConstInt64(expr : ExprConstInt64?) : void
    def abstract visitExprConstInt64(expr : ExprConstInt64?) : ExpressionPtr
// const int
    def abstract preVisitExprConstInt(expr : ExprConstInt?) : void
    def abstract visitExprConstInt(expr : ExprConstInt?) : ExpressionPtr
// const int2
    def abstract preVisitExprConstInt2(expr : ExprConstInt2?) : void
    def abstract visitExprConstInt2(expr : ExprConstInt2?) : ExpressionPtr
// const int3
    def abstract preVisitExprConstInt3(expr : ExprConstInt3?) : void
    def abstract visitExprConstInt3(expr : ExprConstInt3?) : ExpressionPtr
// const int4
    def abstract preVisitExprConstInt4(expr : ExprConstInt4?) : void
    def abstract visitExprConstInt4(expr : ExprConstInt4?) : ExpressionPtr
// const uint 8
    def abstract preVisitExprConstUInt8(expr : ExprConstUInt8?) : void
    def abstract visitExprConstUInt8(expr : ExprConstUInt8?) : ExpressionPtr
// const uint 16
    def abstract preVisitExprConstUInt16(expr : ExprConstUInt16?) : void
    def abstract visitExprConstUInt16(expr : ExprConstUInt16?) : ExpressionPtr
// const uint 64
    def abstract preVisitExprConstUInt64(expr : ExprConstUInt64?) : void
    def abstract visitExprConstUInt64(expr : ExprConstUInt64?) : ExpressionPtr
// const uint
    def abstract preVisitExprConstUInt(expr : ExprConstUInt?) : void
    def abstract visitExprConstUInt(expr : ExprConstUInt?) : ExpressionPtr
// const uint2
    def abstract preVisitExprConstUInt2(expr : ExprConstUInt2?) : void
    def abstract visitExprConstUInt2(expr : ExprConstUInt2?) : ExpressionPtr
// const uint3
    def abstract preVisitExprConstUInt3(expr : ExprConstUInt3?) : void
    def abstract visitExprConstUInt3(expr : ExprConstUInt3?) : ExpressionPtr
// const uint4
    def abstract preVisitExprConstUInt4(expr : ExprConstUInt4?) : void
    def abstract visitExprConstUInt4(expr : ExprConstUInt4?) : ExpressionPtr
// const range
    def abstract preVisitExprConstRange(expr : ExprConstRange?) : void
    def abstract visitExprConstRange(expr : ExprConstRange?) : ExpressionPtr
// const urnage
    def abstract preVisitExprConstURange(expr : ExprConstURange?) : void
    def abstract visitExprConstURange(expr : ExprConstURange?) : ExpressionPtr
// const range64
    def abstract preVisitExprConstRange64(expr : ExprConstRange64?) : void
    def abstract visitExprConstRange64(expr : ExprConstRange64?) : ExpressionPtr
// const urnage64
    def abstract preVisitExprConstURange64(expr : ExprConstURange64?) : void
    def abstract visitExprConstURange64(expr : ExprConstURange64?) : ExpressionPtr
// const bool
    def abstract preVisitExprConstBool(expr : ExprConstBool?) : void
    def abstract visitExprConstBool(expr : ExprConstBool?) : ExpressionPtr
// const float
    def abstract preVisitExprConstFloat(expr : ExprConstFloat?) : void
    def abstract visitExprConstFloat(expr : ExprConstFloat?) : ExpressionPtr
// const float2
    def abstract preVisitExprConstFloat2(expr : ExprConstFloat2?) : void
    def abstract visitExprConstFloat2(expr : ExprConstFloat2?) : ExpressionPtr
// const float3
    def abstract preVisitExprConstFloat3(expr : ExprConstFloat3?) : void
    def abstract visitExprConstFloat3(expr : ExprConstFloat3?) : ExpressionPtr
// const float4
    def abstract preVisitExprConstFloat4(expr : ExprConstFloat4?) : void
    def abstract visitExprConstFloat4(expr : ExprConstFloat4?) : ExpressionPtr
// const string
    def abstract preVisitExprConstString(expr : ExprConstString?) : void
    def abstract visitExprConstString(expr : ExprConstString?) : ExpressionPtr
// const double
    def abstract preVisitExprConstDouble(expr : ExprConstDouble?) : void
    def abstract visitExprConstDouble(expr : ExprConstDouble?) : ExpressionPtr
// fake context
    def abstract preVisitExprFakeContext(expr : ExprFakeContext?) : void
    def abstract visitExprFakeContext(expr : ExprFakeContext?) : ExpressionPtr
// fake line info
    def abstract preVisitExprFakeLineInfo(expr : ExprFakeLineInfo?) : void
    def abstract visitExprFakeLineInfo(expr : ExprFakeLineInfo?) : ExpressionPtr
// reader
    def abstract preVisitExprReader(expr : ExprReader?) : void
    def abstract visitExprReader(expr : ExprReader?) : ExpressionPtr
// unsafe
    def abstract preVisitExprUnsafe(expr : ExprUnsafe?) : void
    def abstract visitExprUnsafe(expr : ExprUnsafe?) : ExpressionPtr
// call macro
    def abstract preVisitExprCallMacro(expr : ExprCallMacro?) : void
    def abstract visitExprCallMacro(expr : ExprCallMacro?) : ExpressionPtr
}

var private gc_root_AstFunctionAnnotation : array<AstFunctionAnnotation?>

[macro_function]
def make_function_annotation(name : string; var someClassPtr) : FunctionAnnotationPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstFunctionAnnotation |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_function_annotation(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstBlockAnnotation : array<AstBlockAnnotation?>

[macro_function]
def make_block_annotation(name : string; var someClassPtr) : FunctionAnnotationPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstBlockAnnotation |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_block_annotation(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstStructureAnnotation : array<AstStructureAnnotation?>

[macro_function]
def make_structure_annotation(name : string; var someClassPtr) : StructureAnnotationPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstStructureAnnotation |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_structure_annotation(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstEnumerationAnnotation : array<AstEnumerationAnnotation?>

[macro_function]
def make_enumeration_annotation(name : string; var someClassPtr) : EnumerationAnnotationPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstEnumerationAnnotation |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_enumeration_annotation(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

def make_visitor(someClass; blk : block<(var adapter : VisitorAdapter?) : void>) {
    static_if (typeinfo is_class(someClass)) {
        unsafe {
            let classPtr = addr(someClass)
            let classInfo = class_info(someClass)
            make_visitor(classPtr, classInfo, blk)
        }
    } else {
        concept_assert(false, "can't make visitor of non-class")
    }
}

var private gc_root_AstReaderMacro : array<AstReaderMacro?>

[macro_function]
def make_reader_macro(name : string; var someClassPtr) : ReaderMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstReaderMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_reader_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstCommentReader : array<AstCommentReader?>

[macro_function]
def make_comment_reader(name : string; var someClassPtr) : CommentReaderPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstCommentReader |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_comment_reader(someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstCallMacro : array<AstCallMacro?>

[macro_function]
def make_call_macro(name : string; var someClassPtr) : CallMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstCallMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_call_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstTypeInfoMacro : array<AstTypeInfoMacro?>

[macro_function]
def make_typeinfo_macro(name : string; var someClassPtr) : TypeInfoMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstTypeInfoMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_typeinfo_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstPassMacro : array<AstPassMacro?>

[macro_function]
def make_pass_macro(name : string; var someClassPtr) : PassMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstPassMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_pass_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstVariantMacro : array<AstVariantMacro?>

[macro_function]
def make_variant_macro(name : string; var someClassPtr) : VariantMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstVariantMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_variant_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstForLoopMacro : array<AstForLoopMacro?>

[macro_function]
def make_for_loop_macro(name : string; var someClassPtr) : ForLoopMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstForLoopMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_for_loop_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstCaptureMacro : array<AstCaptureMacro?>

[macro_function]
def make_capture_macro(name : string; var someClassPtr) : CaptureMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstCaptureMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_capture_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

[macro_function]
def make_type_macro(name : string; var someClassPtr) : TypeMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        let classInfo = class_info(*someClassPtr)
        return make_type_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

var private gc_root_AstSimulateMacro : array<AstSimulateMacro?>

[macro_function]
def make_simulate_macro(name : string; var someClassPtr) : SimulateMacroPtr {
    static_if (typeinfo is_pointer(someClassPtr) && typeinfo is_class(*someClassPtr)) {
        gc_root_AstSimulateMacro |> push(someClassPtr)
        let classInfo = class_info(*someClassPtr)
        return make_simulate_macro(name, someClassPtr, classInfo)
    } else {
        concept_assert(false, "expecting pointer to class")
        return null
    }
}

[macro_function]
def add_new_block_annotation(name : string; var someClassPtr) {
    var ann = make_block_annotation(name, someClassPtr)
    this_module() |> add_function_annotation(ann)
}

[macro_function]
def add_new_function_annotation(name : string; var someClassPtr) {
    var ann = make_function_annotation(name, someClassPtr)
    this_module() |> add_function_annotation(ann)
}

[macro_function]
def add_new_contract_annotation(name : string; var someClassPtr) {
    var ann = make_function_annotation(name, someClassPtr)
    this_module() |> add_function_annotation(ann)
}

[macro_function]
def add_new_structure_annotation(name : string; var someClassPtr) {
    var ann = make_structure_annotation(name, someClassPtr)
    this_module() |> add_structure_annotation(ann)
}

[macro_function]
def add_new_enumeration_annotation(name : string; var someClassPtr) {
    var ann = make_enumeration_annotation(name, someClassPtr)
    this_module() |> add_enumeration_annotation(ann)
}

[macro_function]
def add_new_variant_macro(name : string; var someClassPtr) {
    var ann = make_variant_macro(name, someClassPtr)
    this_module() |> add_variant_macro(ann)
}

[macro_function]
def add_new_for_loop_macro(name : string; var someClassPtr) {
    var ann = make_for_loop_macro(name, someClassPtr)
    this_module() |> add_for_loop_macro(ann)
}

[macro_function]
def add_new_capture_macro(name : string; var someClassPtr) {
    var ann = make_capture_macro(name, someClassPtr)
    this_module() |> add_capture_macro(ann)
}

[macro_function]
def add_new_type_macro(name : string; var someClassPtr) {
    var ann = make_type_macro(name, someClassPtr)
    this_module() |> add_type_macro(ann)
}

[macro_function]
def add_new_simulate_macro(name : string; var someClassPtr) {
    var ann = make_simulate_macro(name, someClassPtr)
    this_module() |> add_simulate_macro(ann)
}

[macro_function]
def add_new_reader_macro(name : string; var someClassPtr) {
    var ann = make_reader_macro(name, someClassPtr)
    this_module() |> add_reader_macro(ann)
}

def add_new_comment_reader(name : string; var someClassPtr) {
    var ann = make_comment_reader(name, someClassPtr)
    this_module() |> add_comment_reader(ann)
}

[macro_function]
def add_new_call_macro(name : string; var someClassPtr) {
    var ann = make_call_macro(name, someClassPtr)
    this_module() |> add_call_macro(ann)
}

[macro_function]
def add_new_typeinfo_macro(name : string; var someClassPtr) {
    var ann = make_typeinfo_macro(name, someClassPtr)
    this_module() |> add_typeinfo_macro(ann)
}

[macro_function]
def add_new_infer_macro(name : string; var someClassPtr) {
    var ann = make_pass_macro(name, someClassPtr)
    this_module() |> add_infer_macro(ann)
}

[macro_function]
def add_new_dirty_infer_macro(name : string; var someClassPtr) {
    var ann = make_pass_macro(name, someClassPtr)
    this_module() |> add_dirty_infer_macro(ann)
}

[macro_function]
def add_new_lint_macro(name : string; var someClassPtr) {
    var ann = make_pass_macro(name, someClassPtr)
    this_module() |> add_lint_macro(ann)
}

[macro_function]
def add_new_global_lint_macro(name : string; var someClassPtr) {
    var ann = make_pass_macro(name, someClassPtr)
    this_module() |> add_global_lint_macro(ann)
}

[macro_function]
def add_new_optimization_macro(name : string; var someClassPtr) {
    var ann = make_pass_macro(name, someClassPtr)
    this_module() |> add_optimization_macro(ann)
}

[generic]
def find_module(prog : smart_ptr<Program>; name : string) : Module? {
    var rm : Module?
    program_for_each_module(prog) $(mod) {
        if (string(mod.name) == name) {
            rm = mod
        }
    }
    if (rm == null) {
        panic("module '{name}' not found")
    }
    return rm
}

[generic]
def find_module(name : string) : Module? {
    return find_module(this_program(), name)
}

[generic]
def find_compiling_module(name : string) : Module? {
    return find_module(compiling_program(), name)
}
