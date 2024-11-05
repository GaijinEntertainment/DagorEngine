options persistent_heap = true
options debugger = true

require rtti
require debugapi
require strings

// sample of debug agent

class SampleDebugAgent : DapiDebugAgent
    def SampleDebugAgent
        pass
    def override onAllocate ( var ctx:Context; data:void?; size:uint64; at:LineInfo ) : void
		print("{ctx} {ctx.name} : allocate {size} bytes at {describe(at)}\n")
    def override onReallocate ( var ctx:Context; data:void?; size:uint64; newData:void?; newSize:uint64; at:LineInfo ) : void
		print("{ctx} {ctx.name} : reallocate {size} bytes to {newSize} bytes at {describe(at)}\n")
    def override onFree ( var ctx:Context; data:void?; at:LineInfo ) : void
		print("{ctx} {ctx.name} : free at {describe(at)}\n")
    def override onAllocateString ( var ctx:Context; data:void?; size:uint64; at:LineInfo ) : void
		let str = escape(unsafe(reinterpret<string> data))
		print("{ctx} {ctx.name} : allocate string {intptr(data)} '{str}' of length {size} bytes at {describe(at)}\n")
    def override onFreeString ( var ctx:Context; data:void?; at:LineInfo ) : void
		let str = escape(unsafe(reinterpret<string> data))
		print("{ctx} {ctx.name} : free string {intptr(data)} `{str}` at {describe(at)}\n")

[export]
def debug_agent ( ctx : Context )
    assert(this_context().category.debug_context)
    install_new_thread_local_debug_agent(new SampleDebugAgent())

struct Foo
	a : int

[export]
def main
    fork_debug_agent_context(@@debug_agent)
	instrument_context_allocations(this_context(), true)
	var i = 0
	i ++
	var a = "hello {i} world"
	print("a = {a}\n")
	i ++
	var b = "hello {i} world N2"
	print("b = {b}\n")
	var arr : array<int>
	for j in range(100)
		arr |> push(j)
	delete arr
	var p = new [[Foo]]
	unsafe
		delete p



