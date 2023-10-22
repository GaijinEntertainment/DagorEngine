options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module apply shared private

require ast
require daslib/ast_boost
require daslib/defer
require daslib/templates_boost
require daslib/macro_boost
require math

[macro_function]
def for_each_subrange ( total:int; blk:block<(r:range):void> )
    let MAX_ARGUMENTS = DAS_MAX_FUNCTION_ARGUMENTS - 2  // we need one for the data itself
    var i = 0
    while i < total
        let fromI = i
        let toI = min(i+MAX_ARGUMENTS,total)
        blk |> invoke(range(fromI,toI))
        i = toI

/*
    def apply`Foo(self:Foo;arg_field1:block<(name:string,value:field1-type):void>;arg_field2:...)
        invoke(arg_field1,"field1",self.field1)
        invoke(arg_field2,"field2",self.field2)
        ...
*/
[macro_function]
def generateApplyVisitStruct ( stype:TypeDeclPtr; frange:range; fnname:string; at:LineInfo )
    assert(stype.baseType==Type tStructure)
    assert(stype.dim |> length==0)
    var inscope selfT <- clone_type(stype)
    selfT.flags |= TypeDeclFlags isExplicit | TypeDeclFlags explicitConst
    var inscope blkList : array<ExpressionPtr>
    var inscope func_args : array<VariablePtr>
    // for fld in stype.structType.fields
    for fldi in frange
        if true
            let fld & = unsafe(stype.structType.fields[fldi])
            blkList |> emplace_new <|  qmacro(invoke($i("__arg_{fld.name}"),$v(string(fld.name)),_`_self.$f(fld.name)))
            var inscope argT <- new [[TypeDecl() baseType=Type tBlock, at=at]]                         // block<(name:string;x:field1type)>
            emplace_new(argT.argTypes, new [[TypeDecl() baseType=Type tString, at=at]])
            var inscope fldT <- clone_type(fld._type)
            if stype.flags.constant
                fldT.flags |= TypeDeclFlags constant
            if !fldT.isRefType
                fldT.flags |= TypeDeclFlags ref
            emplace(argT.argTypes, fldT)
            move_new(argT.firstType) <| new [[TypeDecl() baseType=Type tVoid,at=at]]
            argT.flags = TypeDeclFlags constant
            func_args |> emplace_new <| new [[Variable() name := "__arg_{fld.name}", _type <- argT]]
    var inscope fn <- qmacro_function(fnname) <| $ [unused_argument( _`_self)] ( _`_self:$t(selfT); $a(func_args) )
        $b(blkList)
    fn.flags |= FunctionFlags privateFunction
    compiling_module() |> add_function(fn)

[macro_function]
def generateApplyVisitStruct ( stype:TypeDeclPtr; fnname:string; at:LineInfo )
    for_each_subrange(length(stype.structType.fields)) <| $ ( frange )
        generateApplyVisitStruct(stype,frange,"{fnname}`{frange.x}`{frange.y}",at)

/*
    def apply`Foo(self:Foo;arg_field1:block<(name:string,value:field1-type):void>;arg_field2:...)
        invoke(arg_field1,"field1",self.field1)
        invoke(arg_field2,"field2",self.field2)
        ...
*/
[macro_function]
def generateApplyVisitTuple ( stype:TypeDeclPtr; frange:range; fnname:string; at:LineInfo )
    assert(stype.baseType==Type tTuple)
    assert(stype.dim |> length==0)
    var inscope selfT <- clone_type(stype)
    selfT.flags |= TypeDeclFlags isExplicit | TypeDeclFlags explicitConst
    var inscope blkList : array<ExpressionPtr>
    var inscope func_args : array<VariablePtr>
    for fldi in frange
        if true
            assume flda = stype.argTypes[fldi]
            let fldname = length(stype.argNames)==length(stype.argTypes) ? string(stype.argNames[fldi]) : "_{fldi}"
            blkList |> emplace_new <|  qmacro(invoke($i("__arg_{fldname}"),$v(fldname),_`_self.$f(fldname)))
            var inscope argT <- new [[TypeDecl() baseType=Type tBlock, at=at]]                         // block<(name:string;x:field1type)>
            emplace_new(argT.argTypes, new [[TypeDecl() baseType=Type tString, at=at]])
            var inscope fldT <- clone_type(flda)
            if stype.flags.constant
                fldT.flags |= TypeDeclFlags constant
            if !fldT.isRefType
                fldT.flags |= TypeDeclFlags ref
            emplace(argT.argTypes, fldT)
            move_new(argT.firstType) <| new [[TypeDecl() baseType=Type tVoid,at=at]]
            argT.flags = TypeDeclFlags constant
            func_args |> emplace_new <| new [[Variable() name := "__arg_{fldname}", _type <- argT]]
    var inscope fn <- qmacro_function(fnname) <| $ [unused_argument( _`_self)] ( _`_self:$t(selfT); $a(func_args) )
        $b(blkList)
    fn.flags |= FunctionFlags privateFunction
    compiling_module() |> add_function(fn)

[macro_function]
def generateApplyVisitTuple ( stype:TypeDeclPtr; fnname:string; at:LineInfo )
    for_each_subrange(length(stype.argTypes)) <| $ ( frange )
        generateApplyVisitTuple(stype,frange,"{fnname}`{frange.x}`{frange.y}",at)

/*
    def apply`Foo(self:Foo;arg_field1:block<(name:string,value:field1-type):void>;arg_field2:...)
        if variant_index(self)==0
            invoke(arg_field1,"field1",self.field1)
            return
        if variant_idnex(self)==2
            invoke(arg_field2,"field2",self.field2)
            return
        ...
*/
[macro_function]
def generateApplyVisitVariant ( stype:TypeDeclPtr; frange:range; fnname:string; at:LineInfo )
    assert(stype.baseType==Type tVariant)
    assert(stype.dim |> length==0)
    var inscope selfT <- clone_type(stype)
    selfT.flags |= TypeDeclFlags isExplicit | TypeDeclFlags explicitConst
    var inscope blkList : array<ExpressionPtr>
    var inscope func_args : array<VariablePtr>
    for fldi in frange
        if true
            assume flda = stype.argTypes[fldi]
            let fldname = length(stype.argNames)==length(stype.argTypes) ? string(stype.argNames[fldi]) : "_{fldi}"
            var inscope vexpr <- qmacro_block <|
                if variant_index(_`_self)==$v(fldi)
                    invoke($i("__arg_{fldname}"),$v(fldname),_`_self as $f(fldname))
                    return
            blkList |> emplace(vexpr)
            var inscope argT <- new [[TypeDecl() baseType=Type tBlock, at=at]]                         // block<(name:string;x:field1type)>
            emplace_new(argT.argTypes, new [[TypeDecl() baseType=Type tString, at=at]])
            var inscope fldT <- clone_type(flda)
            if stype.flags.constant
                fldT.flags |= TypeDeclFlags constant
            if !fldT.isRefType
                fldT.flags |= TypeDeclFlags ref
            emplace(argT.argTypes, fldT)
            move_new(argT.firstType) <| new [[TypeDecl() baseType=Type tVoid,at=at]]
            argT.flags = TypeDeclFlags constant
            func_args |> emplace_new <| new [[Variable() name:="__arg_{fldname}", _type <- argT]]
    var inscope fn <- qmacro_function(fnname) <| $ [unused_argument( _`_self)] ( _`_self:$t(selfT); $a(func_args) )
        $b(blkList)
    fn.flags |= FunctionFlags privateFunction
    compiling_module() |> add_function(fn)

[macro_function]
def generateApplyVisitVariant ( stype:TypeDeclPtr; fnname:string; at:LineInfo )
    for_each_subrange(length(stype.argTypes)) <| $ ( frange )
        generateApplyVisitVariant(stype,frange,"{fnname}`{frange.x}`{frange.y}",at)

[call_macro(name="apply")]  // apply(value, block)
class ApplyMacro : AstCallMacro
    //! This macro implements the apply() pattern. The idea is that for each entry in the structure, variant, or tuple,
    //! the block will be invoked. Both element name, and element value are passed to the block.
    //! For example
    //!
    //!     struct Bar
	//!         x, y : float
	//!     apply([[Bar x=1.,y=2.]]) <| $ ( name:string; field )
    //!         print("{name} = {field} ")
    //!
    //! Would print x = 1.0 y = 2.0
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify(expr.arguments |> length==2,prog,expr.at,"expecting apply(value, block)")
        if expr.arguments[0]._type!=null    // need value inferred
            var inscope argT := expr.arguments[0]._type
            macro_verify(argT.dim |> length==0,prog,expr.at,"can't apply to dim")
            macro_verify(argT.baseType==Type tStructure || argT.baseType==Type tTuple || argT.baseType==Type tVariant,
                prog,expr.at,"can only apply to {describe(expr.arguments[0]._type)}")
            macro_verify(expr.arguments[1] is ExprMakeBlock,prog,expr.at,"expecting make block, i.e. $(..)")
            var callName : string
            var nfields : int
            if argT.baseType == Type tStructure
                callName = "apply`struct`{argT.structType.name}`{expr.at.line}"
                nfields = argT.structType.fields |> length
                generateApplyVisitStruct(expr.arguments[0]._type,callName,expr.at)
            elif argT.baseType == Type tTuple
                callName = "apply`tuple`{intptr(expr)}`{expr.at.line}"
                nfields = length(argT.argTypes)
                generateApplyVisitTuple(expr.arguments[0]._type,callName,expr.at)
            elif argT.baseType == Type tVariant
                callName = "apply`variant`{intptr(expr)}`{expr.at.line}"
                nfields = length(argT.argTypes)
                generateApplyVisitVariant(expr.arguments[0]._type,callName,expr.at)
            else
                macro_error(prog,expr.at,"internal error. can't apply to {describe(argT)}")
                return <- [[ExpressionPtr]]
            // make a collection of calls
            var inscope calls : array<ExpressionPtr>
            for_each_subrange(nfields) <| $ ( frange )
                var inscope call <- new [[ExprCall() name:="_::{callName}`{frange.x}`{frange.y}", at=expr.at]]
                emplace_new(call.arguments,clone_expression(expr.arguments[0]))
                for _ in frange
                    emplace_new(call.arguments,clone_expression(expr.arguments[1]))
                calls |> emplace(call)
            if length(calls)==1
                return <- calls[0]
            var inscope callblock <- new [[ExprBlock() at=expr.at, list := calls]]
            return <- callblock
        return <- [[ExpressionPtr]]
