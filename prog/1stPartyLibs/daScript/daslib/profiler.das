options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module profiler shared private

require debugapi
require fio
require strings
require uriparser
require ast

[skip_field_lock_check]
struct PerfNode
    fun : SimFunction?
    count : uint64
    total_time : int64
    enter_time : int64
    total_heap_bytes : uint64
    enter_heap_bytes : uint64
    total_string_heap_bytes : uint64
    enter_string_heap_bytes : uint64
    children : table<SimFunction?;PerfNode?>

struct PerfEvent
    fun : SimFunction?
    ts : int64
    entering : bool
    heapBytes : uint64
    stringHeapBytes : uint64

[skip_field_lock_check]
struct PerfContext
    events : array<PerfEvent>
    enabled : bool

var [[do_not_delete]] g_d_agent : ProfilerDebugAgent?

def public set_enable_profiler ( var ctxId:uint64; enabled:bool )
    if g_d_agent!=null
        g_d_agent->enable_profiler(ctxId, enabled)

class ProfilerDebugAgent : DapiDebugAgent
    us0 : int64 = 0l
    events : table<uint64;PerfContext>
    out : FILE const?
    firstRecord : bool = true
    manual : bool = false
    report_memory : bool = false
    instrumented : table<uint64>
    def dependency
        return @@set_enable_profiler
    def override onInstall ( agent:DebugAgent? ) : void
        this_context().name := "__PROFILER__"
        var args <- get_command_line_arguments()
        for argv,i in args,count()
            if argv=="--das-profiler-log-file" && out==null && ((i+1)<length(args))
                out = fopen(args[i+1], "wb")
            elif argv=="--das-profiler-manual"
                manual = true
            elif argv=="--das-profiler-memory"
                report_memory = true
                unsafe
                    self.onInstrumentFunction = reinterpret<function<(var agent:DapiDebugAgent;var ctx:Context;fn:SimFunction?;entering:bool;userData:uint64):void>> self.onInstrumentFunctionWithMemory
        write("[")
        g_d_agent = unsafe(addr(self))
        us0 = ref_time_ticks()
    def override onUninstall ( agent:DebugAgent? ) : void
        if out!=null
            write("]")
            out |> fclose()
            out = null
    def isProfileable ( var ctx:Context )
        return !(ctx.category.debug_context || ctx.category.macro_context || ctx.category.folding_context ||
            ctx.category.debugger_tick || ctx.category.debugger_attached)
    def override onCreateContext ( var ctx:Context ) : void
        if !isProfileable(ctx) || (instrumented |> key_exists(ctx.getCodeAllocatorId))
            return
        instrumented |> insert(ctx.getCodeAllocatorId)
        events[intptr(unsafe(addr(ctx)))].enabled = !manual
        instrument_all_functions_thread_local(ctx)
    def enable_profiler ( ctxId:uint64; enabled:bool ) : void
        events[ctxId].enabled = enabled
    def override onDestroyContext ( var ctx:Context ) : void
        if !isProfileable(ctx)
            return
        let tid = intptr(unsafe(addr(ctx)))
        if events |> key_exists(tid)
            dump_context(ctx, tid)
            delete events[tid]
        instrumented |> erase(ctx.getCodeAllocatorId)
    def onInstrumentFunctionWithMemory ( var ctx:Context; fun:SimFunction?; entering:bool; userData:uint64 ) : void
        unsafe
            let tid = intptr(addr(ctx))
            var ev & = events[tid]
            if ev.enabled
                var hb_shb : uint64[2]
                unsafe(get_heap_stats(ctx, unsafe(addr(hb_shb[0]))))
                ev.events |> push([[PerfEvent
                    fun = fun,
                    entering = entering,
                    ts = get_time_nsec(us0),
                    heapBytes = hb_shb[0],
                    stringHeapBytes = hb_shb[1] ]])
    def override onInstrumentFunction ( var ctx:Context; fun:SimFunction?; entering:bool; userData:uint64 ) : void
        unsafe
            let tid = intptr(addr(ctx))
            var ev & = events[tid]
            if ev.enabled
                ev.events |> push([[PerfEvent
                    fun = fun,
                    entering = entering,
                    ts = get_time_nsec(us0)]])
    def dump(text:string)
        if firstRecord
            firstRecord = false
        else
            write(",\n")
        write(text)
    def write(text:string)
        if out != null
            out |> fprint(text)
    def dump_event(ev:PerfEvent; tid:uint64)
        let phase = ev.entering ? "B" : "E"
        let fnName = ev.fun.mangledName |> escape()
        dump("\{\"name\":\"{fnName}\",\"cat\": \"PERF\",\"ph\":\"{phase}\",\"pid\": 0,\"tid\":{int64(tid)},\"ts\":{ev.ts/1000l}\}")
    def dump_node ( node:PerfNode?; tab:int = -1 )
        if node==null
            return
        if node.fun!=null
            let tabs = repeat("  ",tab)
            if report_memory
                var hb = node.total_heap_bytes
                var shb = node.total_string_heap_bytes
                for ch in values(node.children)
                    hb -= ch.total_heap_bytes
                    shb -= ch.total_string_heap_bytes
                to_log(LOG_INFO,"{tabs}{node.fun.mangledName} {int64(node.count)} {node.total_time}ns heap={int64(hb)} string_heap={int64(shb)}\n")
            else
                to_log(LOG_INFO,"{tabs}{node.fun.mangledName} {int64(node.count)} {node.total_time}ns\n")
        for ch in values(node.children)
            dump_node(ch, tab+1)
    def dump_context_stack ( tid:uint64 )
        // print("context: {tid} // {ctx.name} {ctx.category}\n")
        var root = new PerfNode()
        var stack : array<PerfNode?>
        var sp = root
        for ev in events[tid].events
            if manual && ev.fun.mangledName |> starts_with("@profiler_boost::")
                continue
            if ev.entering
                stack |> push(sp)
                if !sp.children |> key_exists(ev.fun)
                    sp.children[ev.fun] = new [[PerfNode fun=ev.fun]]
                sp = sp.children[ev.fun]
                sp.count ++
                sp.enter_time = ev.ts
                sp.enter_heap_bytes = ev.heapBytes
                sp.enter_string_heap_bytes = ev.stringHeapBytes
            else
                sp.total_time += ev.ts - sp.enter_time
                sp.total_heap_bytes += ev.heapBytes - sp.enter_heap_bytes
                sp.total_string_heap_bytes += ev.stringHeapBytes - sp.enter_string_heap_bytes
                sp = length(stack)!=0 ? stack |> back() : root
                if length(stack)>0 // protection for mismatching enable\disable
                    stack |> pop()
        dump_node(root)
        unsafe
            delete root
    def dump_meta ( var ctx:Context; tid:uint64 )
        let ctxName = empty(ctx.name) ? "" : " '{ctx.name}'"
        let wasDead = (ctx.category & context_category_flags dead) == context_category_flags dead
        ctx.category &= ~context_category_flags dead
        let ctxDesc = "{tid}{ctxName} cat:{ctx.category}" |> escape()
        dump("\{\"args\": \{ \"name\": \"{ctxDesc}\" \}, \"name\": \"thread_name\", \"cat\": \"__metadata\", \"ph\": \"M\", \"pid\": 0, \"tid\": {int64(tid)}, \"ts\": 0 \}")
        if wasDead { ctx.category |= context_category_flags dead; }
        // dump("\{\"args\": \{ \"value\" : \"ns\" \}, \"name\": \"displayTimeUnit\", \"cat\": \"__metadata\", \"ph\": \"M\", \"pid\": 0, \"tid\": {int64(tid)}, \"ts\": 0 \}")
    def dump_events ( tid:uint64 )
        for ev in events[tid].events
            dump_event(ev, tid)
    def dump_context ( var ctx:Context; tid:uint64 )
        dump_context_stack(tid)
        dump_meta(ctx, tid)
        dump_events(tid)
        delete events[tid].events
        events |> erase(tid)



def debug_agent ( ctx : Context )
    assert(this_context().category.debug_context)
    install_new_thread_local_debug_agent(new ProfilerDebugAgent())

[_macro]
def installing
    if is_compiling_macros_in_module("profiler") && !is_in_completion()
        if !is_in_debug_agent_creation()
            if !has_debug_agent_context("profiler")
                fork_debug_agent_context(@@debug_agent)
