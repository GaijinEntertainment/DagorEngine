options debugger = true

require rtti
require debugapi

// sample of debug agent

class SampleDebugAgent : DapiDebugAgent
    def SampleDebugAgent
        pass
    def override onBreakpoint ( var ctx:Context; at:LineInfo; reason,text:string ) : void
        print("{ctx} {ctx.name} : breakpoint at line {int(at.line)}, {reason}, {text}\n")
        instrument_node(ctx, true) <| $ ( ati )
            if ati.fileInfo == at.fileInfo
                if ati.line == at.line + 2u
                    return true
            return false
    def override onSingleStep ( var ctx:Context; at:LineInfo ) : void
        print("{ctx} {ctx.name} : step at {int(at.line)}\n")
    def override onInstrument ( var ctx:Context; at:LineInfo ) : void
        print("{ctx} {ctx.name} : instrument at {int(at.line)}\n")
        set_single_step(ctx,true)

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
    print("before x=6\n")
	x = 6
	for t in range(10)
		print("x={x} t={t}\n")
		x ++
        foo(x)
    var z = 13
    z ++

