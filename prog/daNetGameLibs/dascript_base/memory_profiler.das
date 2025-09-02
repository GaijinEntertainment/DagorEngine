options indenting = 4
options no_unused_block_arguments
options no_unused_function_arguments
options skip_module_lock_checks
options no_global_variables = false

module memory_profiler

require daProfiler
require debugapi
require rtti
require math
require daslib/strings_boost

// USAGE:
// 1. add `require %danetlibs.dascript_base.memory_profiler` to your script
// 2. play the game
// 3. exit the game or reload the script with hot reload, results will be printed to the log file.
//    (look for `memory used by context`)


let
    MEMORY_PROFILER_AGENT_NAME = "memory_profiler_agent"
    VALUE_WIDTH = 12

struct FuncData
    name : string
    mangledName : string
    line : int
    file : string


struct TreeData
    totalUsedHeap : uint64
    totalUsedStringHeap : uint64
    usedHeap : uint64
    usedStringHeap : uint64
    calls : uint64
    children : table<uint64; TreeData?>
    hasHeap : bool


struct ContextData
    name : string
    functions : table<uint64 /* mangled name hash*/; FuncData>
    tree : table<uint64; TreeData?>


struct Event
    @do_not_delete data : TreeData?
    heap : uint64
    stringHeap : uint64
    childHeap : uint64
    childStringHeap : uint64


var data : table<uint64 /*ctx addr*/; ContextData>
var stack : array<Event>


def name_max_width(ctx : ContextData; data : table<uint64; TreeData?>; offset = 0) : int
    if length(data) == 0
        return 0
    var res = 0
    for mnh, it in keys(data), values(data)
        if it.hasHeap
            ctx.functions |> get(mnh) <| $(fn)
                res = max(res, length(fn.name) + offset)
            if length(it.children) > 0
                let childMax = name_max_width(ctx, it.children, offset + 2)
                res = max(res, childMax)
    return res


def print_tree(ctx : ContextData; data : table<uint64; TreeData?>; fn_max_width : int; offset = 0)
    if length(data) == 0
        return
    for mnh, it in keys(data), values(data)
        if it.hasHeap
            ctx.functions |> get(mnh) <| $(fn)
                print("{repeat(" ", offset)}{wide(fn.name, fn_max_width - offset)}{wide(string(int64(it.totalUsedHeap)), VALUE_WIDTH)}{wide(string(int64(it.usedHeap)), VALUE_WIDTH)}{wide(string(int64(it.totalUsedStringHeap)), VALUE_WIDTH)}{wide(string(int64(it.usedStringHeap)), VALUE_WIDTH)}{wide(string(it.calls), 6)}\t{fn.file}:{fn.line} {fn.mangledName}")
            if length(it.children) > 0
                print_tree(ctx, it.children, fn_max_width, offset + 2)


def print_data(tab : ContextData)
    let nameWidth = max(25, name_max_width(tab, tab.tree) + 1)
    var totalHeap = 0ul
    var totalStringHeap = 0ul
    for it in values(tab.tree)
        totalHeap += it.totalUsedHeap
        totalStringHeap += it.totalUsedStringHeap
    print("memory used by context '{tab.name}' allocated heap: {int64(totalHeap)} bytes, string heap {int64(totalStringHeap)} bytes")
    print("{wide("function", nameWidth)}{wide("heap (total)", VALUE_WIDTH)}{wide("heap", VALUE_WIDTH)}{wide("str (total)", VALUE_WIDTH)}{wide("strings", VALUE_WIDTH)}calls\tlocation")
    print_tree(tab, tab.tree, nameWidth)


class MemoryProfilerAgent : DapiDebugAgent

    [unused_argument(fun)] def override onInstrumentFunction(var ctx : Context; fun : SimFunction ?; entering : bool; user_data : uint64) : void
        var heapStats : uint64[2]
        unsafe(get_heap_stats(ctx, unsafe(addr(heapStats[0]))))
        let stringHeapSize = heapStats[1] - unsafe(temp_string_size(ctx))
        let ctxId = intptr(unsafe(addr(ctx)))
        if entering
            var fnData : TreeData?
            if length(stack) == 0
                if data[ctxId].tree[user_data] == null
                    data[ctxId].tree[user_data] = new TreeData()
                fnData = data[ctxId].tree[user_data]
            else
                var parentEvent & = unsafe(stack[length(stack) - 1])
                if parentEvent.data.children[user_data] == null
                    parentEvent.data.children[user_data] = new TreeData()
                fnData = parentEvent.data.children[user_data]
            ++fnData.calls
            stack |> emplace() <| Event(data = fnData, heap = heapStats[0], stringHeap = stringHeapSize)
        elif length(stack) > 0
            var event & = unsafe(stack[length(stack) - 1])
            let totalHeapDiff = heapStats[0] - event.heap
            let totalStringHeapDiff = stringHeapSize - event.stringHeap
            event.data.totalUsedHeap += totalHeapDiff
            event.data.totalUsedStringHeap += totalStringHeapDiff

            let heapDiff = totalHeapDiff - event.childHeap
            let stringHeapDiff = totalStringHeapDiff - event.childStringHeap
            event.data.usedHeap += heapDiff
            event.data.usedStringHeap += stringHeapDiff
            let hasHeap = totalHeapDiff != 0ul || totalStringHeapDiff != 0ul || event.childHeap != 0ul || event.childStringHeap != 0ul
            event.data.hasHeap ||= hasHeap

            if event.data.hasHeap && length(stack) > 1
                var parentEvent & = unsafe(stack[length(stack) - 2])
                parentEvent.data.hasHeap = true
                parentEvent.childHeap += totalHeapDiff
                parentEvent.childStringHeap += totalStringHeapDiff

            stack |> pop()

    def override onSimulateContext(var ctx : Context) : void
        if ctx.category.debug_context || ctx.category.debugger_tick || ctx.category.macro_context
            return
        let ctxName = "{ctx.name}"
        ctx |> instrument_all_functions() <| $ [unused_argument(fn)] (fn, sfn)
            let at = sfn.lineInfo
            var fileName = ctxName
            var line = 0
            if at != null
                line = int(at.line)
                if at.fileInfo != null
                    at.fileInfo.name |> peek() <| $(file_name)
                        daProfiler_resolve_path(file_name) <| $(path)
                            fileName := path
            else
                fileName = ctxName

            let ctxId = intptr(unsafe(addr(ctx)))
            assume tab = data[ctxId]
            tab.functions[sfn.mangledNameHash] <- FuncData(name = sfn.name, mangledName = sfn.mangledName, line = line, file = fileName)
            if empty(tab.name) && !empty(ctx.name)
                tab.name = string(ctx.name)
            return sfn.mangledNameHash

    def override onUninstall(agent : DebugAgent?)
        if agent == thisAgent
            unsafe
                delete data
            delete stack
            delete self

    def override onDestroyContext(var ctx : Context) : void
        if ctx.category.debug_context || ctx.category.debugger_tick || ctx.category.macro_context || ctx.category.folding_context
            return
        let ctxId = intptr(unsafe(addr(ctx)))
        let found = data |> get(ctxId) <| $(tab)
            tab |> print_data()
            unsafe
                delete tab

        if found
            data |> erase(ctxId)


[export, unused_argument(ctx)]
def memory_profiler_agent(ctx : Context)
    install_new_debug_agent(new MemoryProfilerAgent(), MEMORY_PROFILER_AGENT_NAME)


[init, export]
def init()
    if !has_debug_agent_context(MEMORY_PROFILER_AGENT_NAME) && !is_in_aot() && !is_in_completion() && !is_in_debug_agent_creation()
        fork_debug_agent_context(@@memory_profiler_agent)
