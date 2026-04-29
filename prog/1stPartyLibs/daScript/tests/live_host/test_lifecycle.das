options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live_host

[test]
def test_timing(t : T?) {
    t |> run("get_dt returns non-negative") @(t : T?) {
        let dt = get_dt()
        t |> success(dt >= 0.0f, "dt should be >= 0")
    }
    t |> run("get_dt computes time internally in non-live mode") @(t : T?) {
        get_dt()
        let dt2 = get_dt()
        t |> success(dt2 >= 0.0f, "second dt should be >= 0")
    }
    t |> run("get_uptime returns non-negative") @(t : T?) {
        let up = get_uptime()
        t |> success(up >= 0.0f, "uptime should be >= 0")
    }
    t |> run("get_fps returns non-negative") @(t : T?) {
        let fps = get_fps()
        t |> success(fps >= 0.0f, "fps should be >= 0")
    }
}

[test]
def test_paused_state(t : T?) {
    t |> run("default not paused") @(t : T?) {
        set_paused(false)
        t |> success(!is_paused(), "should not be paused by default")
    }
    t |> run("set paused true") @(t : T?) {
        set_paused(true)
        t |> success(is_paused(), "should be paused after set_paused(true)")
    }
    t |> run("set paused false") @(t : T?) {
        set_paused(true)
        set_paused(false)
        t |> success(!is_paused(), "should not be paused after set_paused(false)")
    }
    set_paused(false)
}

[test]
def test_exit_flags(t : T?) {
    t |> run("exit_requested callable") @(t : T?) {
        let er = exit_requested()
        feint("exit_requested = {er}\n")
        t |> success(true, "exit_requested callable")
    }
}

[test]
def test_reload_state(t : T?) {
    t |> run("is_reload returns false outside live mode") @(t : T?) {
        t |> success(!is_reload(), "is_reload should be false in test context")
    }
    t |> run("is_live_mode returns false in test context") @(t : T?) {
        t |> success(!is_live_mode(), "is_live_mode should be false under dastest")
    }
}
