options rtti
options no_aot
options indenting = 4
options no_global_variables = false
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options persistent_heap = true
options strict_smart_pointers = true
options stack = 4_194_304

module debug shared

require math
require strings
require rtti
require debugapi
require fio
require network
require uriparser

require daslib/json
require daslib/json_boost
require daslib/defer
require daslib/apply_in_context
require daslib/jobque_boost
require daslib/strings_boost
require daslib/dap
require daslib/algorithm
require daslib/debug_eval


let private
    LOCAL_VARS = 1ul
    ARGUMENT_VARS = 2ul
    GLOBAL_VARS = 3ul
    STATE_VARS = 4ul
    // EVAL_POOL = 1000ul
    BLOCKS_POOL = 4000ul

    MAX_STACK = 1000ul
    MAX_VARIABLES = 10_000_000ul

    print_flags_debug = (
            print_flags escapeString
        |   print_flags namesAndDimensions
        |   print_flags singleLine
        // |   print_flags humanReadable
        // |   print_flags typeQualifiers
        // |   print_flags refAddresses
    )

def private ctx_at(var ctx: Context): DAContextAt
    unsafe
        return intptr(addr(ctx))

def describe(ti: rtti::TypeInfo)
    unsafe
        return describe(addr(ti))

def private get_int_arg(args: array<string>; name: string; def_val: int): int
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? int(args[idx + 1]) : def_val

def private ends_with_separator(str: string): bool
    for c in "\\/"
        if ends_with(str, to_char(c))
            return true
    return false

def private starts_with_separator(str: string): bool
    for c in "\\/"
        if starts_with(str, to_char(c))
            return true
    return false

def private trim_path(path: string): string
    if path == "."
        return ""
    if path |> starts_with("./") || path |> starts_with(".\\")
        return path |> slice(2)
    return path

def private join_path(a, path_b: string): string
    let b = trim_path(path_b)
    if length(a) == 0
        return fix_path(b)
    if length(b) == 0
        return fix_path(a)
    let res = build_string() <| $(builder)
        builder |> write(a)
        let ends = ends_with_separator(a)
        let starts = starts_with_separator(b)
        if ends && starts
            builder |> write(slice(b, 1))
        elif !ends && !starts
            builder |> write("/")
            builder |> write(b)
        else
            builder |> write(b)
    return fix_path(res)

def private fix_path(path: string): string
    return path |> trim_path() |> file_name_to_uri() |> normalize_uri() |> uri_to_file_name()

def private resolve_path(path: string; paths: array<string>): string
    if path |> empty()
        return path
    for it in paths
        let newPath = it |> join_path(path)
        if stat(newPath).is_valid
            return newPath
    return fix_path(path)

def private resolve_path(path: string; paths: array<string>; aliases: table<string; string>): string
    if path |> empty()
        return path
    for k, v in keys(aliases), values(aliases)
        if path |> starts_with(k)
            let fixedPath = v |> join_path <| slice(path, length(k))
            return fixedPath |> resolve_path(paths)
    return path |> resolve_path(paths)

def private resolve_path_cache(path: string; paths: array<string>; aliases: table<string; string>; var cache: table<string; string>): string
    if cache |> key_exists(path)
        return cache?[path] ?? ""
    let absPath = resolve_path(path, paths, aliases)
    cache[path] = absPath
    return absPath

def private bytes_hr(value : uint64)
    if value > uint64(1024 * 1024)
        return "{float(value) / (1024f * 1024f)}mb ({int(value)})"
    if value > uint64(1024)
        return "{float(value) / 1024f}kb ({int(value)})"
    return "{int(value)} b"


class DAWalker: DapiDataWalker

    maxChildrenCount: uint = 500u

    varsStack: array<DAVariable?>
    [[do_not_delete]] frame: DAStackFrame?
    visited: array<tuple<ps: void?; hash: uint64>>
    inlinePreviewLimit: int = 50

    def startWalk(var f: DAStackFrame; var v: DAVariable; cb: block<(): void>)
        unsafe
            frame = addr(f)
            varsStack |> push(addr(v))
        cb |> invoke()
        frame = null
        varsStack |> clear()
        delete visited

    def popStack()
        let n = length(varsStack)
        if n > 1
            varsStack |> erase(n - 1)

    def startCont()
        let n = length(varsStack)
        if varsStack[n-1].children == null
            varsStack[n-1].children = new [[array<DAVariable>]]

    def closeCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if empty(v.value)
                v.value = v._type

    def override canVisitStructure(ps: void?; si: StructInfo): bool
        for vis in visited
            if vis.ps == ps && vis.hash == si.hash
                return false
        return true

    def override beforeStructure(ps: void?; si: StructInfo): void
        visited |> emplace([[auto ps, si.hash]])

    def override beforeStructureField(ps:void?; si: StructInfo; pv: void?; vi:VarInfo; last: bool): void
        self->startCont()
        let n = length(varsStack)
        if n > 0
            var ti = type_info(vi)
            unsafe
                var v & = unsafe(varsStack[n - 1])
                *v.children |> emplace([[DAVariable uid=frame.varId++, name=vi.name, _type=describe(ti), address=intptr(pv), size=ti.size ]])
                varsStack |> push(addr((*v.children)[length(*v.children) - 1]))

    def override afterStructureField(ps:void?; si: StructInfo; pv: void?; vi:VarInfo; last: bool): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children == null
                var ti = type_info(vi)
                v.value = sprint_data_fast(pv, ti, print_flags_debug)
            if empty(v.value)
                v.value = v._type
        self->popStack()

    def override afterStructure(ps: void?; si: StructInfo): void
        visited |> erase(length(visited) - 1)
        self->closeCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children != null
                v.value = join_with_names(*v.children, inlinePreviewLimit)
            if empty(v.value)
                v.value = v._type

    def override afterStructureCancel(ps: void?; si: StructInfo): void
        visited |> erase(length(visited) - 1)
        self->closeCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children != null
                v.value = join_with_names(*v.children, inlinePreviewLimit)
            if empty(v.value)
                v.value = v._type

    def override canVisitArray(ps: void?; ti: TypeInfo): bool
        let arr = unsafe(reinterpret<DapiArray?> ps)
        return maxChildrenCount < 0u || arr.size <= maxChildrenCount

    def override canVisitArrayData(ti: TypeInfo; count:uint): bool
        return maxChildrenCount < 0u || count <= maxChildrenCount

    def override beforeArrayData(ps: void?; stride: uint; count: uint; ti:TypeInfo): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            v.indexedVariables = count

    def override beforeArrayElement(ps: void?; ti: TypeInfo; pe: void?; index: uint; last: bool): void
        self->startCont()
        let n = length(varsStack)
        if n > 0
            unsafe
                var v & = unsafe(varsStack[n - 1])
                *v.children |> emplace([[DAVariable uid=frame.varId++, name="{int64(index)}", _type=describe(ti), address=intptr(pe), size=ti.size]])
                varsStack |> push(addr((*v.children)[length(*v.children) - 1]))

    def override afterArrayElement(ps: void?; ti: TypeInfo; pe: void?; index: uint; last: bool): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children == null
                unsafe
                    v.value = sprint_data_fast(pe, addr(ti), print_flags_debug)
            if empty(v.value)
                v.value = v._type
        self->popStack()

    def override afterArrayData(ps: void?; stride: uint; count: uint; ti: TypeInfo): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if count > 500u
                print("IN VARIABLE {v.name} TOO MANY ELEMENTS {int64(count)}\n")
                assert(false)

            if v.children != null
                let preview = join(*v.children, inlinePreviewLimit - 5)
                if !empty(preview)
                    v.value = "[{int64(count)}] {preview}"
            if empty(v.value)
                v.value = "[{int64(count)}] {v._type}"

    def override beforeTable(pa: DapiTable; ti: TypeInfo): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            v.indexedVariables = pa.size

    def override canVisitTable (ps: void?; ti: TypeInfo): bool
        let pa = unsafe(reinterpret<DapiTable?> ps)
        return maxChildrenCount < 0u || pa.size <= maxChildrenCount

    def override beforeTableKey(pa: DapiTable; ti: TypeInfo; pk: void?; ki: TypeInfo; index: uint; last: bool): void
        self->startCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            unsafe
                *v.children |> emplace([[DAVariable uid=frame.varId++, name=sprint_data(pk, addr(ki), print_flags_debug), _type=describe(ki), address=intptr(pk), size=ki.size]])
                varsStack |> push(addr((*v.children)[length(*v.children) - 1]))

    def override beforeTableValue(pa: DapiTable; ti: TypeInfo; pv: void?; kv: TypeInfo; index: uint; last: bool): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            v._type = describe(kv)

    def override afterTableValue(pa: DapiTable; ti: TypeInfo; pv: void?; kv: TypeInfo; index: uint; last: bool): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children == null
                unsafe
                    v.value = sprint_data_fast(pv, addr(kv), print_flags_debug)
            if empty(v.value)
                v.value = v._type
        self->popStack()

    def override afterTable(pa: DapiTable; ti: TypeInfo): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children != null
                let preview = join_with_names(*v.children, inlinePreviewLimit - 5)
                if !empty(preview)
                    v.value = "[{int64(pa.size)}] {preview}"
            if empty(v.value)
                v.value = "[{int64(pa.size)}] {v._type}"

    tupleIndex: int = 0
    def override beforeTuple(ps: void?; ti: TypeInfo): void
        self->startCont()
        tupleIndex = 0

    def override beforeTupleEntry(ps: void?; ti: TypeInfo; pv: void?; vi: TypeInfo; last: bool): void
        self->startCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            *v.children |> emplace([[DAVariable uid=frame.varId++, name="{tupleIndex++}", _type=describe(vi), address=intptr(pv), size=vi.size]])
            unsafe
                varsStack |> push(addr((*v.children)[length(*v.children) - 1]))

    def override afterTupleEntry(ps: void?; ti: TypeInfo; pv: void?; vi: TypeInfo; last: bool): void
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children == null
                unsafe
                    v.value = sprint_data_fast(pv, addr(vi), print_flags_debug)
            if empty(v.value)
                v.value = v._type
        self->popStack()

    def override afterTuple(ps: void?; ti: TypeInfo): void
        self->closeCont()
        let n = length(varsStack)
        if n > 0
            var v & = unsafe(varsStack[n - 1])
            if v.children != null
                v.value = join(*v.children, inlinePreviewLimit)
            if empty(v.value)
                v.value = v._type

struct private DAVariable
    uid: uint64
    name: string
    value: string
    _type: string
    [[do_not_delete]] typeInfo : TypeInfo const?
    rawValue : float4
    indexedVariables: uint
    address: uint64
    size: uint
    children: array<DAVariable>?

def join_with_names(a: array<DAVariable>; limit: int): string
    return build_string() <| $(str)
        var first = true
        var len = limit
        let n = length(a)
        for child, idx in a, range(n)
            len -= length(child.name) + length(child.value) + 2
            if !first
                if len <= 3 && idx < n - 1
                    str |> write(",..")
                    break
                str |> write(", ")
                len -= 2
            first = false
            str |> write(child.name)
            str |> write(": ")
            str |> write(child.value)


def join(a: array<DAVariable>; limit: int): string
    return build_string <| $(str)
        var first = true
        let n = length(a)
        var len = limit
        for v, idx in a, range(n)
            len -= length(v.value)
            if !first
                if len <= 3 && idx < n - 1
                    str |> write(",..")
                    break
                str |> write(", ")
                len -= 2
            first = false
            str |> write(v.value)


def find_child_var(val: DAVariable; id: uint64; cb: block<(res:DAVariable): void>)
    if val.uid == id
        cb |> invoke(val)
        return true
    if val.children != null
        for c in *val.children
            if c |> find_child_var(id, cb)
                return true
    return false

def find_child_var(val: array<DAVariable>; cb: block<(res:DAVariable): bool>)
    for v in val
        if v |> find_child_var(cb)
            return true
    return false


def find_child_var(val: DAVariable; cb: block<(res:DAVariable): bool>)
    if cb |> invoke(val)
        return true
    if val.children != null
        for c in *val.children
            if c |> find_child_var(cb)
                return true
    return false



struct private DAStackFrame
    name: string
    path: string
    [[do_not_delete]] func: FuncInfo?
    isBlock: bool = false
    spAddr: uint64 = 0ul
    line: uint = 1u
    variables: array<DAVariable>
    arguments: array<DAVariable>
    globals: array<DAVariable>
    state: array<tuple<uid: uint64; name:string; vars:array<DAVariable>>>

    varId: uint64 = BLOCKS_POOL + 1ul


def find_child_var(stack: DAStackFrame; id: uint64; cb: block<(res:DAVariable): void>)
    for c in stack.variables
        if c |> find_child_var(id, cb)
            return
    for c in stack.arguments
        if c |> find_child_var(id, cb)
            return
    for c in stack.globals
        if c |> find_child_var(id, cb)
            return
    for c in stack.state
        for v in c.vars
            if v |> find_child_var(id, cb)
                return


def find_child_var(stack: DAStackFrame; cb: block<(res:DAVariable; idx: uint64): bool>): bool
    for c in stack.variables
        if cb |> invoke(c, LOCAL_VARS)
            return true
    for c in stack.arguments
        if cb |> invoke(c, ARGUMENT_VARS)
            return true
    for c in stack.globals
        if cb |> invoke(c, GLOBAL_VARS)
            return true
    for idx, c in iter_range(stack.state), stack.state
        for v in c.vars
            if cb |> invoke(v, STATE_VARS + uint64(idx))
                return true
    return false


def iter_top_child_var(stack: DAStackFrame; cb: block<(res:DAVariable): void>)
    for c in stack.variables
        cb |> invoke(c)
    for c in stack.arguments
        cb |> invoke(c)
    for c in stack.globals
        cb |> invoke(c)
    // for c in stack.state
    //     cb |> invoke(c)


class private DAStackWalker: DapiStackWalker
    dataWalkerAdapter: smart_ptr<DataWalker>
    dataWalker: DAWalker?

    collectAllGlobals : bool = false

    wasGlobalsCollected : bool = false

    [[do_not_delete]] ctx: DAContext?
    [[do_not_delete]] workingPaths: array<string>?
    [[do_not_delete]] pathAliases: table<string; string>?
    [[do_not_delete]] pathsCache: table<string; string>?

    def DAStackWalker()
        dataWalker = new DAWalker()
        unsafe
            dataWalkerAdapter <- make_data_walker(dataWalker)

    def operator delete
        unsafe
            delete dataWalkerAdapter
            delete dataWalker

    def collectGlobalAt(vinfo:VarInfo; var frame : DAStackFrame&)

        var value: void?
        unsafe
            value = get_context_global_variable(ctx.ctx, vinfo.name)
        let ti = type_info(vinfo)

        var global = [[DAVariable uid=frame.varId++, name=vinfo.name, _type=describe(ti), address=intptr(value), size=ti.size, typeInfo=ti]]

        if ti != null
            dataWalker->startWalk(frame, global) <| $
                dataWalkerAdapter |> walk_data(value, *ti)

        if global.children == null
            global.value = sprint_data_fast(value, ti, print_flags_debug)
        if empty(global.value)
            global.value = global._type

        frame.globals |> emplace(global)

    def collectGlobals()
        if wasGlobalsCollected
            return
        wasGlobalsCollected = true
        let stackLen = length(ctx.stack)
        if stackLen == 0
            return

        var frame & = unsafe(ctx.stack[stackLen-1])

        if collectAllGlobals
            for i in range(get_total_variables(*ctx.ctx))
                let vinfo & = unsafe(get_variable_info(ctx.ctx, i))
                if vinfo.name |> starts_with("g```") || vinfo.name == "__rtti_require"
                    continue
                collectGlobalAt(vinfo, frame)
            return

        if frame.func != null
            for i in range(frame.func.globalCount)
                let vinfo = unsafe(frame.func.globals[i])
                if vinfo.name |> starts_with("g```") || vinfo.name == "__rtti_require"
                    continue
                // if vinfo.scope == frame.func
                collectGlobalAt(*vinfo, frame)


    def override onCallAOT(pp:Prologue; fileName:string#): void
        // print("AOT {fileName}\n")
        let n = length(ctx.stack)
        if n > 0
            ctx.stack[n - 1].name = pp.info != null ? "def {pp.info.name} [AOT]" : "def [AOT]"
            ctx.stack[n - 1].path = "{fileName}"

            self->collectGlobals()

    def override onCallAt(pp:Prologue; info:FuncInfo; at:LineInfo): void
        // print("def {info.name} at {at}\n")
        let n = length(ctx.stack)
        if n > 0
            ctx.stack[n - 1].name = "def {info.name}"
            ctx.stack[n - 1].path = "{at.fileInfo.name}" |> resolve_path_cache(*workingPaths, *pathAliases, *pathsCache)
            ctx.stack[n - 1].line = at.line
            self->collectGlobals()

    def override onCall(pp:Prologue; info:FuncInfo): void
        // print("def {info.name}\n")
        let n = length(ctx.stack)
        if n > 0
            ctx.stack[n - 1].name = "def {info.name}"
            self->collectGlobals()

    def override onBeforeCall(pp: Prologue; sp: void?): void
        let iblock = intptr(pp._block)
        let isBlock = (iblock & 1ul) != 0ul
        let funcInfo = isBlock ? [[FuncInfo?]] : pp.info
        ctx.stack |> emplace([[DAStackFrame() func=funcInfo, isBlock=isBlock, spAddr=intptr(sp) ]])

    def override onVariable(inf: FuncInfo; vinfo: LocalVariableInfo; arg: void?; inScope: bool): void
        if !inScope
            return
        let stackLen = length(ctx.stack)
        if stackLen > 0
            var frame & = unsafe(ctx.stack[stackLen - 1])
            var ti = type_info(vinfo)
            var variable = [[DAVariable uid=frame.varId++, name=vinfo.name, _type=describe(ti), address=intptr(arg), size=ti.size, typeInfo=ti]]
            if ti != null
                dataWalker->startWalk(frame, variable) <| $
                    dataWalkerAdapter |> walk_data(arg, *ti)

            if variable.children == null
                let value = !inScope ? "<uninitialized>" : arg != null ? sprint_data_fast(arg, ti, print_flags_debug): "<optimized>"
                variable.value = value
            elif empty(variable.value)
                variable.value = variable._type

            frame.variables |> emplace(variable)

    def override onArgument (inf: FuncInfo; index: int; vinfo: VarInfo; arg: float4)
        let stackLen = length(ctx.stack)
        if  stackLen > 0
            var frame & = unsafe(ctx.stack[stackLen-1])
            let ti = type_info(vinfo)
            var variable = [[DAVariable uid=frame.varId++, name=vinfo.name, _type=describe(ti), address=ti.isRef ? intptr(unsafe(reinterpret<void?>arg)) : 0ul, size=ti.size, typeInfo=ti, rawValue=arg]]
            if ti != null
                dataWalker->startWalk(frame, variable) <| $
                    dataWalkerAdapter |> walk_data(arg, *ti)

            if variable.children == null
                variable.value = sprint_data_fast(arg, ti, print_flags_debug)
            if empty(variable.value)
                variable.value = variable._type

            frame.arguments |> emplace(variable)

enum DABreakpointState
    Uninitialized = 0
    Initialized
    Instrumented

typedef
    DAContextAt = uint64

struct private DABreakpoint
    id: uint64
    line: uint

    prevState: DABreakpointState
    state: DABreakpointState
    ctxAt : DAContextAt

typedef
    DABreakpoints = table<string; array<DABreakpoint>>

struct private DAContext
    id: uint64
    at: DAContextAt
    [[do_not_delete]] ctx: Context?
    stack: array<DAStackFrame>
    continueRequested: bool = false
    pauseRequested: bool = false
    stepInRequested: bool = false
    stepRequestedStack: int = 0 // stack depth


def private reset_debug_flags(var ctx: DAContext)
    ctx.continueRequested = false
    ctx.pauseRequested = false
    ctx.stepInRequested = false
    ctx.stepRequestedStack = 0


def private to_string(ctx: DAContext)
    return build_string() <| $(str)
        if ctx.ctx.category.debugger_attached
            str |> write("*")
        let ctxNameLen = length(ctx.ctx.name)
        if ctxNameLen > 0
            str |> write(ctx.ctx.name)
        else
            str |> write(ctx.at)
        if int(ctx.ctx.category) > 0
            str |> write(" ")
            str |> write(ctx.ctx.category)
        if ctxNameLen > 0
            str |> write(" ")
            str |> write(ctx.at)
        str |> write(" (")
        str |> write(int64(ctx.id))
        str |> write(")")


def private compare_path(p1, p2: string)
    if p1 == p2
        return true
    return to_lower(p1) == to_lower(p2)

class private DAgent: DapiDebugAgent

    walkerAdapter: smart_ptr<StackWalker>
    walker: DAStackWalker?
    server: DAServer?

    breakpoints: DABreakpoints
    breakpointId: uint64 = 1ul

    contexts: array<DAContext>
    contextId: uint64 = 1ul

    waitConnection: bool = true
    withInstruments: bool = false
    workingPaths: array<string>
    pathAliases: table<string; string>
    pathsCache: table<string; string>

    collectingAgentData: bool = false

    logStrMemory = false
    initialStrLimit = uint64(8 * 1024 * 1024)
    strLimit: uint64 = uint64(8 * 1024 * 1024)

    pauseCtx: rtti::Context? = null
    pauseAt: LineInfo const? = null

    evalFrame: DAStackFrame <- DAStackFrame()

    hwBreakpoints: array<int>

    def initAgent(var ctx: Context)
        waitConnection = wait_debugger()
        withInstruments = use_instruments()

        walker = new DAStackWalker()
        unsafe
            walkerAdapter <- make_stack_walker(walker)
            walker.workingPaths = addr(workingPaths)
            walker.pathAliases = addr(pathAliases)
            walker.pathsCache = addr(pathsCache)
        self->startServer()
        self->addContext(ctx)
        self->addContext(this_context())

    def startServer()
        let args <- get_command_line_arguments()
        let port = args |> get_int_arg("--das-debug-port", 10000)
        server = new DAServer()
        server.port = port
        unsafe
            server.agent = addr(self)
        server->make_server_adapter()
        if !server->init(port)
            to_log(LOG_ERROR, "debug server failed to initialize\n")
            unsafe
                delete server

    def addPath(path: string)
        if workingPaths |> find_index(path) < 0
            workingPaths |> push(path)

    def setPaths(paths: array<string>; aliases: table<string; string>)
        workingPaths |> clear()
        pathsCache |> clear()
        for path in paths
            self->addPath(path)

        let dasRoot = get_das_root()
        if !empty(dasRoot) && dasRoot != "."
            self->addPath(dasRoot |> join_path("daslib"))
            self->addPath(dasRoot |> join_path("src/builtin"))

        pathAliases |> clear()
        for k, v in keys(aliases), values(aliases)
            pathAliases[k] = v

    def override onUninstall(agent:DebugAgent?)
        if agent == thisAgent
            delete self

    def operator delete
        unsafe
            delete walkerAdapter
            delete walker
            delete server

    def log(msg: string)
        if server != null
            server->log(msg)
        else
            print("{msg}\n")

    def reqPause(var ctx: DAContext)
        ctx |> reset_debug_flags()
        ctx.pauseRequested = true

        if withInstruments
            for c in contexts
                if !c.ctx.category.debug_context && !ctx.ctx.category.debugger_tick
                    *c.ctx |> set_single_step(true)

    def reqResume(var exclude: DAContext)
        if withInstruments
            for ctx in contexts
                if ctx.id != exclude.id && !ctx.ctx.category.debug_context
                    *ctx.ctx |> set_single_step(false)

    def onBreakpointsChanged(ini: SetBreakpointsArguments): SetBreakpointsResponse
        let path = ini.source.path
        var res: SetBreakpointsResponse
        if key_exists(breakpoints, path)
            delete breakpoints[path]
        on_breakpoints_reset(path, length(ini.breakpoints))

        breakpoints |> erase(path)
        if length(ini.breakpoints) >= 0
            var breaks <- [[array<DABreakpoint>]]
            for b in ini.breakpoints
                breaks |> emplace([[DABreakpoint line=uint(b.line), id=breakpointId ]])
                res.breakpoints |> emplace([[Breakpoint
                    verified=!withInstruments,
                    id=double(breakpointId),
                    source=ini.source,
                    line=b.line
                ]])
                breakpointId += 1ul
            breakpoints[path] <- breaks

        if !withInstruments
            return <- res

        for resBr in res.breakpoints
            resBr.message = "{path}: instrumentation wasn't initined"

        for ctx in contexts
            if ctx.ctx.category.debug_context || ctx.ctx.category.debugger_tick
                continue
            *ctx.ctx |> clear_instruments()
            *ctx.ctx |> instrument_node(true) <| $(ati)
                if ati.fileInfo == null
                    return false
                let file = string(ati.fileInfo.name) |> resolve_path_cache(workingPaths, pathAliases, pathsCache)
                let currentPath = compare_path(file, path)
                if currentPath
                    for resBr in res.breakpoints
                        if !resBr.verified
                            resBr.message = "{path}: instumentation is inited, but this line is without instructions"
                for fileBr, brs in keys(breakpoints), values(breakpoints)
                    if compare_path(fileBr, file)
                        for br in brs
                            if br.state == DABreakpointState Uninitialized
                                br.state = DABreakpointState Initialized
                        for br in brs
                            if br.line == ati.line
                                if currentPath
                                    for resBr in res.breakpoints
                                        resBr.verified ||= uint(resBr.line) == br.line
                                        if resBr.verified
                                            br.state = DABreakpointState Instrumented
                                            resBr.message = ""
                                return true
                        break
                return false
        return <- res

    def override onInstrument(var ctx: Context; at: LineInfo): void
        self->onPause(ctx, at, "breakpoint", "")

    def override onBreakpoint(var ctx: Context; at:LineInfo; reason, text:string): void
        if reason == "exception"
            self->getContextDataFor(ctx) <| $(var ctxData: DAContext)
                ctxData |> reset_debug_flags()
        self->onPause(ctx, at, reason, text)

    def beforePause(ctxData: DAContext; at: LineInfo)
        pauseCtx = ctxData.ctx
        pauseAt = unsafe(addr(at))
        evalFrame.varId = 1000ul //EVAL_POOL
        unsafe
            delete evalFrame.variables

    def afterPause()
        pauseCtx = null
        pauseAt = null

    def onPause(var ctx: Context; at: LineInfo; reason, text: string): void
        if at.fileInfo == null
            return
        if server == null
            return
        if !waitConnection && (!server.configurationDone || !server.threadsDone)
            return
        let path = "{at.fileInfo.name}" |> resolve_path_cache(workingPaths, pathAliases, pathsCache)
        self->log("{ctx} `{reason}` breakpoint at {path}:{int(at.line)}\n")

        self->addContext(ctx)
        self->getContextDataFor(ctx) <| $(var ctxData: DAContext)
            ctx |> set_single_step(true)
            self->sendStopped(ctx, ctxData, path, at, reason, text, -1ul)
            while !ctxData.continueRequested && !ctxData.stepInRequested && ctxData.stepRequestedStack == 0
                tick_debugger()
            ctxData.continueRequested = false
            self->afterPause()
            ctx |> set_single_step(!withInstruments || ctxData.stepInRequested || ctxData.stepRequestedStack > 0)

    def override onSingleStep(var ctx: Context; at: LineInfo): void
        if at.fileInfo == null
            return
        if server == null
            return

        // unsafe
            // self->log("{double(ctx_at(ctx))} step at {string(at.fileInfo.name) |> resolve_path_cache(workingPaths, pathAliases, pathsCache)}:{int(at.line)}\n")
            // print("{double(ctx_at(ctx))} step at {string(at.fileInfo.name) |> resolve_path_cache(workingPaths, pathAliases, pathsCache)}:{int(at.line)}\n")

        if ctx.category.debugger_attached
            self->addContext(ctx)

        if waitConnection && (!server.configurationDone || !server.threadsDone)
            if ctx.category.debugger_attached
                while !server.configurationDone || !server.threadsDone
                    tick_debugger()
            else
                self->log("[E] non debug context in single step {ctx.name}")
            ctx |> set_single_step(!withInstruments)

        self->getContextDataFor(ctx) <| $(var ctxData: DAContext)
            let file = string(at.fileInfo.name) |> resolve_path_cache(workingPaths, pathAliases, pathsCache)
            if ctxData.pauseRequested
                self->sendStopped(ctx, ctxData, file, at, "pause", "", -1ul)
                while ctxData.pauseRequested
                    tick_debugger()
                self->afterPause()
                self->reqResume(ctxData)

            if ctxData.stepInRequested
                ctxData.stepInRequested = false
                self->sendStopped(ctx, ctxData, file, at, "step", "", -1ul)
                while !ctxData.continueRequested && !ctxData.stepInRequested && ctxData.stepRequestedStack == 0
                    tick_debugger()
                ctxData.continueRequested = false
                self->afterPause()

            if ctxData.stepRequestedStack > 0
                if stack_depth(*ctxData.ctx) > ctxData.stepRequestedStack
                    return
                ctxData.stepRequestedStack = 0
                self->sendStopped(ctx, ctxData, file, at, "step", "", -1ul)
                while !ctxData.continueRequested && !ctxData.stepInRequested && ctxData.stepRequestedStack == 0
                    tick_debugger()
                self->afterPause()
                ctxData.continueRequested = false

            if withInstruments
                ctx |> set_single_step(ctxData.stepInRequested || ctxData.stepRequestedStack > 0)
                return
            // self->log("search break `{file}`:{at.line} - {inBreak}\n")
            var i = 0
            while i < length(breakpoints)
                var fileBr: string
                for j, it in range(i + 1), keys(breakpoints)
                    if j == i
                        fileBr = it
                        break
                i += 1
                if !compare_path(fileBr, file)
                    continue
                breakpoints |> get(file) <| $(brs)
                    var brIdx = 0
                    while brIdx < length(brs)
                        let br & = unsafe((brs)[brIdx++])
                        // self->log("search break `{file}` {brs} in {file}\n")
                        if br.line != uint(at.line)
                            continue
                        self->sendStopped(ctx, ctxData, file, at, "breakpoint", "", br.id)

                        ctxData.continueRequested = false
                        while !ctxData.continueRequested && !ctxData.stepInRequested && ctxData.stepRequestedStack == 0
                            tick_debugger()
                        ctxData.continueRequested = false
                        self->afterPause()
                        break

    def nextStep(var ctx: Context; var ctxData:DAContext; path: string; at:LineInfo)
        unsafe
            delete ctxData.stack
        walker.wasGlobalsCollected = false
        unsafe
            walker.ctx = addr(ctxData)
        walkerAdapter |> walk_stack(ctx, at)
        if length(ctxData.stack) > 0
            for i in range(1, length(ctxData.stack))
                let j = length(ctxData.stack) - i
                let q = j - 1
                ctxData.stack[j].path = ctxData.stack[q].path
                ctxData.stack[j].line = ctxData.stack[q].line
            ctxData.stack[0].path = path
            ctxData.stack[0].line = at.line

            if !collectingAgentData
                collectingAgentData = true
                collect_debug_agent_state(ctx, at)
                collectingAgentData = false

    def override onVariable(var ctx: Context; category, name: string; info: TypeInfo; data: void?): void
        self->getContextDataFor(ctx) <| $(var ctxData: DAContext)
            let stackLen = length(ctxData.stack)
            if  stackLen > 0
                var frame & = unsafe(ctxData.stack[0])
                var variable = [[DAVariable uid=frame.varId++, name=name, _type=describe(info), address=intptr(data), size=info.size]]
                walker.dataWalker->startWalk(frame, variable) <| $
                    walker.dataWalkerAdapter |> walk_data(data, info)
                if variable.children == null
                    unsafe
                        variable.value = sprint_data_fast(data, addr(info), print_flags_debug)
                if empty(variable.value)
                    variable.value = variable._type

                for s in frame.state
                    if s.name == category
                        s.vars |> emplace(variable)
                        return
                frame.state |> emplace <| [[auto STATE_VARS+uint64(length(frame.state)), category, [{auto[] variable}] ]]

    def sendStopped(var ctx: Context; var ctxData:DAContext; path: string; at:LineInfo; reason, text: string; brId: uint64)
        self->log("<< stopped `{reason}` at {path}:{int(at.line)}")

        self->beforePause(ctxData, at)
        self->nextStep(ctx, ctxData, path, at)
        server->sendStopped(ctxData, reason, text, brId)

    def override onTick()
        if server != null
            if !server.disconnected
                server->tick()
            else
                waitConnection = false
                unsafe
                    delete server
                self->startServer()

        var alloc = string_heap_bytes_allocated()
        if alloc > strLimit
            // string_heap_report()
            var report: string
            if logStrMemory
                report = "STRING HEAP COLLECT: {bytes_hr(alloc)} -> "
            unsafe
                string_heap_collect()
            alloc = string_heap_bytes_allocated()
            if logStrMemory
                to_log(LOG_DEBUG, "dap: {report}{bytes_hr(alloc)} limit : {bytes_hr(strLimit)}\n")
            if alloc > strLimit
                strLimit *= uint64(2)
                if logStrMemory
                    to_log(LOG_DEBUG, "dap: UP string heap limit: {bytes_hr(strLimit)}\n")
            elif strLimit > initialStrLimit && alloc < strLimit / uint64(2)
                strLimit = strLimit / uint64(2)
                if logStrMemory
                    to_log(LOG_DEBUG, "dap: DOWN string heap limit: {bytes_hr(strLimit)}\n")

    def invalidateBreakpoints(var ctx: Context) : void
        if !withInstruments
            return
        if server == null || !server.configurationDone
            return
        if ctx.category.debug_context || ctx.category.debugger_tick
            return

        for brs in values(breakpoints)
            for br in brs
                br.prevState = br.state

        let found = self->getContextDataFor(ctx) <| $(var ctxData: DAContext) // TODO: speedup getContextDataFor
            *ctxData.ctx |> clear_instruments()
            *ctxData.ctx |> instrument_node(true) <| $(ati)
                if ati.fileInfo == null
                    return false
                let file = string(ati.fileInfo.name) |> resolve_path_cache(workingPaths, pathAliases, pathsCache)
                for fileBr, brs in keys(breakpoints), values(breakpoints)
                    if compare_path(fileBr, file)
                        for br in brs
                            br.ctxAt = ctxData.at
                            if br.state == DABreakpointState Uninitialized
                                br.state = DABreakpointState Initialized
                        for br in brs
                            if br.line == ati.line
                                br.state = DABreakpointState Instrumented
                                // print("dap: reset breakpoint {file}:{int(br.line)}\n")
                                return true
                        break
                return false

        if !found
            // removed context, uninitialize related breakpoints
            let ctxAt = ctx_at(ctx)
            for brs in values(breakpoints)
                for br in brs
                    if br.ctxAt == ctxAt
                        br.state = DABreakpointState Uninitialized
                        br.ctxAt = 0ul

        for fileBr, brs in keys(breakpoints), values(breakpoints)
            let baseName = fileBr |> base_name()
            for br in brs
                let wasVerified = br.prevState == DABreakpointState Instrumented
                let isVerified = br.state == DABreakpointState Instrumented
                if wasVerified != isVerified
                    server->sendEvent("breakpoint", JV([[BreakpointEvent
                        reason="changed",
                        breakpoint=[[Breakpoint
                            id=double(br.id),
                            verified=br.state == DABreakpointState Instrumented,
                            source=[[ Source path=fileBr, name=baseName]],
                            line=double(br.line),
                            message=br.state == DABreakpointState Uninitialized ? "{fileBr}: instrumentation wasn't initined" : br.state == DABreakpointState Initialized ? "{fileBr}: instumentation is inited, but this line is without instructions" : ""
                        ]]
                    ]]))

    def override onCreateContext(var ctx: Context): void
        // unsafe
        //     print("dap: context created {intptr(addr(ctx))} {ctx.category} `{ctx.name}`")
        self->addContext(ctx)
        invalidateBreakpoints(ctx)

    def override onDestroyContext(var ctx: Context): void
        // unsafe
        //     print("dap: context destroyed {intptr(addr(ctx))} {ctx.category} `{ctx.name}`\n")
        self->removeContext(ctx)
        invalidateBreakpoints(ctx)

    def getContextDataFor(var ctx: Context; cb: block<(var ctx: DAContext): void>): bool
        let at = ctx_at(ctx)
        for i in iter_range(contexts)
            if contexts[i].at == at
                cb |> invoke(contexts[i])
                return true
        return false

    def getContextData(id: DAContextAt; cb: block<(var ctx: DAContext): void>): bool
        for i in iter_range(contexts)
            if contexts[i].id == id
                cb |> invoke(contexts[i])
                return true
        return false

    def getContextIdx(var ctx: Context): int
        let ctxAt = ctx_at(ctx)
        for idx in iter_range(contexts)
            if contexts[idx].at == ctxAt
                return idx
        return -1

    def addContext(var ctx: Context): bool
        // unsafe
        //     print("dap: add context {intptr(addr(ctx))}")
        let idx = self->getContextIdx(ctx)
        if idx >= 0
            return false
        unsafe
            contexts |> emplace([[DAContext at=ctx_at(ctx), id=contextId, ctx=addr(ctx)]], 0)
        contextId += 1ul
        contexts |> sort <| $(a, b: DAContext)
            let aid = uint(a.ctx.category)
            let bid = uint(b.ctx.category)
            return aid > bid

        if server != null && server.threadsDone
            server->sendEvent("thread", JV([[ThreadEvent
                reason="started",
                threadId=double(contextId)
            ]]))

        return true

    def removeContext(var ctx: Context): void
        let idx = self->getContextIdx(ctx)
        if idx >= 0
            if server != null && server.threadsDone
                server->sendEvent("thread", JV([[ThreadEvent
                    reason="exited",
                    threadId=double(contexts[idx].id)
                ]]))
            unsafe
                delete contexts[idx]
            contexts |> erase(idx)

class private DAServer: Server
    port: int
    [[do_not_delete]] agent: DAgent?

    disconnected: bool = false

    // message data
    current_string: array<uint8>
    toRead: int = 0

    initialized: int = -1 // initialized event: ticks delay
    configurationDone: bool = false
    threadsDone: bool = false

    def DAServer
        Server`Server(cast<Server> self)

    def override onError(msg:string; code:int)
        to_log(LOG_ERROR, "dap: server error:{code} - {msg}\n")

    def override onConnect()
        to_log(LOG_INFO, "dap: connected ::{port}\n")

    def override onDisconnect()
        if length(current_string) > 0
            print(string(current_string))
            current_string |> clear()
        // print("disconnected\n")

    def restart()
        if _server != null
            unsafe
                var session : smart_ptr<NetworkServer>
                self->save(session)
                gc0_save_smart_ptr("telnet-session", session)

    def override tick()
        Server`tick(self)
        if initialized > 0
            initialized -= 1
            if initialized == 0
                self->sendEvent("initialized", null)

    def override onData(message:uint8?; size:int)
        for i in range(size)
            var uch : uint8
            unsafe
                uch = message[i]
            let ch = int(uch)
            if toRead == 0
                push(current_string, uch)
                let prefix = "Content-Length: "
                if length(current_string) <= length(prefix)
                    continue
                if ch != '\n'
                    continue
                let str = string(current_string)
                if str |> starts_with(prefix)
                    unsafe
                        let lenStr = chop(str, 16, length(current_string) - length(prefix) - 2) // \r\n length
                        toRead = to_int(lenStr)
                        clear(current_string)
                continue
            if ch == '\n' || ch == '\r'
                continue
            push(current_string, uch)
            if length(current_string) < toRead
                continue
            toRead = 0
            var error : string
            var msg = json::read_json(current_string, error)
            clear(current_string)
            self->log(">>>\n\n{write_json(msg)}\n\n")

            if msg.value is _object
                let seq = msg |> jon("seq", 0lf)
                let typ = msg |> jos("type", "n/a")
                if typ == "request"
                    let command = msg |> jos("command", "n/a")
                    let arguments = msg |> joj("arguments")
                    if command == "initialize"
                        self->reqInitialize(seq, command, arguments)
                    elif command == "configurationDone"
                        self->reqConfigurationDone(seq, command, arguments)
                    elif command == "disconnect"
                        self->reqDisconnect(seq, command, arguments)
                    elif command == "terminate"
                        self->reqTerminate(seq, command, arguments)
                    elif command == "launch"
                        self->reqLaunch(seq, command, arguments)
                    elif command == "attach"
                        self->reqAttach(seq, command, arguments)
                    elif command == "setBreakpoints"
                        self->reqSetBreakpoints(seq, command, arguments)
                    elif command == "setDataBreakpoints"
                        self->reqSetDataBreakpoints(seq, command, arguments)
                    elif command == "dataBreakpointInfo"
                        self->reqDataBreakpointInfo(seq, command, arguments)
                    elif command == "threads"
                        self->reqThreads(seq, command, arguments)
                    elif command == "stackTrace"
                        self->reqStackTrace(seq, command, arguments)
                    elif command == "scopes"
                        self->reqScopes(seq, command, arguments)
                    elif command == "variables"
                        self->reqVariables(seq, command, arguments)
                    elif command == "continue"
                        self->reqContinue(seq, command, arguments)
                    elif command == "pause"
                        self->reqPause(seq, command, arguments)
                    elif command == "stepIn"
                        self->reqStepIn(seq, command, arguments)
                    elif command == "next"
                        self->reqNext(seq, command, arguments)
                    elif command == "stepOut"
                        self->reqStepOut(seq, command, arguments)
                    elif command == "evaluate"
                        self->reqEvaluate(seq, command, arguments)
                    else
                        self->log("[Error] unknown request message\n{write_json(msg)}\n")
            else
                self->log("[Error] unknown message\n{write_json(msg)}\n")
            unsafe
                delete msg

    def reqInitialize(seq: double; command: string; arguments: JsonValue?)
        // var inscope ini <- InitializeRequestArguments(arguments)
        var inscope res = [[Capabilities
                                supportsConfigurationDoneRequest=true,
                                supportsRestartRequest=false,
                                supportTerminateDebuggee=true,
                                supportsTerminateRequest=true,
                                supportsExceptionOptions=false,
                                supportsExceptionFilterOptions=false,
                                supportsDataBreakpoints=true
                                // supportsDelayedStackTraceLoading=true
                    ]]
        self->sendSuccessResponse(seq, command, JV(res))
        initialized = 100
        // self->sendEvent("initialized")

    def reqConfigurationDone(seq: double; command: string; data: JsonValue?)
        configurationDone = true
        self->sendSuccessResponse(seq, command, null)

    def setPaths(data: JsonValue?)
        var inscope resPaths: array<string>
        var inscope pathAliases: table<string; string>
        if data != null && data is _object
            let paths = data |> joj("paths")
            if paths != null && paths is _array
                let pathsList & = unsafe(paths as _array)
                for it in pathsList
                    if it is _string
                        resPaths |> push(it as _string)
            let cwd = data |> jos("cwd", "")
            if !empty(cwd)
                resPaths |> push(cwd)
            let aliases = data |> joj("pathAliases")
            if aliases != null && aliases is _object
                let aliasesList & = unsafe(aliases as _object)
                for k, v in keys(aliasesList), values(aliasesList)
                    if v is _string
                        pathAliases[k] = v as _string
        agent->setPaths(resPaths, pathAliases)

    def setDebuggerSettings(data: JsonValue?)
        let limit = data |> jon("inlinePreviewLimit", double(agent.walker.dataWalker.inlinePreviewLimit))
        agent.walker.dataWalker.inlinePreviewLimit = int(limit)

        let maxChildrenCount = data |> jon("maxChildrenCount", double(agent.walker.dataWalker.maxChildrenCount))
        agent.walker.dataWalker.maxChildrenCount = uint(maxChildrenCount)

        let collectAllGlobals = data |> job("collectAllGlobals", agent.walker.collectAllGlobals)
        agent.walker.collectAllGlobals = collectAllGlobals

    def reqLaunch(seq: double; command: string; data: JsonValue?)
        self->setPaths(data)
        self->setDebuggerSettings(data)
        self->sendSuccessResponse(seq, command, null)

    def reqAttach(seq: double; command: string; data: JsonValue?)
        self->setPaths(data)
        self->setDebuggerSettings(data)
        self->sendSuccessResponse(seq, command, null)

    def reqDisconnect(seq: double; command: string; data: JsonValue?)
        // var inscope ini = DisconnectRequest(data)
        self->sendSuccessResponse(seq, command, null)
        for ctx in agent.contexts
            ctx |> reset_debug_flags()
            ctx.continueRequested = true
        for it in values(agent.breakpoints)
            delete it
        agent.breakpoints |> clear()
        // ini.arguments.restart
        disconnected = true

    def reqTerminate(seq: double; command: string; data: JsonValue?)
        self->sendSuccessResponse(seq, command, null)
        unsafe
            fio::exit(0)

    def reqSetBreakpoints(seq: double; command: string; data: JsonValue?)
        var inscope ini <- SetBreakpointsArguments(data)
        var inscope res <- agent->onBreakpointsChanged(ini)
        self->sendSuccessResponse(seq, command, JV(res))

    def reqDataBreakpointInfo(seq: double; command: string; data: JsonValue?)
        var inscope ini <- DataBreakpointInfoArguments(data)
        let ctxAndFrame = uint64(ini.variablesReference) / MAX_VARIABLES
        let ctxId = uint64(ctxAndFrame) / MAX_STACK
        var found = false
        var valid = false
        agent->getContextData(ctxId) <| $(var ctxData: DAContext)
            let frameId = int(ctxAndFrame % MAX_STACK)
            if frameId >= 0 && frameId < length(ctxData.stack)
                let kind = uint64(ini.variablesReference) % MAX_VARIABLES
                let line & = unsafe(ctxData.stack[frameId])
                if kind == LOCAL_VARS || kind == ARGUMENT_VARS || kind == GLOBAL_VARS
                    let vars & = unsafe(kind == LOCAL_VARS ? line.variables : kind == ARGUMENT_VARS ? line.arguments : line.globals)
                    vars |> find_child_var() <| $(v)
                        if v.name == ini.name
                            found = true
                            valid = v.address != 0ul
                            if valid
                                var inscope res = [[DataBreakpointInfoResponse dataId="{intptr(ctxData.ctx)},{v.address},{int(v.size)}", description="{v.name} ({v.address}:{min(8, int(v.size))})" ]]
                                self->sendSuccessResponse(seq, command, JV(res))
                        return found
                elif kind >= STATE_VARS && kind <= BLOCKS_POOL
                    for state in line.state
                        for v in state.vars
                            if v.name == ini.name
                                found = true
                                valid = v.address != 0ul
                                if valid
                                    var inscope res = [[DataBreakpointInfoResponse dataId="{intptr(ctxData.ctx)},{v.address},{int(v.size)}", description="{v.name} ({v.address}:{min(8, int(v.size))})" ]]
                                    self->sendSuccessResponse(seq, command, JV(res))
                                break
                else
                    line |> find_child_var(kind) <| $(v)
                        v |> find_child_var() <| $(vr)
                            if vr.name == ini.name
                                found = true
                                valid = vr.address != 0ul
                                if valid
                                    var inscope res = [[DataBreakpointInfoResponse dataId="{intptr(ctxData.ctx)},{vr.address},{int(vr.size)}", description="{v.name}.{vr.name} ({vr.address}:{min(8, int(vr.size))})" ]]
                                    self->sendSuccessResponse(seq, command, JV(res))
                            return found
        if !found || !valid
            self->sendSuccessResponse(seq, command, JVNull())

    def reqSetDataBreakpoints(seq: double; command: string; data: JsonValue?)
        var inscope ini <- SetDataBreakpointsArguments(data)
        var inscope res: SetBreakpointsResponse

        for bp in agent.hwBreakpoints
            unsafe
                clear_hw_breakpoint(bp)
        agent.hwBreakpoints |> clear()

        for b in ini.breakpoints
            var setId = -1
            var hex = 0ul
            if b.enabled
                var parts <- b.dataId |> split(",")
                let ctxHex = to_uint64(parts[0], true)
                hex = to_uint64(parts[1], true)
                let size = to_int(parts[2])
                unsafe
                    var ctxPtr = reinterpret<Context?> ctxHex
                    let ptr = reinterpret<void?> hex
                    setId = set_hw_breakpoint(*ctxPtr, ptr, clamp(size, 1, 8), true)
                if setId != -1
                    agent.hwBreakpoints |> push(setId)
                delete parts
            res.breakpoints |> emplace([[Breakpoint verified=setId != -1, id=double(hex) ]])

        self->sendSuccessResponse(seq, command, JV(res))

    def reqThreads(seq: double; command: string; data: JsonValue?)
        var inscope res: ThreadsResponseBody
        threadsDone = true
        for ctx in agent.contexts
            res.threads |> emplace([[Thread id=double(ctx.id), name=ctx |> to_string() ]])
        self->sendSuccessResponseStr(seq, command, sprint_json(res, false))

    def reqStackTrace(seq: double; command: string; data: JsonValue?)
        var inscope ini <- StackTraceArguments(data)
        var inscope res: StackTraceResponseBody
        agent->getContextData(uint64(ini.threadId)) <| $(var ctxData: DAContext)
            for idx, line in range(ini.levels), ctxData.stack
                res.stackFrames |> emplace([[StackFrame
                    id=double(ctxData.id * MAX_STACK + uint64(idx)),
                    name=line.name,
                    line=double(line.line),
                    column=1lf,
                    source=[[Source name=line.path, path=line.path]]
                ]])
            res.totalFrames = double(length(ctxData.stack))
        self->sendSuccessResponseStr(seq, command, sprint_json(res, false))

    def reqScopes(seq: double; command: string; data: JsonValue?)
        var inscope ini <- ScopesArguments(data)
        var inscope res: ScopesResponseBody
        let ctxId = uint64(ini.frameId / double(MAX_STACK))
        agent->getContextData(ctxId) <| $(var ctxData: DAContext)
            let frameId = int(ini.frameId % double(MAX_STACK))
            if frameId >= 0 && frameId < length(ctxData.stack)
                let spAddr = ctxData.stack[frameId].spAddr
                var globs = 0
                var idx = frameId
                var first = true
                while idx < length(ctxData.stack)
                    let line & = unsafe(ctxData.stack[idx])
                    if first || line.spAddr == spAddr
                        globs += length(line.globals)
                        if length(line.variables) > 0
                            res.scopes |> emplace([[Scope
                                name=first ? "Locals" : "Locals ({line.name})",
                                variablesReference=double((ctxId * MAX_STACK + uint64(idx)) * MAX_VARIABLES + LOCAL_VARS)
                            ]])
                        if length(line.arguments) > 0
                            res.scopes |> emplace([[Scope
                                name=first ? "Arguments" : "Arguments ({line.name})",
                                variablesReference=double((ctxId * MAX_STACK + uint64(idx)) * MAX_VARIABLES + ARGUMENT_VARS)
                            ]])
                        if length(line.state) > 0
                            for i, state in iter_range(line.state), line.state
                                res.scopes |> emplace([[Scope
                                    name=state.name,
                                    variablesReference=double((ctxId * MAX_STACK + uint64(idx)) * MAX_VARIABLES + STATE_VARS + uint64(i))
                                ]])
                        first = false
                    idx += 1
                if globs > 0
                    res.scopes |> emplace([[Scope
                        name="Globals",
                        variablesReference=double(uint64(ini.frameId) * MAX_VARIABLES + GLOBAL_VARS)
                    ]])
        self->sendSuccessResponseStr(seq, command, sprint_json(res, false))

    def reqVariables(seq: double; command: string; data: JsonValue?)
        var inscope ini <- VariablesArguments(data)
        var inscope res: VariablesResponseBody
        let ctxAndFrame = uint64(ini.variablesReference) / MAX_VARIABLES
        let ctxId = uint64(ctxAndFrame) / MAX_STACK
        if ctxId == 0ul // eval ctx
            agent.evalFrame |> find_child_var(uint64(ini.variablesReference)) <| $(child)
                if child.children != null
                    let varsNum = length(*child.children)
                    for idx in range(ini.start < 0lf ? 0 : min(varsNum, int(ini.start)), ini.count < 0lf ? varsNum : min(varsNum, int(ini.start + ini.count)))
                        let v & = unsafe((*child.children)[idx])
                        res.variables |> emplace([[Variable
                            name=v.name,
                            value=v.value,
                            _type=v._type,
                            indexedVariables=double(v.indexedVariables),
                            variablesReference=double(v.children != null ? v.uid : 0ul)
                        ]])
        else
            agent->getContextData(ctxId) <| $(var ctxData: DAContext)
                let frameId = int(ctxAndFrame % MAX_STACK)
                if frameId >= 0 && frameId < length(ctxData.stack)
                    let kind = uint64(ini.variablesReference) % MAX_VARIABLES
                    let line & = unsafe(ctxData.stack[frameId])
                    if kind == LOCAL_VARS || kind == ARGUMENT_VARS || kind == GLOBAL_VARS
                        let vars & = unsafe(kind == LOCAL_VARS ? line.variables : kind == ARGUMENT_VARS ? line.arguments : line.globals)
                        let varsNum = length(vars)
                        for idx in range(ini.start < 0lf ? 0 : min(varsNum, int(ini.start)), ini.count < 0lf ? varsNum : min(varsNum, int(ini.start + ini.count)))
                            let v & = unsafe(vars[idx])
                            let ref = v.children != null ? ctxAndFrame * MAX_VARIABLES + v.uid : 0ul
                            res.variables |> emplace([[Variable
                                name=v.name,
                                value=v.value,
                                _type=v._type,
                                indexedVariables=double(v.indexedVariables),
                                variablesReference=double(ref)
                            ]])
                    elif kind >= STATE_VARS && kind <= BLOCKS_POOL
                        for state in line.state
                            if state.uid == kind
                                for v in state.vars
                                    let ref = v.children != null ? ctxAndFrame * MAX_VARIABLES + v.uid : 0ul
                                    res.variables |> emplace([[Variable
                                        name=v.name,
                                        value=v.value,
                                        _type=v._type,
                                        indexedVariables=double(v.indexedVariables),
                                        variablesReference=double(ref)
                                    ]])
                                break
                    else
                        line |> find_child_var(uint64(kind)) <| $(child)
                            if child.children != null
                                let varsNum = length(*child.children)
                                for idx in range(ini.start < 0lf ? 0 : min(varsNum, int(ini.start)), ini.count < 0lf ? varsNum : min(varsNum, int(ini.start + ini.count)))
                                    let v & = unsafe((*child.children)[idx])
                                    let ref = v.children != null ? ctxAndFrame * MAX_VARIABLES + v.uid : 0ul
                                    res.variables |> emplace([[Variable
                                        name=v.name,
                                        value=v.value,
                                        _type=v._type,
                                        indexedVariables=double(v.indexedVariables),
                                        variablesReference=double(ref)
                                    ]])
        self->sendSuccessResponseStr(seq, command, sprint_json(res, false)) // TODO: , false

    def reqContinue(seq: double; command: string; data: JsonValue?)
        var inscope ini <- ContinueArguments(data)
        agent->getContextData(uint64(ini.threadId)) <| $(var ctx: DAContext)
            ctx |> reset_debug_flags()
            ctx.continueRequested = true
        self->sendSuccessResponse(seq, command, null)

    def reqPause(seq: double; command: string; data: JsonValue?)
        var inscope ini <- PauseArguments(data)
        agent->getContextData(uint64(ini.threadId)) <| $(var ctx: DAContext)
            agent->reqPause(ctx)
        self->sendSuccessResponse(seq, command, null)

    def reqStepIn(seq: double; command: string; data: JsonValue?)
        var inscope ini <- StepInArguments(data)
        agent->getContextData(uint64(ini.threadId)) <| $(var ctx: DAContext)
            ctx |> reset_debug_flags()
            ctx.stepInRequested = true
        self->sendSuccessResponse(seq, command, null)

    def reqNext(seq: double; command: string; data: JsonValue?)
        var inscope ini <- NextArguments(data)
        agent->getContextData(uint64(ini.threadId)) <| $(var ctx: DAContext)
            ctx |> reset_debug_flags()
            ctx.stepRequestedStack = stack_depth(*ctx.ctx)
            ctx.stepInRequested = ctx.stepRequestedStack == 0
        self->sendSuccessResponse(seq, command, null)

    def reqStepOut(seq: double; command: string; data: JsonValue?)
        var inscope ini <- NextArguments(data)
        agent->getContextData(uint64(ini.threadId)) <| $(var ctx: DAContext)
            ctx |> reset_debug_flags()
            ctx.stepRequestedStack = max(stack_depth(*ctx.ctx) - 1, 0)
            ctx.stepInRequested = ctx.stepRequestedStack == 0
        self->sendSuccessResponse(seq, command, null)

    def reqEvaluate(seq: double; command: string; data: JsonValue?)
        var inscope ini <- EvaluateArguments(data)

        var found = false
        let ctxId = uint64(ini.frameId / double(MAX_STACK))
        agent->getContextData(ctxId) <| $(var ctxData: DAContext)
            let frameId = int(ini.frameId % double(MAX_STACK))
            if frameId >= 0 && frameId < length(ctxData.stack)
                let spAddr = ctxData.stack[frameId].spAddr
                var idx = frameId
                var first = true
                while !found && idx < length(ctxData.stack)
                    let line & = unsafe(ctxData.stack[idx])
                    if first || line.spAddr == spAddr
                        found = line |> find_child_var() <| $[unused_argument(varIdx)](child, varIdx)
                            if child.name == ini.expression
                                var inscope res = [[EvaluateResponse
                                    result=child.value,
                                    _type=child._type,
                                    indexedVariables=double(child.indexedVariables),
                                    variablesReference=child.children != null ? double((ctxId * MAX_STACK + uint64(idx)) * MAX_VARIABLES + child.uid) : 0lf]]
                                self->sendSuccessResponseStr(seq, command, sprint_json(res, false))
                                return true
                            return false
                        first = false
                    idx ++
                if !found
                    for i in range(get_total_variables(*ctxData.ctx))
                        let vinfo & = unsafe(get_variable_info(ctxData.ctx, i))
                        if vinfo.name == ini.expression
                            unsafe
                                agent.walker.ctx = addr(ctxData)
                            agent.walker->collectGlobalAt(vinfo, ctxData.stack[frameId])
                            found = ctxData.stack[frameId] |> find_child_var() <| $[unused_argument(varIdx)](child, varIdx)
                                if child.name == ini.expression
                                    var inscope res = [[EvaluateResponse
                                        result=child.value,
                                        _type=child._type,
                                        indexedVariables=double(child.indexedVariables),
                                        variablesReference=child.children != null ? double((ctxId * MAX_STACK + uint64(frameId)) * MAX_VARIABLES + child.uid) : 0lf]]
                                    self->sendSuccessResponseStr(seq, command, sprint_json(res, false))
                                    return true
                                return false
                            break
                if !found
                    var inscope vars : table<string; Result>
                    var frame = frameId
                    let stackLen = length(ctxData.stack)
                    // collect all vars from all blocks till the function frame
                    while frame < stackLen && ctxData.stack[frame].isBlock
                        frame += 1
                    if frame >= stackLen
                        frame = stackLen - 1
                    while frame >= frameId
                        ctxData.stack[frame] |> iter_top_child_var() <| $(child)
                            if child.typeInfo != null
                                let tInfo & = unsafe(*child.typeInfo)
                                vars[child.name] <- [[Result tinfo=tInfo, value=child.rawValue, data=unsafe(reinterpret<void?>child.address)]]
                        --frame

                    var result = debug_eval(vars, ini.expression)
                    var resStr = ""
                    if !empty(result.error)
                        resStr = result.error
                    elif result.data != null
                        resStr = sprint_data(result.data, unsafe(addr(result.tinfo)), print_flags_debug)
                    else
                        resStr = sprint_data(result.value, unsafe(addr(result.tinfo)), print_flags_debug)
                    var inscope res = [[EvaluateResponse
                        result=resStr,
                        _type=!empty(result.error) ? describe(result.tinfo) : "",
                        indexedVariables=0lf,
                        variablesReference=0lf]]
                    self->sendSuccessResponseStr(seq, command, sprint_json(res, false))
        if !found
            self->sendSuccessResponse(seq, command, null)

    def log(msg: string)
        // print("{msg}\n")
        if self->is_connected()
            self->sendOutput("console", "{msg}\n")

    def sendOutput(category: string; output: string)
        var inscope res = [[OutputEventBody category=category, output=output ]]
        self->sendRawData(JV({{"type"=>JV("event"); "event"=>JV("output"); "body"=>JV(res)}}), /*log*/false)

    def sendEvent(event: string; body: JsonValue?)
        self->sendRawData(JV({{"type"=>JV("event"); "event"=>JV(event); "body"=>body}}), /*log*/true)

    def sendSuccessResponse(id: double; command: string; body: JsonValue?)
        self->sendRawData(JV({{"request_seq"=>JV(id); "type"=>JV("response");
                                "body"=>body; "command"=>JV(command); "success"=>JV(true)}}), /*log*/true)

    def sendSuccessResponseStr(id: double; command: string; body: string)
        // self->sendRawData(JV({{"request_seq"=>JV(id); "type"=>JV("response");
        //                         "body"=>body; "command"=>JV(command); "success"=>JV(true)}}), /*log*/true)
        self->sendRawDataStr("\{\"request_seq\":{id}, \"type\":\"response\", \"body\":{body}, \"command\":\"{command}\", \"success\":true\}", /*log*/true)

    // https://microsoft.github.io/language-server-protocol/specifications/specification-current/#responseMessage
    def sendRawData(var data : JsonValue?; logData: bool)
        let msg = write_json(data)
        var packStr = "Content-Length: {length(msg)}\r\n\r\n{msg}"
        if logData
            self->log("<------\n{packStr}")
        // else
        //     print("<------\n{packStr}\n")
        unsafe
            self->send(reinterpret<uint8?> packStr, length(packStr))
            delete data

    def sendRawDataStr(msg : string; logData: bool)
        var packStr = "Content-Length: {length(msg)}\r\n\r\n{msg}"
        if logData
            self->log("<------\n{packStr}")
        // else
        //     print("<------\n{packStr}\n")
        unsafe
            self->send(reinterpret<uint8?> packStr, length(packStr))

    def sendStopped(ctx: DAContext; reason, text: string; brId: uint64)
        self->sendEvent("stopped", JV({{
            "reason"=>JV(reason);
            "text"=>empty(text) ? JVNull() : JV(text);
            "threadId"=>JV(double(ctx.id));
            "allThreadsStopped"=>JV(false);
            "hitBreakpointIds"=>brId == -1ul ? null : JV([{auto[] JV(brId)}])
        }}))



class private EvalWalker : DapiStackWalker
    W : StringBuilderWriter?

    def EvalWalker(var wr: StringBuilderWriter)
        W = unsafe(addr(wr))

    def override onArgument(info: FuncInfo; index: int; vinfo: VarInfo; arg: float4): void
        if vinfo.flags.refType
            let parg = unsafe(reinterpret<void?> arg)
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} & = * reinterpret<{describe(type_info(vinfo))}?> {parg}ul // ref argument\n")
        elif vinfo.dimSize > 0u
            let parg = unsafe(reinterpret<void?> arg)
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(vinfo))}?> {parg}ul // ref argument\n")
        else
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = ")
            if vinfo.basicType==Type tString
                *W |> write("\"")
                *W |> write(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
                *W |> write("\"")
            else
                *W |> write(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
            *W |> write("\n")

    def override onVariable(inf: FuncInfo; vinfo: LocalVariableInfo; arg: void?; inScope: bool): void
        if inScope
            if arg != null
                if vinfo.flags.ref
                    unsafe
                        let pref = unsafe(reinterpret<void??> arg)
                        var cvinfo = vinfo
                        cvinfo.flags &= ~ TypeInfoFlags ref
                        *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(cvinfo))}?> {*pref}ul // already ref\n")
                elif vinfo.dimSize > 0u
                    *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(vinfo))}?> {arg}ul\n")
                else
                    *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} & = * reinterpret<{describe(type_info(vinfo))}?> {arg}ul\n")


def private tick_debugger()
    tick_debug_agent("~debug")
    sleep(10u)

var private g```dAgent : DAgent?

[apply_in_context(name="~debug")]
def wait_for_debugger() : bool
    while (g```dAgent == null || g```dAgent.server == null
       || (g```dAgent.waitConnection && (!g```dAgent.server.configurationDone || !g```dAgent.server.threadsDone)))
        sleep(10u)
    return true

[unused_argument(ctx)]
def private start_debug_agent(var ctx: Context)
    g```dAgent = new DAgent()
    g```dAgent->initAgent(ctx)
    install_new_debug_agent(g```dAgent, "~debug")

def private start_agent(force:bool = false)
    if force || !has_debug_agent_context("~debug")
        fork_debug_agent_context(@@start_debug_agent)
        print(use_instruments() ? "dap: install debug agent (with instruments)\n" : "dap: install debug agent\n")
    else
        print("dap: debug agent already exists\n")
    set_single_step(this_context(), !use_instruments() || wait_debugger())
    return true

[init]
def init_stepping_debugger()
    if (use_instruments() || !wait_debugger() || (is_in_aot() && loads_itself()) ||
        is_in_debug_agent_creation() || !is_compiling_macros_in_module("debug") || is_in_completion())
        return
    set_single_step(this_context(), true)

[_macro]
def private auto_start_debug()
    if (is_in_aot() && loads_itself()) || is_in_debug_agent_creation() || !is_compiling_macros_in_module("debug") || is_in_completion()
        return
    this_context().category |= context_category_flags debugger_attached
    set_single_step(this_context(), !use_instruments() || wait_debugger())
    if !has_debug_agent_context("~debug")
        if start_agent()
            new_thread <| @
                this_context().category &= ~context_category_flags debugger_attached
                this_context().category |= context_category_flags debugger_tick
                set_single_step(this_context(), false)
                this_context().name := "tick thread"
                while !is_job_que_shutting_down()
                    tick_debugger()
            if wait_debugger()
                wait_for_debugger()


def private use_instruments() // or stepping debugger
    let args <- get_command_line_arguments()
    return args |> find_index("--das-stepping-debugger") < 0


def private wait_debugger(): bool
    let args <- get_command_line_arguments()
    return args |> find_index("--das-wait-debugger") >= 0


def private loads_itself(): bool
    let args <- get_command_line_arguments()
    for i in range(1, length(args))
        if args[i] |> ends_with("daslib\\debug.das") || args[i] |> ends_with("daslib/debug.das")
            return true
    return false

def private sprint_data_fast(data:void?|float4; info:TypeInfo const?; flags:print_flags): string
    if info.basicType==Type tArray || info.basicType==Type tTable
        return ""
    return sprint_data(data, info, flags)
