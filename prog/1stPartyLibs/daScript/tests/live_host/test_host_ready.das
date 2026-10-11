options gen2
options no_aot
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/fio
require daslib/command_line
require daslib/strings_boost
require _live_host_spawn
require strings

//! daslang-live serves its HTTP API on the port its argv names. The host reads the LiveHost
//! shared module's lifecycle exports before any require names that module, so a module scan
//! that defers it has to bring it in first - otherwise live mode never turns on, no server
//! starts, and every client waits out its ready budget.

[test]
def test_host_serves_status(t : T?) {
    let exe = live_exe()
    if (empty(exe)) {
        t |> skip("no daslang-live beside {get_das_exe()}")
        return
    }
    if (free_port(PORT_BAND_READY) < 0) {
        t |> skip("a host answers on every port from {PORT_BAND_READY} to {PORT_BAND_READY + PORT_SPAN - 1}")
        return
    }
    t |> run("/status answers within the ready budget, and /shutdown ends the host with 0") @(t : T?) {
        let script = path_join(get_das_root(), "tests/live_host/_fixture_host_ready.das")
        let no_args : array<string>
        var lines : array<string>
        let spawned = with_ready_host(exe, script, PORT_BAND_READY, no_args, lines) $(var host; port; ready) {
            t |> success(ready >= 0.0, "daslang-live did not serve /status within 30 s:\n{join(lines, "\n")}")
            if (ready >= 0.0) {
                post_shutdown(port)
                let rc = wait_exit_draining(host, 20.0, lines)
                t |> equal(rc, 0, "the host exited with 0 after /shutdown:\n{join(lines, "\n")}")
            }
        }
        t |> success(spawned, "no port to spawn on:\n{join(lines, "\n")}")
    }
}
