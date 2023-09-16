options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module ast_debug shared private

require debugapi
require daslib/ast_boost
require daslib/strings

def report_to_debugger ( var ctx:Context; category,name:string; value:auto(TT) )
    let tinfo = typeinfo(rtti_typeinfo value)
    let pdata = unsafe(addr(value))
    report_context_state(ctx, category, name, unsafe(addr(tinfo)), pdata)

def isExpressionType ( _vinfo )
    if _vinfo.basicType != Type tPointer || _vinfo.firstType == null
        return false
    let vinfo = _vinfo.firstType
    if vinfo.basicType != Type tHandle
        return false
    if vinfo.annotation._module.name!="ast"
        return false
    if !string(vinfo.annotation.name) |> starts_with("Expr")
        return false
    return true

def isModulePtrType ( _vinfo; mod, what:string )
    if _vinfo.basicType != Type tPointer || _vinfo.firstType == null
        return false
    let vinfo = _vinfo.firstType
    if vinfo.basicType != Type tHandle
        return false
    if vinfo.annotation._module.name!=mod
        return false
    if vinfo.annotation.name!=what
        return false
    return true

def isAstPtrType ( _vinfo; what:string )
    return isModulePtrType(_vinfo,"ast",what)

def isRttiPtrType ( _vinfo; what:string )
    return isModulePtrType(_vinfo,"rtti",what)

def isRttiType ( vinfo; what:string )
    if vinfo.basicType != Type tHandle
        return false
    if vinfo.annotation._module.name!="rtti"
        return false
    if vinfo.annotation.name!=what
        return false
    return true

def describe_arg ( var ctxid:Context; vinfo; arg:void? ) : void
    unsafe
        if isRttiType(vinfo,"AnnotationList")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<AnnotationList?> arg))
            return
        elif isRttiType(vinfo,"AnnotationArgumentList")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<AnnotationArgumentList?> arg))
            return
        let parg = reinterpret<void??> arg
        if parg==null || *parg==null
            report_to_debugger(ctxid, "ast_debug", vinfo.name, null)
            return
        if isExpressionType(vinfo)
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<ExpressionPtr?> arg))
            report_to_debugger(ctxid, "ast_debug", vinfo.name+".detailed", debug_expression(*reinterpret<ExpressionPtr?> arg))
        elif isAstPtrType(vinfo,"TypeDecl")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<TypeDeclPtr?> arg))
        elif isAstPtrType(vinfo,"Function")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<FunctionPtr?> arg))
        elif isAstPtrType(vinfo,"Variable")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(*reinterpret<VariablePtr?> arg))
        elif isRttiPtrType(vinfo,"AnnotationDeclaration")
            report_to_debugger(ctxid, "ast_debug", vinfo.name, describe(**reinterpret<AnnotationDeclarationPtr?> arg))

class SampleStackWalker : DapiStackWalker
    ctxid : Context?
    def override onArgument ( info:FuncInfo; index:int; vinfo:VarInfo; arg:float4 ) : void
        if vinfo.flags.ref || vinfo.flags.refType
            unsafe
                describe_arg(*ctxid,vinfo,* reinterpret<void??>(addr(arg)))
        else
            describe_arg(*ctxid,vinfo,unsafe(addr(arg)))
    def override onVariable ( inf:FuncInfo; vinfo:LocalVariableInfo; arg:void?; inScope:bool ) : void
        if !inScope
            return
        if vinfo.flags.ref || vinfo.flags.refType
            unsafe
                describe_arg(*ctxid,vinfo,* reinterpret<void??>(arg))
        else
            describe_arg(*ctxid,vinfo,arg)
    def override onAfterCall ( pp:Prologue ) : bool
        return false    // only stack top

class ContextStateAgent : DapiDebugAgent
    walker_adapter : smart_ptr<StackWalker>
    walker : SampleStackWalker?
    def ContextStateAgent
        walker = new SampleStackWalker()
        unsafe
            walker_adapter <- make_stack_walker(walker)
    def operator delete
        unsafe
            delete walker_adapter
            delete walker
    def override onCollect(var ctx:Context; at:LineInfo)
        walker.ctxid = unsafe(addr(ctx))
        walk_stack(walker_adapter,ctx,at)


def state_agent ( ctx : Context )
    install_new_debug_agent(new ContextStateAgent(), "ast_debug")

[_macro]
def private auto_start_state()
    if is_compiling_macros_in_module("ast_debug")
        if !is_in_debug_agent_creation()
            if !has_debug_agent_context("ast_debug")
                fork_debug_agent_context(@@state_agent)
