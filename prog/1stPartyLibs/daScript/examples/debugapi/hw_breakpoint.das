// Hardware Breakpoints — Data Watchpoints
//
// Demonstrates `set_hw_breakpoint` and `clear_hw_breakpoint` —
// hardware data breakpoints that fire when a memory location is
// modified.  This is a powerful debugging tool for tracking down
// unexpected writes to variables.
//
// How it works:
//   - `set_hw_breakpoint(ctx, addr, size, writeOnly)` installs
//     a hardware breakpoint on `size` bytes starting at `addr`.
//     `writeOnly=true` triggers only on writes (not reads).
//     Returns a breakpoint handle (int).
//   - When the watched memory is written to, the debug agent's
//     `onBreakpoint` is called at the NEXT line after the write
//     (the write has already happened by then).
//   - `clear_hw_breakpoint(handle)` removes the breakpoint.
//
// Requirements:
//   - `options debugger = true` — enables the debug infrastructure
//   - `require daslib/debug` — provides the debug agent that
//     handles breakpoint events (or write your own agent)
//   - A debug agent must be installed to receive breakpoint events
//
// Use cases:
//   - Finding who modifies a variable unexpectedly
//   - Tracking state corruption in complex systems
//   - Implementing conditional data breakpoints in custom debuggers
//
// Run: daslang.exe examples/debugapi/hw_breakpoint.das

options gen2
options debugger = true

require daslib/rtti
require daslib/debugger

// ============================================================
// A debug agent that reports hardware breakpoint hits
// ============================================================

class BreakpointAgent : DapiDebugAgent {
    hit_count : int = 0
    last_line : int = 0

    def override onBreakpoint(var ctx : Context; at : LineInfo;
                              reason, text : string) : void {
        hit_count++
        last_line = int(at.line)
        print("  [breakpoint #{hit_count}] {reason} at line {int(at.line)}: {text}\n")
    }
}

[export]
def install_bp_agent(ctx : Context) {
    assert(this_context().category.debug_context)
    install_new_debug_agent(new BreakpointAgent(), "bp_agent")
}

// ============================================================
// Example 1: Watching a single integer variable
// ============================================================

def demo_watch_int() {
    print("=== hw breakpoint: watching an int ===\n")

    var target = 10
    print("  initial target = {target}\n")

    // Set a hardware breakpoint on 4 bytes (sizeof int) at &target
    var bp = 0
    unsafe {
        bp = set_hw_breakpoint(this_context(), addr(target), 4, true)
    }
    print("  breakpoint installed (handle={bp})\n")

    // Each write to `target` triggers the breakpoint.
    // The agent's onBreakpoint fires on the NEXT line after
    // the write — by then the new value is already stored.
    target = 20
    print("  after write: target = {target}\n")

    target = 30
    print("  after write: target = {target}\n")

    target = 40
    print("  after write: target = {target}\n")

    // Clear the breakpoint — further writes are silent
    unsafe {
        clear_hw_breakpoint(bp)
    }
    print("  breakpoint cleared\n")

    target = 50
    print("  after cleared write: target = {target} (no breakpoint)\n")
}

// ============================================================
// Example 2: Watching a struct field
// ============================================================

struct Position {
    x : float
    y : float
}

def demo_watch_struct_field() {
    print("\n=== hw breakpoint: watching a struct field ===\n")

    var pos = Position(x = 0.0, y = 0.0)

    // Watch only the `y` field — 4 bytes at offset of `y`
    var bp = 0
    unsafe {
        bp = set_hw_breakpoint(this_context(), addr(pos.y), 4, true)
    }
    print("  watching pos.y\n")

    // Writing to pos.x does NOT trigger the breakpoint
    pos.x = 1.0
    print("  pos.x = {pos.x} (no trigger)\n")

    // Writing to pos.y DOES trigger it
    pos.y = 2.0
    print("  pos.y = {pos.y} (triggered)\n")

    pos.y = 3.0
    print("  pos.y = {pos.y} (triggered again)\n")

    unsafe {
        clear_hw_breakpoint(bp)
    }
    print("  breakpoint cleared\n")
}

// ============================================================
// Example 3: Scoped breakpoint pattern
// ============================================================
// A helper that installs a breakpoint for a block scope and
// automatically clears it when the block exits.

def watch_variable(var ctx : Context; data : void?; size : int; blk : block) {
    var bp = 0
    unsafe {
        bp = set_hw_breakpoint(ctx, data, size, true)
    }
    invoke(blk)
    unsafe {
        clear_hw_breakpoint(bp)
    }
}

def demo_scoped_breakpoint() {
    print("\n=== hw breakpoint: scoped pattern ===\n")

    var value = 100

    unsafe {
        watch_variable(this_context(), addr(value), 4) {
            value = 200
            print("  inside scope: value = {value}\n")
            value = 300
            print("  inside scope: value = {value}\n")
        }
    }

    // After the scope, no breakpoint — writes are silent
    value = 400
    print("  outside scope: value = {value} (no trigger)\n")
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    // Install the debug agent first
    fork_debug_agent_context(@@install_bp_agent)

    demo_watch_int()
    demo_watch_struct_field()
    demo_scoped_breakpoint()

    delete_debug_agent_context("bp_agent")
}
