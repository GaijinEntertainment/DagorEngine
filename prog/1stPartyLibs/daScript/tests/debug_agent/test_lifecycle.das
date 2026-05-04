// Test: debug agent lifecycle — create, check, context access
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger

class LifecycleAgent : DapiDebugAgent {
    tag : int = 999
}

var lifecycle_agent_installed : bool = false

def setup_lifecycle_agent(ctx : Context) {
    install_new_debug_agent(new LifecycleAgent(), "lifecycle")
    lifecycle_agent_installed = true
}

def ensure_lifecycle() {
    if (!lifecycle_agent_installed) {
        fork_debug_agent_context(@@setup_lifecycle_agent)
    }
}

[export, pinvoke]
def lifecycle_noop() {
    pass
}

[test]
def test_has_agent_before_install(t : T?) {
    t |> run("has_debug_agent_context false before install") @(t : T?) {
        t |> equal(has_debug_agent_context("nonexistent_agent_xyz"), false)
    }
}

[test]
def test_has_agent_after_install(t : T?) {
    t |> run("has_debug_agent_context true after install") @(t : T?) {
        ensure_lifecycle()
        t |> equal(has_debug_agent_context("lifecycle"), true)
    }
}

[test]
def test_get_agent_context_callable(t : T?) {
    t |> run("get_debug_agent_context returns callable context") @(t : T?) {
        ensure_lifecycle()
        // If this doesn't throw, the context is valid
        unsafe {
            invoke_in_context(get_debug_agent_context("lifecycle"), "lifecycle_noop")
        }
        t |> equal(true, true)
    }
}

[test]
def test_fork_idempotent(t : T?) {
    t |> run("re-installing same agent name does not crash") @(t : T?) {
        ensure_lifecycle()
        fork_debug_agent_context(@@setup_lifecycle_agent)
        t |> equal(has_debug_agent_context("lifecycle"), true)
    }
}

[test]
def test_delete_agent(t : T?) {
    t |> run("delete_debug_agent_context removes agent by name") @(t : T?) {
        ensure_lifecycle()
        t |> equal(has_debug_agent_context("lifecycle"), true)
        delete_debug_agent_context("lifecycle")
        t |> equal(has_debug_agent_context("lifecycle"), false)
        lifecycle_agent_installed = false
    }
}
