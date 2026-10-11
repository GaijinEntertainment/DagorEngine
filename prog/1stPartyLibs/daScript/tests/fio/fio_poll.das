options gen2
options no_aot

require dastest/testing_boost public
require math
require daslib/fio
require daslib/strings_boost

//! `fpoll` on a pipe: false while the child is silent, true once it wrote, true at its EOF;
//! `funbuffered` on the same stream is a no-op the read survives; a regular file is always ready

def das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}

//! a deadline on a child, never a claim about speed: a Debug host compiles what the child
//! requires several times slower
def guard(seconds : float) : float {
    return shared_module_extension() == "_debug.shared_module" ? min(seconds * 3.0, 300.0) : seconds
}

[test]
def test_fpoll_pipe(t : T?) {
    t |> run("a silent child polls false, a written line polls true, the EOF polls true") @(t : T?) {
        let tmp = create_temp_directory_result("das fio poll ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root_dir = tmp as value
        let release = path_join(root_dir, "release")
        let fixture = path_join(get_das_root(), "tests/fio/_fixture_poll_child.das")
        let argv <- [das_exe(), "-dasroot", get_das_root(), fixture, "--", release]
        var line = ""
        var silent = false
        var ready = false
        var at_eof = false
        let rc = unsafe(popen_argv(argv, 0.0, $(f) {
            if (f == null) return
            funbuffered(f)
            silent = !fpoll(f, 200)
            fwrite(release, "go")
            ready = fpoll(f, int(guard(30.0) * 1000.0))
            line = fgets(f)
            at_eof = fpoll(f, int(guard(30.0) * 1000.0))
            let tail = fgets(f)
            at_eof = at_eof && empty(tail)
        }))
        t |> equal(rc, 0, "child exited 0")
        t |> success(silent, "nothing to read while the child waits on the release file")
        t |> success(ready, "readable once the child wrote its line")
        t |> equal(rtrim(line, "\r\n"), "polled", "the line the child wrote")
        t |> success(at_eof, "a closed pipe polls true and reads empty")
        let cleanup = rmdir_rec_result(root_dir)
        t |> success(cleanup is value && cleanup as value, "temp dir removed")
    }
}

[test]
def test_fpoll_regular_file(t : T?) {
    t |> run("a regular file is always readable") @(t : T?) {
        var terr : string
        let tmp = temp_directory(terr)
        let fname = path_join(empty(tmp) ? "." : tmp, "fio_poll_{ref_time_ticks()}.txt")
        t |> success(fwrite(fname, "x"), "the probe file is written")
        var ready = false
        fopen(fname, "rb") $(f) {
            if (f != null) {
                funbuffered(f)
                ready = fpoll(f, 0)
            }
        }
        t |> success(ready, "fpoll reports a regular file readable with a zero timeout")
        t |> success(remove(fname))
    }
}
