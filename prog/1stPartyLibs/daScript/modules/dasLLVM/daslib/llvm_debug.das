options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true

module llvm_debug shared private

require debugapi
require llvm
require llvm/llvm_boost

def report_to_debugger ( var ctx:Context; category,name:string; value:auto(TT) )
    let tinfo = typeinfo(rtti_typeinfo value)
    let pdata = unsafe(addr(value))
    report_context_state(ctx, category, name, unsafe(addr(tinfo)), pdata)

def isLLVMOpaquePtr ( _vinfo; what:string )
    if _vinfo.basicType != Type tPointer || _vinfo.firstType == null
        return false
    let vinfo = _vinfo.firstType
    if vinfo.basicType != Type tHandle
        return false
    if vinfo.annotation._module.name!="llvm"
        return false
    if vinfo.annotation.name!=what
        return false
    return true

def isLLVMOpaqueValuePtr ( vinfo )
    return isLLVMOpaquePtr(vinfo,"LLVMOpaqueValue")

def isLLVMOpaqueTypePtr ( vinfo )
    return isLLVMOpaquePtr(vinfo,"LLVMOpaqueType")

def isLLVMOpaqueBlockPtr ( vinfo )
    return isLLVMOpaquePtr(vinfo,"LLVMOpaqueBasicBlock")

def describe_arg ( var ctxid:Context; vinfo; arg:void? ) : void
    unsafe
        let parg = reinterpret<void??> arg
        if parg==null || *parg==null
            report_to_debugger(ctxid, "llvm_debug", vinfo.name, null)
            return
        if isLLVMOpaqueValuePtr(vinfo)
            let pexpression = reinterpret<LLVMOpaqueValue??> arg
            report_to_debugger(ctxid, "llvm_debug", vinfo.name, describe(*pexpression))
        elif isLLVMOpaqueTypePtr(vinfo)
            let pexpression = reinterpret<LLVMOpaqueType??> arg
            report_to_debugger(ctxid, "llvm_debug", vinfo.name, describe(*pexpression))
        elif isLLVMOpaqueBlockPtr(vinfo)
            let pexpression = reinterpret<LLVMOpaqueBasicBlock??> arg
            report_to_debugger(ctxid, "llvm_debug", vinfo.name, describe(*pexpression))
            report_to_debugger(ctxid, "llvm_debug", "{vinfo.name}.detail", describe(LLVMBasicBlockAsValue(*pexpression)))

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
            describe_arg(*ctxid, vinfo, arg)
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
    install_new_debug_agent(new ContextStateAgent(), "llvm_debug")

[_macro]
def private auto_start_llvm_state()
    if is_compiling_macros_in_module("llvm_debug")
        if !is_in_debug_agent_creation()
            if !has_debug_agent_context("llvm_debug")
                fork_debug_agent_context(@@state_agent)
