options debugger = true

require rtti
require debugapi

// sample of debug agent

class SampleStackWalker : DapiStackWalker
    def override onCallAOT ( pp:Prologue; fileName:string# ) : void
        print("AOT {fileName}\n")
    def override onCallAt ( pp:Prologue; info:FuncInfo; at:LineInfo ) : void
        print("{info.name} at {at}\n")
    def override onCall ( pp:Prologue; info:FuncInfo ) : void
        print("{info.name}\n")
    def override onArgument ( info:FuncInfo; index:int; vinfo:VarInfo; arg:float4 ) : void
        print("\targ {vinfo.name} ")
        print(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
        print("\n")
    def override onVariable ( inf:FuncInfo; vinfo:LocalVariableInfo; arg:void?; inScope:bool ) : void
        print("\tvar {vinfo.name} ")
        if inScope
            if arg != null
                print(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
            else
                print ("optimized out")
        else
            print("out of scope")
        print("\n")

class SampleDebugAgent : DapiDebugAgent
    walker_adapter : smart_ptr<StackWalker>
    walker : SampleStackWalker?
    def SampleDebugAgent
        walker = new SampleStackWalker()
        unsafe
            walker_adapter <- make_stack_walker(walker)
    def operator delete
        unsafe
            delete walker_adapter
            delete walker
    def override onBreakpoint ( var ctx:Context; at:LineInfo; reason,text:string ) : void
        print("{ctx} {ctx.name} : breakpoint at line {int(at.line)}, {reason}, {text}\n")
        collect_debug_agent_state(ctx, at)
        set_single_step(ctx,true)
    def override onSingleStep ( var ctx:Context; at:LineInfo ) : void
        print("{ctx} {ctx.name} : step at {int(at.line)}\n")
        collect_debug_agent_state(ctx, at)
        walk_stack(walker_adapter,ctx,at)
    def override onVariable ( var ctx:Context; category,name:string; info:TypeInfo; data:void? ) : void
        unsafe
            let value = sprint_data(data,addr(info),print_flags humanReadable)
            print("reported {category}:{name} = {value}\n")

[export]
def debug_agent ( ctx : Context )
    assert(this_context().category.debug_context)
    install_new_debug_agent(new SampleDebugAgent(), "debug")

def report_to_debugger( var ctx:Context; category,name:string; value:auto(TT) )
    unsafe
        let tinfo = typeinfo(rtti_typeinfo value)
        let pdata = addr(value)
        print("send {category} {name}\n")
        debug(tinfo)
        report_context_state(ctx, category, name, addr(tinfo), pdata)

class ContextStateAgent : DapiDebugAgent
    def override onCollect(var ctx: Context): void
        let sdepth = stack_depth(ctx)
        print("onCollect, sending depth {sdepth}\n")
        report_to_debugger(ctx, "Context info", "Stack depth", sdepth)
        var foo : array<bool>
        for x in range(5)
            foo |> push((x & 1)!=0)
        report_to_debugger(ctx, "Context info", "Random foo", foo)
        var bar : bool[4]
        report_to_debugger(ctx, "Context info", "Random bar", bar)

[export]
def context_state_agent ( ctx : Context )
    print("insall debug agent - context goo\n")
    assert(this_context().category.debug_context)
    install_new_debug_agent(new ContextStateAgent(), "context_state")

def foo(var x:int&)
    x ++
    x ++

[export]
def main
    this_context().name := "stackwalk main context"
    fork_debug_agent_context(@@debug_agent)
    fork_debug_agent_context(@@context_state_agent)
	var x = 5
	breakpoint()
	x = 6
	for t in range(10)
		print("x={x} t={t}\n")
		x ++
        foo(x)
    var z = 13
    z ++

