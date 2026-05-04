options gen2
options rtti
options no_aot
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module dap shared

//! Debug Adapter Protocol (DAP) data structures.
//!
//! Defines request, response, and event types for the
//! `DAP specification <https://microsoft.github.io/debug-adapter-protocol/specification>`_.
//! Used by the ``debug`` module to communicate with DAP clients.

require daslib/json
require daslib/json_boost
require daslib/strings_boost


// https://microsoft.github.io/debug-adapter-protocol/specification#Requests_Initialize
struct InitializeRequestArguments {
    //! Arguments for the DAP initialize request.
}

def InitializeRequestArguments(data : JsonValue?) {
    //! Constructs an InitializeRequestArguments from a JSON value.
    return InitializeRequestArguments()
}


struct DisconnectArguments {
    //! Arguments for the DAP disconnect request.
    restart : bool //! Whether to restart the debuggee after disconnecting.
    terminateDebuggee : bool //! Whether to terminate the debuggee when disconnecting.
    suspendDebuggee : bool //! Whether to suspend the debuggee when disconnecting.
}

def DisconnectArguments(data : JsonValue?) {
    //! Constructs a DisconnectArguments from a JSON value.
    return DisconnectArguments(restart = job(data, "restart"),
                                 terminateDebuggee = job(data, "terminateDebuggee"),
                                 suspendDebuggee = job(data, "suspendDebuggee")
            )
}


struct Capabilities {
    //! Debugger capabilities reported in the initialize response.
    supportsConfigurationDoneRequest : bool //! Whether the adapter supports the configurationDone request.
    supportsRestartRequest : bool //! Whether the adapter supports the restart request.
    supportTerminateDebuggee : bool //! Whether the adapter supports terminating the debuggee.
    supportsTerminateRequest : bool //! Whether the adapter supports the terminate request.
    supportsExceptionOptions : bool //! Whether the adapter supports exception options.
    supportsExceptionFilterOptions : bool //! Whether the adapter supports exception filter options.
    supportsDelayedStackTraceLoading : bool //! Whether the adapter supports delayed stack trace loading.
    supportsDataBreakpoints : bool //! Whether the adapter supports data breakpoints.
}



struct DataBreakpoint {
    //! A data breakpoint that triggers on memory access.
    dataId : string //! Identifier for the data to watch.
    accessType : string //! Access type that triggers the breakpoint: read, write, or readWrite.
    condition : string //! Optional expression condition for the breakpoint.
    hitCondition : string //! Optional hit count condition for the breakpoint.
    description : string //! Human-readable description of the breakpoint.
    enabled : bool //! Whether the breakpoint is enabled.
}

def DataBreakpoint(data : JsonValue?) {
    //! Constructs a DataBreakpoint from a JSON value.
    return DataBreakpoint(dataId = jos(data, "dataId"),
                            accessType = jos(data, "accessType"),
                            condition = jos(data, "condition"),
                            hitCondition = jos(data, "hitCondition"),
                            description = jos(data, "description"),
                            enabled = job(data, "enabled", true)
            )
}

struct SetDataBreakpointsArguments {
    //! Arguments for the setDataBreakpoints request.
    breakpoints : array<DataBreakpoint> //! Array of data breakpoints to set.
}

def SetDataBreakpointsArguments(data : JsonValue?) {
    //! Constructs a SetDataBreakpointsArguments from a JSON value, parsing the breakpoints array.
    var res <- SetDataBreakpointsArguments()
    var breakpoints = joj(data, "breakpoints")
    if (breakpoints != null) {
        res.breakpoints |> reserve(length(breakpoints.value as _array))
        for (it in breakpoints.value as _array) {
            res.breakpoints |> emplace(DataBreakpoint(it))
        }
    }
    return <- res
}

struct DataBreakpointInfoArguments {
    //! Arguments for the dataBreakpointInfo request.
    variablesReference : double //! Reference to the variable container.
    name : string //! Name of the variable.
}

def DataBreakpointInfoArguments(data : JsonValue?) {
    //! Constructs a DataBreakpointInfoArguments from a JSON value.
    return DataBreakpointInfoArguments(
                variablesReference = jon(data, "variablesReference"),
                name = jos(data, "name")
            )
}
struct DataBreakpointInfoResponse {
    //! Response body for the dataBreakpointInfo request.
    dataId : string //! Identifier for the data, used when setting a data breakpoint.
    description : string //! Human-readable description of the data.
    //accessTypes: DataBreakpointAccessType[];
}


struct SourceBreakpoint {
    //! A breakpoint specified by source location line number.
    line : double //! Line number of the breakpoint.
}

def SourceBreakpoint(data : JsonValue?) {
    //! Constructs a SourceBreakpoint from a JSON value.
    return SourceBreakpoint(line = jon(data, "line"))
}

struct Source {
    //! A source file descriptor with name and path.
    name : string //! Short name of the source.
    path : string //! Full file-system path of the source.
}

def Source(data : JsonValue?) {
    //! Constructs a Source from a JSON value.
    return Source(name = jos(data, "name"), path = jos(data, "path"))
}


struct SetBreakpointsArguments {
    //! Arguments for the setBreakpoints request.
    source : Source //! Source file for which breakpoints are set.
    breakpoints : array<SourceBreakpoint> //! Array of source breakpoints to set.
    sourceModified : bool //! Whether the source has been modified since last build.
}

def SetBreakpointsArguments(data : JsonValue?) {
    //! Constructs a SetBreakpointsArguments from a JSON value, parsing source and breakpoints.
    var res <- SetBreakpointsArguments(source = Source(joj(data, "source")),
                                         sourceModified = job(data, "sourceModified"))
    var breakpoints = joj(data, "breakpoints")
    if (breakpoints != null) {
        res.breakpoints |> reserve(length(breakpoints.value as _array))
        for (it in breakpoints.value as _array) {
            res.breakpoints |> emplace(SourceBreakpoint(it))
        }
    }
    return <- res
}


struct Breakpoint {
    //! A breakpoint with verification status and location.
    id : double //! Unique identifier for the breakpoint.
    verified : bool //! Whether the breakpoint has been verified by the debugger.
    source : Source //! Source file containing the breakpoint.
    line : double //! Actual line number of the breakpoint.
    message : string //! Optional message about the breakpoint state.
}

struct SetBreakpointsResponse {
    //! Response body for the setBreakpoints request.
    breakpoints : array<Breakpoint> //! Array of breakpoints with their verification status.
}



struct Thread {
    //! A thread with an identifier and name.
    id : double //! Unique identifier for the thread.
    name : string //! Human-readable name of the thread.
}


struct ThreadsResponseBody {
    //! Response body for the threads request.
    threads : array<Thread> //! Array of threads.
}


struct StackTraceArguments {
    //! Arguments for the stackTrace request.
    threadId : double //! Thread for which to retrieve the stack trace.
    startFrame : double //! Index of the first frame to return.
    levels : double //! Maximum number of frames to return.
}

def StackTraceArguments(data : JsonValue?) {
    //! Constructs a StackTraceArguments from a JSON value.
    return <- StackTraceArguments(threadId = jon(data, "threadId"), startFrame = jon(data, "startFrame"), levels = jon(data, "levels"))
}
struct StackFrame {
    //! A stack frame with source location and identifier.
    id : double //! Unique identifier for the stack frame.
    name : string //! Name of the frame, typically the function name.
    source : Source //! Source file of the frame.
    line : double //! Line number in the source file.
    column : double //! Column number in the source file.
}
struct StackTraceResponseBody {
    //! Response body for the stackTrace request.
    stackFrames : array<StackFrame> //! Array of stack frames.
    totalFrames : double //! Total number of frames available.
}


struct ScopesArguments {
    //! Arguments for the scopes request.
    frameId : double //! Stack frame for which to retrieve scopes.
}

def ScopesArguments(data : JsonValue?) {
    //! Constructs a ScopesArguments from a JSON value.
    return <- ScopesArguments(frameId = jon(data, "frameId"))
}


struct Scope {
    //! A named variable scope with a variables reference.
    name : string //! Name of the scope (e.g. Locals, Arguments).
    variablesReference : double //! Reference used to retrieve the variables of this scope.
}

struct ScopesResponseBody {
    //! Response body for the scopes request.
    scopes : array<Scope> //! Array of scopes for the given frame.
}


struct VariablesArguments {
    //! Arguments for the variables request.
    variablesReference : double //! Reference to the variable container to expand.
    start : double //! Start index of variables to return (for paging).
    count : double //! Number of variables to return (for paging).
}

def VariablesArguments(data : JsonValue?) {
    //! Constructs a VariablesArguments from a JSON value.
    return <- VariablesArguments(
        variablesReference = jon(data, "variablesReference"),
        start = jon(data, "start", -1lf),
        count = jon(data, "count", -1lf)
    )
}


struct Variable {
    //! A variable with name, value, and type information.
    name : string //! Name of the variable.
    value : string //! String representation of the variable's value.
    variablesReference : double //! Reference to child variables, if any.
    @rename _type : string //! Type of the variable.
    indexedVariables : double //! Number of indexed child variables.
}
struct VariablesResponseBody {
    //! Response body for the variables request.
    variables : array<Variable> //! Array of variables.
}


def JV(data : Variable) {
    //! Converts a Variable struct to its DAP JSON representation.
    let valStr = unescape(data.value)
    return JV({
        "name" => JV(data.name),
        "value" => JV(valStr),
        "variablesReference" => JV(data.variablesReference),
        "indexedVariables" => data.indexedVariables > 0lf ? JV(data.indexedVariables) : JVNull(),
        "type" => JV(data._type)
    })
}

struct OutputEventBody {
    //! Body of the output event for debugger console messages.
    category : string //! Category of the output (e.g. console, stdout, stderr).
    output : string //! The output text.
}


struct ContinueArguments {
    //! Arguments for the continue request.
    threadId : double //! Thread to continue.
}

def ContinueArguments(data : JsonValue?) {
    //! Constructs a ContinueArguments from a JSON value.
    return <- ContinueArguments(threadId = jon(data, "threadId"))
}


struct PauseArguments {
    //! Arguments for the pause request.
    threadId : double //! Thread to pause.
}

def PauseArguments(data : JsonValue?) {
    //! Constructs a PauseArguments from a JSON value.
    return <- PauseArguments(threadId = jon(data, "threadId"))
}


struct StepInArguments {
    //! Arguments for the stepIn request.
    threadId : double //! Thread to step into.
}

def StepInArguments(data : JsonValue?) {
    //! Constructs a StepInArguments from a JSON value.
    return <- StepInArguments(threadId = jon(data, "threadId"))
}


struct NextArguments {
    //! Arguments for the next (step over) request.
    threadId : double //! Thread to step over.
}

def NextArguments(data : JsonValue?) {
    //! Constructs a NextArguments from a JSON value.
    return <- NextArguments(threadId = jon(data, "threadId"))
}


struct StepOutArguments {
    //! Arguments for the stepOut request.
    threadId : double //! Thread to step out of.
}

def StepOutArguments(data : JsonValue?) {
    //! Constructs a StepOutArguments from a JSON value.
    return <- StepOutArguments(threadId = jon(data, "threadId"))
}


struct EvaluateArguments {
    //! Arguments for the evaluate request.
    expression : string //! Expression to evaluate.
    frameId : double //! Stack frame in which to evaluate the expression.
    context : string //! Context in which the expression is evaluated (e.g. watch, repl, hover).
}

def EvaluateArguments(data : JsonValue?) {
    //! Constructs an EvaluateArguments from a JSON value.
    return <- EvaluateArguments(frameId = jon(data, "frameId"), expression = jos(data, "expression"), context = jos(data, "context"))
}

struct EvaluateResponse {
    //! Response body for the evaluate request.
    result : string //! String representation of the evaluation result.
    @rename _type : string
    //! Type of the evaluation result.
    variablesReference : double //! Reference to child variables of the result, if any.
    indexedVariables : double //! Number of indexed child variables in the result.
}

def JV(data : EvaluateResponse) {
    //! Converts an EvaluateResponse struct to its DAP JSON representation.
    return JV({
        "result" => JV(data.result),
        "variablesReference" => JV(data.variablesReference),
        "indexedVariables" => data.indexedVariables > 0lf ? JV(data.indexedVariables) : JVNull(),
        "type" => JV(data._type)
    })
}


struct BreakpointEvent {
    //! Event body indicating a breakpoint status change.
    reason : string //! Reason for the event: changed, new, or removed.
    breakpoint : Breakpoint //! The breakpoint whose status changed.
}

struct ThreadEvent {
    //! Event body indicating a thread started or exited.
    reason : string //! Reason for the event: started or exited.
    threadId : double //! Thread identifier.
}


def joj(val : JsonValue?; id : string) : JsonValue? {
    //! Returns a nested JSON object field by name, or null if not found.
    var res : JsonValue? = null
    if (val == null || !(val is _object)) {
        return res
    }
    get(val as _object, id) $(v) {
        res = unsafe(reinterpret<JsonValue?> v)
    }
    return res
}

def jon(val : JsonValue?; id : string; defVal = 0lf) : double {
    //! Returns a numeric JSON field by name, or `defVal` if not found.
    var res = defVal
    if (val == null || !(val is _object)) {
        return res
    }
    get(val.value as _object, id) $(v) {
        if (v.value is _number) {
            res = v.value as _number
        } elif (v.value is _longint) {
            res = double(v.value as _longint)
        }
    }
    return res
}

def j_s(val : JsonValue?; defVal = "") : string {
    //! Returns the string value of a JSON value, or `defVal` if not a string.
    return val?.value ?as _string ?? defVal
}

def jos(val : JsonValue?; id : string; defVal = "") : string {
    //! Returns a string JSON field by name, or `defVal` if not found.
    var res = defVal
    if (val == null || !(val is _object)) {
        return res
    }
    get(val.value as _object, id) $(v) {
        if (v.value is _string) {
            res = v.value as _string
        }
    }
    return res
}

def job(val : JsonValue?; id : string; defVal = false) : bool {
    //! Returns a boolean JSON field by name, or `defVal` if not found.
    var res = defVal
    if (val == null || !(val is _object)) {
        return res
    }
    get(val.value as _object, id) $(v) {
        if (v.value is _bool) {
            res = v.value as _bool
        }
    }
    return res
}
