options debugger = true

require rtti
require debugapi

// sample of debug agent

class SampleDataWalker : DapiDataWalker
    def override Float ( var value:float & ) : void
        print("[float] {value}")
    def override Int ( var value:int & ) : void
        print("[int] {value}")

class SampleStackWalker : DapiStackWalker
    data_walker_adapter : smart_ptr<DataWalker>
    data_walker : SampleDataWalker?
    def SampleStackWalker
        data_walker = new SampleDataWalker()
        unsafe
            data_walker_adapter <- make_data_walker(data_walker)
    def operator delete
        unsafe
            delete data_walker_adapter
            delete data_walker
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
                // walk_data(data_walker_adapter,arg,*type_info(vinfo))
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
        set_single_step(ctx,true)
    def override onSingleStep ( var ctx:Context; at:LineInfo ) : void
        print("{ctx} {ctx.name} : step at {int(at.line)}\n")
        walk_stack(walker_adapter,ctx,at)

[export]
def debug_agent ( ctx : Context )
    assert(this_context().category.debug_context)
    install_new_debug_agent(new SampleDebugAgent(), "debug")

def foo(var x:int&)
    x ++
    x ++

[export]
def main
    this_context().name := "stackwalk main context"
    fork_debug_agent_context(@@debug_agent)
	var x = 5
	breakpoint()
	x = 6
	for t in range(10)
		print("x={x} t={t}\n")
		x ++
        foo(x)
    var z = 13
    z ++

