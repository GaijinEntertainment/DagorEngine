// Instrumentation — Line-Level and Function-Level Hooks
//
// Demonstrates two instrumentation mechanisms:
//
//   1. Line-level instrumentation (`instrument_node`)
//      Marks specific source lines to trigger `onInstrument` and
//      optionally `onSingleStep` in the debug agent.  Use this
//      for conditional breakpoints, coverage tracking, or tracing
//      specific code regions.
//
//   2. Function-level instrumentation (`instrument_all_functions`)
//      Registers a callback for every function entry and exit via
//      `onInstrumentFunction`.  Each function gets a user-defined
//      `userData` value (e.g. an index for profiling).  Use this
//      for call graphs, profiling, or entrance/exit logging.
//
// Both mechanisms require a debug agent to receive callbacks.
//
// Key API:
//   instrument_node(ctx, enable) <| $(at) { return should_instrument }
//     — filter which lines to instrument; return true to mark a line
//   set_single_step(ctx, enable)
//     — enable/disable single-stepping after a breakpoint/instrument hit
//   instrument_all_functions(ctx) <| $(fn, sfn) { return userData }
//     — instrument all functions; return per-function userData
//   onInstrument(ctx, at)
//     — called when execution reaches an instrumented line
//   onSingleStep(ctx, at)
//     — called on every line when single-stepping is active
//   onInstrumentFunction(ctx, fun, entering, userData)
//     — called on function enter (entering=true) and exit (entering=false)
//
// Run: daslang.exe examples/debugapi/instrumentation.das

options gen2
options debugger = true

require daslib/rtti
require daslib/debugger
require strings
require math

// ============================================================
// Section 1: Line-level instrumentation
// ============================================================
// This agent demonstrates selective line instrumentation.
// When a breakpoint fires, it instruments a specific line
// ahead in the same file.  When execution reaches that line,
// onInstrument fires.  We then enable single-stepping to
// trace a few lines.

class LineInstrumentAgent : DapiDebugAgent {
    steps_remaining : int = 0

    def override onBreakpoint(var ctx : Context; at : LineInfo;
                              reason, text : string) : void {
        print("  [breakpoint] line {int(at.line)}\n")

        // Instrument lines within the next 5 lines of the same file
        let target_start = at.line + 2u
        let target_end = at.line + 5u
        instrument_node(ctx, true) <| $(line_at) {
            if (line_at.fileInfo == at.fileInfo) {
                if (line_at.line >= target_start && line_at.line <= target_end) {
                    return true
                }
            }
            return false
        }
    }

    def override onInstrument(var ctx : Context; at : LineInfo) : void {
        print("  [instrument] line {int(at.line)} -- enabling single-step\n")
        set_single_step(ctx, true)
        steps_remaining = 3
    }

    def override onSingleStep(var ctx : Context; at : LineInfo) : void {
        print("  [step] line {int(at.line)}\n")
        steps_remaining--
        if (steps_remaining <= 0) {
            set_single_step(ctx, false)
            print("  [step] stopped single-stepping\n")
        }
    }
}

[export]
def install_line_agent(ctx : Context) {
    install_new_debug_agent(new LineInstrumentAgent(), "line_inst")
}

def target_function(n : int) : int {
    var result = 0
    for (i in range(n)) {
        result += i
    }
    result *= 2
    result += 1
    return result
}

def demo_line_instrumentation() {
    print("=== line-level instrumentation ===\n")

    fork_debug_agent_context(@@install_line_agent)

    // The breakpoint triggers our agent, which instruments
    // lines ahead of it, and then single-steps a few lines.
    breakpoint()
    print("  line after breakpoint\n")
    let r = target_function(5)
    print("  result = {r}\n")
    print("  done with line instrumentation\n")

    delete_debug_agent_context("line_inst")
}

// ============================================================
// Section 2: Function-level instrumentation
// ============================================================
// This agent is notified on every function enter and exit.
// Each function gets a unique userData (an index assigned at
// instrumentation time).  This enables:
//   - Call graph construction
//   - Function-level profiling (measure enter-to-exit time)
//   - Call counting

class FunctionProfileAgent : DapiDebugAgent {
    indent : int = 0
    call_counts : array<int>

    def override onInstrumentFunction(var ctx : Context; fun : SimFunction?;
                                      entering : bool; userData : uint64) : void {
        let idx = int(userData)

        // Grow the counts array if needed
        while (length(call_counts) <= idx) {
            call_counts |> push(0)
        }

        if (entering) {
            call_counts[idx]++
            let pad = build_string() $(var writer) {
                for (_ in range(indent)) {
                    write(writer, "  ")
                }
            }
            print("  {pad}-> {fun.mangledName} (call #{call_counts[idx]})\n")
            indent++
        } else {
            indent = max(0, indent - 1)
            let pad = build_string() $(var writer) {
                for (_ in range(indent)) {
                    write(writer, "  ")
                }
            }
            print("  {pad}<- {fun.mangledName}\n")
        }
    }
}

[export]
def install_func_agent(ctx : Context) {
    install_new_debug_agent(new FunctionProfileAgent(), "func_prof")
}

// Application functions to instrument

def compute_sum(n : int) : int {
    var s = 0
    for (i in range(n)) {
        s += i
    }
    return s
}

def compute_product(a, b : int) : int {
    return a * b
}

def do_work() {
    let s = compute_sum(10)
    let p = compute_product(s, 2)
    print("  work result = {p}\n")
}

def demo_function_instrumentation() {
    print("\n=== function-level instrumentation ===\n")

    // Instrument all functions BEFORE forking the agent, so
    // the agent receives onInstrumentFunction callbacks.
    var func_index = 0
    instrument_all_functions(this_context()) <| $(fn, sfn) {
        let idx = func_index
        func_index++
        return uint64(idx)
    }
    print("  instrumented {func_index} functions\n")

    fork_debug_agent_context(@@install_func_agent)

    do_work()
    do_work()

    delete_debug_agent_context("func_prof")
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_line_instrumentation()
    demo_function_instrumentation()
}
