options gen2 = false
options indenting = 4
options no_unused_block_arguments
options no_unused_function_arguments
options no_global_variables = false
options no_aliasing = false
options stack = 1048576 // 1mb
options persistent_heap
options gc

module memory_profiler private

require daProfiler
require daslib/debugger
require daslib/rtti
require math
require daslib/strings_boost

// USAGE:
// 1. add `require %danetlibs.dascript_base.memory_profiler` to your script
// 2. play the game
// 3. exit the game or reload the script with hot reload, results will be printed to the log file.
//    (look for `Memory profiler results for`)


let
    MEMORY_PROFILER_AGENT_NAME = "memory_profiler_agent"
    DEEP_PROFILER = true
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
    // DEEP_PROFILER data
    heapAllocationNames : table<uint64; string> // hash of name -> name
    healAllocationSizes : table<uint64; uint64> // hash of name -> size
    stringAllocationNames : table<uint64; string> // hash of name -> name
    stringAllocationSizes : table<uint64; uint64> // hash of name -> size



struct ContextData
    name : string
    functions : table<uint64 /* mangled name hash*/; FuncData>
    tree : array<table<uint64; TreeData?>>


struct Event
    @do_not_delete data : TreeData?
    heap : uint64
    stringHeap : uint64
    childHeap : uint64
    childStringHeap : uint64

struct AllAllocations
    heap : table<uint64; uint64>
    stringHeap : table<uint64; uint64>

var private @ignore_heap_usage data : table<uint64 /*ctx addr*/; ContextData>
var private @ignore_heap_usage stack : table<uint64; array<Event>>
var private @ignore_heap_usage allocations : table<uint64; AllAllocations>


def name_max_width(ctx : ContextData; data : table<uint64; TreeData?>; offset = 0) : int
    if length(data) == 0
        return 0
    var res = 0
    for mnh, it in keys(data), values(data)
        if it.hasHeap
            get(ctx.functions, mnh) $(fn)
                res = max(res, length(fn.name) + offset)
            if length(it.children) > 0
                let childMax = name_max_width(ctx, it.children, offset + 2)
                res = max(res, childMax)
    return res


def print_tree(ctx : ContextData; var data : table<uint64; TreeData?>; fn_max_width : int; offset = 0)
    if length(data) == 0
        return
    let offsetStr = repeat(" ", offset)
    for mnh, it in keys(data), values(data)
        if it.hasHeap
            get(ctx.functions, mnh) $(fn)
                print("{offsetStr}{wide(fn.name, fn_max_width - offset)}{wide(string(int64(it.totalUsedHeap)), VALUE_WIDTH)}{wide(string(int64(it.usedHeap)), VALUE_WIDTH)}{wide(string(int64(it.totalUsedStringHeap)), VALUE_WIDTH)}{wide(string(int64(it.usedStringHeap)), VALUE_WIDTH)}{wide(string(it.calls), 6)}    {fn.file}:{fn.line} {fn.mangledName}")
            if DEEP_PROFILER
                if length(it.healAllocationSizes) > 0
                    print("{offsetStr}  -heap allocations:")
                    for hash, size in keys(it.healAllocationSizes), values(it.healAllocationSizes)
                        print("{offsetStr}    {int64(size)} bytes - {it.heapAllocationNames?[hash] ?? ""}")
                if length(it.stringAllocationSizes) > 0
                    print("{offsetStr}  -string allocations:")
                    for hash, size in keys(it.stringAllocationSizes), values(it.stringAllocationSizes)
                        print("{offsetStr}    {int64(size)} bytes - {it.stringAllocationNames?[hash] ?? ""}")

            if length(it.children) > 0
                print_tree(ctx, it.children, fn_max_width, offset + 2)


def print_data(var tab : ContextData)
    for idx, tree in count(), tab.tree
        var totalHeap = 0ul
        var totalStringHeap = 0ul
        for fnId, it in keys(tree), values(tree)
            totalHeap += it.totalUsedHeap
            totalStringHeap += it.totalUsedStringHeap
        let nameWidth = max(25, name_max_width(tab, tree) + 1)
        print("Memory profiler results for context '{tab.name}' gc step {idx + 1}")
        print("allocated heap: {int64(totalHeap)} bytes, string heap {int64(totalStringHeap)} bytes")
        print("{wide("function", nameWidth)}{wide("heap (total)", VALUE_WIDTH)}{wide("heap", VALUE_WIDTH)}{wide("str (total)", VALUE_WIDTH)}{wide("strings", VALUE_WIDTH)}calls\tlocation")
        print_tree(tab, tree, nameWidth)


def ignore_context(ctx : Context)
    return ctx.category.debug_context || ctx.category.debugger_tick || ctx.category.macro_context || ctx.category.folding_context

class MemoryProfilerAgent : DapiDebugAgent

    def getLastTreeData(ctxId : uint64; cb : block<(var data : TreeData?) : void>) : void
        get(stack, ctxId) $(events)
            if length(events) > 0
                invoke(cb, events[length(events) - 1].data)

    def atHash(at : LineInfo) : uint64
        return hash(at)

    def atName(at : LineInfo)
        return describe(at)

    def override onAllocate(var ctx : Context; data : void?; size : uint64; at : LineInfo) : void
        let ctxId = intptr(unsafe(addr(ctx)))
        getLastTreeData(ctxId) $(var tree : TreeData?)
            get_with_default(allocations, ctxId) $(var heapData)
                let hash = atHash(at)
                if !key_exists(tree.heapAllocationNames, hash)
                    tree.heapAllocationNames.insert(hash, atName(at))
                tree.healAllocationSizes.insert(hash, tree.healAllocationSizes?[hash] ?? 0ul + size)
                insert(heapData.heap, intptr(data), size)

    [unused_argument(data)] def override onReallocate(var ctx : Context; data : void ?; size : uint64; newData : void ?; newSize : uint64; at : LineInfo) : void
        let ctxId = intptr(unsafe(addr(ctx)))
        getLastTreeData(ctxId) $(var tree : TreeData?)
            get_with_default(allocations, ctxId) $(var heapData)
                let hash = atHash(at)
                if !key_exists(tree.heapAllocationNames, hash)
                    tree.heapAllocationNames.insert(hash, atName(at))
                tree.healAllocationSizes.insert(hash, tree.healAllocationSizes?[hash] ?? 0ul + newSize - size)
                heapData.heap.insert(intptr(newData), newSize)

    def override onFree(var ctx : Context; data : void?; at : LineInfo) : void
        let ctxId = intptr(unsafe(addr(ctx)))
        getLastTreeData(ctxId) $(var tree : TreeData?)
            get(allocations, ctxId) $(var heapData)
                let hash = atHash(at)
                let dataPtr = intptr(data)
                if key_exists(heapData.heap, dataPtr)
                    let size = heapData.heap[dataPtr]
                    erase(heapData.heap, dataPtr)
                    tree.healAllocationSizes.insert(hash, tree.healAllocationSizes?[hash] ?? 0ul - size)

    def override onAllocateString(var ctx : Context; data : void?; size : uint64; tempString : bool; at : LineInfo) : void
        if tempString
            return
        let ctxId = intptr(unsafe(addr(ctx)))
        getLastTreeData(ctxId) $(var tree : TreeData?)
            get_with_default(allocations, ctxId) $(var heapData)
                let hash = atHash(at)
                if !key_exists(tree.heapAllocationNames, hash)
                    tree.heapAllocationNames.insert(hash, atName(at))
                tree.stringAllocationSizes.insert(hash, tree.stringAllocationSizes?[hash] ?? 0ul + size)
                heapData.stringHeap.insert(intptr(data), size)

    def override onFreeString(var ctx : Context; data : void?; tempString : bool; at : LineInfo) : void
        if tempString
            return
        let ctxId = intptr(unsafe(addr(ctx)))
        getLastTreeData(ctxId) $(var tree : TreeData?)
            get(allocations, ctxId) $(var heapData)
                let hash = atHash(at)
                let dataPtr = intptr(data)
                if key_exists(heapData.stringHeap, dataPtr)
                    let size = heapData.stringHeap[dataPtr]
                    erase(heapData.stringHeap, dataPtr)
                    tree.stringAllocationSizes.insert(hash, tree.stringAllocationSizes?[hash] ?? 0ul - size)


    [unused_argument(fun)] def override onInstrumentFunction(var ctx : Context; fun : SimFunction ?; entering : bool; user_data : uint64) : void
        let ctxId = intptr(unsafe(addr(ctx)))
        get_with_default(stack, ctxId) $(var stackData)
            var heapStats : uint64[2]
            unsafe(get_heap_stats(ctx, unsafe(addr(heapStats[0]))))
            let stringHeapSize = heapStats[1] - unsafe(temp_string_size(ctx))
            if entering
                var fnData : TreeData?
                if length(stackData) == 0
                    get_with_default(data, ctxId) $(var tab)
                        if length(tab.tree) == 0
                            emplace(tab.tree, table<uint64; TreeData?>())
                        assume last = tab.tree[length(tab.tree) - 1]
                        if !key_exists(last, user_data)
                            insert(last, user_data, new TreeData())
                        fnData = last.get_value(user_data)
                else
                    assume parentEvent = stackData[length(stackData) - 1]
                    if !key_exists(parentEvent.data.children, user_data)
                        parentEvent.data.children.insert(user_data, new TreeData())
                    fnData = parentEvent.data.children.get_value(user_data)
                ++fnData.calls
                emplace(stackData, Event(data = fnData, heap = heapStats[0], stringHeap = stringHeapSize))
            elif length(stackData) > 0
                assume event = stackData[length(stackData) - 1]
                let totalHeapDiff = heapStats[0] - event.heap
                let totalStringHeapDiff = stringHeapSize - event.stringHeap
                event.data.totalUsedHeap += totalHeapDiff
                event.data.totalUsedStringHeap += totalStringHeapDiff

                let heapDiff = totalHeapDiff - event.childHeap
                let stringHeapDiff = totalStringHeapDiff - event.childStringHeap
                event.data.usedHeap += heapDiff
                event.data.usedStringHeap += stringHeapDiff
                event.data.hasHeap ||= totalHeapDiff != 0ul || totalStringHeapDiff != 0ul || event.childHeap != 0ul || event.childStringHeap != 0ul

                if event.data.hasHeap && length(stackData) > 1
                    assume parentEvent = stackData[length(stackData) - 2]
                    parentEvent.data.hasHeap = true
                    parentEvent.childHeap += totalHeapDiff
                    parentEvent.childStringHeap += totalStringHeapDiff

                pop(stackData)

    def override onSimulateContext(var ctx : Context) : void
        if ctx.category.debug_context || ctx.category.debugger_tick || ctx.category.macro_context
            return
        let ctxName = "{ctx.name}"
        if DEEP_PROFILER
            instrument_context_allocations(ctx, true)
        instrument_all_functions(ctx) $ [unused_argument(fn)] (fn, sfn)
            let at = sfn.lineInfo
            var fileName = ctxName
            var line = 0
            if at != null
                line = int(at.line)
                if at.fileInfo != null
                    peek(at.fileInfo.name) $(file_name)
                        daProfiler_resolve_path(file_name) $(path)
                            fileName := path
            else
                fileName = ctxName

            let ctxId = intptr(unsafe(addr(ctx)))
            get_with_default(data, ctxId) $(var tab)
                tab.functions.emplace(sfn.mangledNameHash, FuncData(name = sfn.name, mangledName = sfn.mangledName, line = line, file = fileName))
                if empty(tab.name) && !empty(ctxName)
                    tab.name = ctxName
            return sfn.mangledNameHash

    def override onBeforeGC(var ctx : Context) : void
        if ignore_context(ctx)
            return
        let ctxId = intptr(unsafe(addr(ctx)))
        get(data, ctxId) $(tab)
            emplace(tab.tree, table<uint64; TreeData?>())

    def override onUninstall(agent : DebugAgent?)
        if agent == thisAgent
            for it in values(data)
                print_data(it)
            unsafe
                delete data
            delete stack
            delete allocations
            delete self

    def override onDestroyContext(var ctx : Context) : void
        if ignore_context(ctx)
            return
        let ctxId = intptr(unsafe(addr(ctx)))
        get(data, ctxId) $(tab)
            print_data(tab)
            unsafe
                delete tab

        erase(data, ctxId)
        get(stack, ctxId) $(st)
            unsafe
                delete st
        erase(stack, ctxId)



[export, unused_argument(ctx)]
def memory_profiler_agent(ctx : Context)
    install_new_debug_agent(new MemoryProfilerAgent(), MEMORY_PROFILER_AGENT_NAME)


[init, export]
def init()
    if !has_debug_agent_context(MEMORY_PROFILER_AGENT_NAME) && !is_in_aot() && !is_in_completion() && !is_in_debug_agent_creation()
        fork_debug_agent_context(@@memory_profiler_agent)
