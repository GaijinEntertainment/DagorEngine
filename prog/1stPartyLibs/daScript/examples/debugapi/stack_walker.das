// Stack Walking — Inspecting the Call Stack at Runtime
//
// Demonstrates `DapiStackWalker` — a debug API class that walks
// the call stack at any point during execution, visiting each
// stack frame and its arguments and local variables.
//
// This is the companion to `DapiDataWalker` (Tutorial 47).
// While DapiDataWalker inspects data values, DapiStackWalker
// inspects the execution context: call frames, arguments, locals.
//
// DapiStackWalker callbacks:
//   onCallAt(pp, info, at)     — stack frame with source location
//   onCall(pp, info)           — stack frame without source info
//   onCallAOT(pp, fileName)   — AOT-compiled frame
//   onCallJIT(pp, fileName)   — JIT-compiled frame
//   onArgument(info, index, vinfo, arg)  — function argument
//   onVariable(info, vinfo, arg, inScope) — local variable
//
// To walk the stack you need a `StackWalker` adapter:
//   1. Create your DapiStackWalker subclass
//   2. Wrap it: `adapter <- make_stack_walker(walker)`
//   3. Call: `walk_stack(adapter, ctx, at)`
//
// Combining with DapiDataWalker:
//   Embed a DapiDataWalker inside the stack walker to print
//   variable values in a custom format.  The `onArgument` and
//   `onVariable` callbacks provide type info and data pointers
//   that can be passed to `walk_data` or `sprint_data`.
//
// Run: daslang.exe examples/debugapi/stack_walker.das

options gen2
options debugger = true
options persistent_heap = true

require daslib/rtti
require daslib/debugger
require strings

// ============================================================
// Section 1: Basic stack walker
// ============================================================
// Prints each call frame and its arguments/variables.

class BasicStackWalker : DapiStackWalker {
    frame_depth : int = 0

    def make_indent() : string {
        return repeat("  ", frame_depth)
    }

    def override onCallAt(pp : Prologue; info : FuncInfo; at : LineInfo) : void {
        let indent = make_indent()
        print("  {indent}[frame {frame_depth}] {info.name} at line {int(at.line)}\n")
        frame_depth++
    }

    def override onCall(pp : Prologue; info : FuncInfo) : void {
        let indent = make_indent()
        print("  {indent}[frame {frame_depth}] {info.name} (no source info)\n")
        frame_depth++
    }

    def override onArgument(info : FuncInfo; index : int; vinfo : VarInfo;
                            arg : float4) : void {
        let indent = make_indent()
        let value = sprint_data(arg, type_info(vinfo), print_flags.singleLine)
        print("  {indent}  arg[{index}] {vinfo.name} = {value}\n")
    }

    def override onVariable(inf : FuncInfo; vinfo : LocalVariableInfo;
                            arg : void?; inScope : bool) : void {
        let indent = make_indent()
        if (inScope) {
            if (arg != null) {
                let value = sprint_data(arg, type_info(vinfo), print_flags.singleLine)
                print("  {indent}  var {vinfo.name} = {value}\n")
            } else {
                print("  {indent}  var {vinfo.name} (optimized out)\n")
            }
        } else {
            print("  {indent}  var {vinfo.name} (out of scope)\n")
        }
    }
}

// ============================================================
// Agent that walks the stack on breakpoint
// ============================================================

class StackWalkAgent : DapiDebugAgent {
    walker_adapter : smart_ptr<StackWalker>
    walker : BasicStackWalker?

    def StackWalkAgent() {
        walker = new BasicStackWalker()
        unsafe {
            walker_adapter <- make_stack_walker(walker)
        }
    }

    def operator delete() {
        unsafe {
            delete walker_adapter
            delete walker
        }
    }

    def override onBreakpoint(var ctx : Context; at : LineInfo;
                              reason, text : string) : void {
        print("  --- stack walk at line {int(at.line)} ---\n")
        walker.frame_depth = 0
        walk_stack(walker_adapter, ctx, at)
        print("  --- end stack walk ---\n")
    }
}

[export]
def install_stack_agent(ctx : Context) {
    assert(this_context().category.debug_context)
    install_new_debug_agent(new StackWalkAgent(), "stack_agent")
}

// ============================================================
// Application code with nested calls
// ============================================================

def inner_function(x : int; y : float) : float {
    var result = float(x) + y
    breakpoint()  // stack walker triggers here
    return result
}

def middle_function(n : int) : float {
    var sum = 0.0
    for (i in range(n)) {
        sum += inner_function(i, 0.5)
    }
    return sum
}

def demo_basic_stack_walk() {
    print("=== basic stack walking ===\n")

    fork_debug_agent_context(@@install_stack_agent)

    let result = middle_function(1)
    print("  result = {result}\n\n")

    delete_debug_agent_context("stack_agent")
}

// ============================================================
// Section 2: State collection with stack walking
// ============================================================
// Combines `onCollect` (state collection) with stack walking.
// The agent collects custom diagnostic data AND walks the stack
// in a single breakpoint/collection event.
//
// `collect_debug_agent_state` triggers `onCollect` on all agents.
// Inside `onCollect`, the agent can:
//   - Walk the stack to see caller info
//   - Report custom variables via `report_context_state`
//   - Build diagnostic objects from runtime state

class DiagnosticAgent : DapiDebugAgent {
    walker_adapter : smart_ptr<StackWalker>
    walker : BasicStackWalker?
    collection_count : int = 0

    def DiagnosticAgent() {
        walker = new BasicStackWalker()
        unsafe {
            walker_adapter <- make_stack_walker(walker)
        }
    }

    def operator delete() {
        unsafe {
            delete walker_adapter
            delete walker
        }
    }

    def override onCollect(var ctx : Context; at : LineInfo) : void {
        collection_count++
        print("  --- diagnostic collection #{collection_count} ---\n")

        // Report custom variables to any listening debug tools
        let depth = stack_depth(ctx)
        unsafe {
            let tinfo = typeinfo rtti_typeinfo(depth)
            report_context_state(ctx, "Diagnostics", "stack_depth",
                unsafe(addr(tinfo)), unsafe(addr(depth)))
            let tinfo2 = typeinfo rtti_typeinfo(collection_count)
            report_context_state(ctx, "Diagnostics", "collection_count",
                unsafe(addr(tinfo2)), unsafe(addr(collection_count)))
        }

        // Walk the stack for a full picture
        walker.frame_depth = 0
        walk_stack(walker_adapter, ctx, at)
        print("  --- end collection ---\n")
    }

    def override onVariable(var ctx : Context; category, name : string;
                            info : TypeInfo; data : void?) : void {
        unsafe {
            let value = sprint_data(data, addr(info), print_flags.singleLine)
            print("  reported {category}: {name} = {value}\n")
        }
    }
}

[export]
def install_diagnostic_agent(ctx : Context) {
    install_new_debug_agent(new DiagnosticAgent(), "diagnostics")
}

def some_nested_work(depth : int) {
    if (depth <= 0) {
        // Trigger state collection at the deepest point
        collect_debug_agent_state(this_context(), get_line_info(1))
        return
    }
    some_nested_work(depth - 1)
}

def demo_diagnostic_collection() {
    print("=== diagnostic collection with stack walking ===\n")

    fork_debug_agent_context(@@install_diagnostic_agent)

    some_nested_work(3)

    delete_debug_agent_context("diagnostics")
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_basic_stack_walk()
    demo_diagnostic_collection()
}
