options gen2
options no_aot
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/fio
require daslib/command_line
require daslib/strings_boost
require daslib/rtti
require _live_host_spawn
require strings

//! The audio system a program created survives a reload, a full reload and a reset under the
//! live host: the program's shutdown() leaves it up (audio_live_keeps_handles), audio_live
//! carries the handles over with the old context's reference, and the next init() pushes
//! through them; the real shutdown finalizes it once. None of it throws, and nothing is left
//! in the job-status census the host prints at exit.

def private lines_mention(lines : array<string>; needle : string) : bool {
    for (line in lines) {
        return true if (find(line, needle) >= 0)
    }
    return false
}

[test]
def test_audio_survives_reloads(t : T?) {
    if (!has_module("audio")) {
        t |> skip("no audio module in this build")
        return
    }
    let exe = live_exe()
    if (empty(exe)) {
        t |> skip("no daslang-live beside {get_das_exe()}")
        return
    }
    if (free_port(PORT_BAND_AUDIO) < 0) {
        t |> skip("a host answers on every port from {PORT_BAND_AUDIO} to {PORT_BAND_AUDIO + PORT_SPAN - 1}")
        return
    }
    t |> run("two reloads, a full reload and a reset keep the audio system up, and /shutdown leaves nothing") @(t : T?) {
        let script = path_join(get_das_root(), "tests/live_host/_fixture_audio_reload.das")
        var lines : array<string>
        let spawned = with_ready_host(exe, script, PORT_BAND_AUDIO, ["--", "--null-audio"], lines) $(var host; port; ready) {
            t |> success(ready >= 0.0, "daslang-live did not serve /status within 30 s:\n{join(lines, "\n")}")
            if (ready >= 0.0) {
                var generation = 0
                for (path in ["/reload", "/reload", "/reload/full", "/reset"]) {
                    generation ++
                    let done = post_and_wait_generation(host, port, path, generation, 30.0, lines)
                    t |> success(done, "{path} did not complete within 30 s:\n{join(lines, "\n")}")
                }
                post_shutdown(port)
                let rc = wait_exit_draining(host, 20.0, lines)
                t |> equal(rc, 0, "the host exited with 0 after /shutdown:\n{join(lines, "\n")}")
                t |> success(!lines_mention(lines, "EXCEPTION"), "a lifecycle call threw:\n{join(lines, "\n")}")
                t |> success(!lines_mention(lines, "leaked JobStatus"), "the host left a stream or status behind:\n{join(lines, "\n")}")
            }
        }
        t |> success(spawned, "no port to spawn on:\n{join(lines, "\n")}")
    }
}
