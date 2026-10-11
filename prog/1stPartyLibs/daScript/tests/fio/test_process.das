options gen2
options no_aot

require dastest/testing_boost public
require math
require daslib/fio
require daslib/strings_boost

// argv[0] is the running interpreter (dastest is launched as `daslang dastest/dastest.das ...`),
// so it spawns the daslang we want the child to run. Same trick as popen_argv.das.
def das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}

// dagor runs the suite with a relative -dasroot (".."), and the first case gives the child its own
// cwd, so the root has to be absolute before it reaches the child's command line. path_join drops
// the first part when the second is already absolute.
def private resolved_das_root() : string {
    return path_join(getcwd(), get_das_root())
}

def has_line(lines : array<string>; s : string) : bool {
    for (l in lines) {
        if (l == s) return true
    }
    return false
}

def child_argv(fixture, ready, release : string) : array<string> {
    return [das_exe(), "-dasroot", resolved_das_root(), fixture, "--", ready, release]
}

def make_temp_root(t : T?; prefix : string) : string {
    let tmp = create_temp_directory_result(prefix)
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return ""
    }
    return tmp as value
}

//! a deadline on a child's startup or exit, never a claim about speed: a Debug host compiles
//! what the child requires several times slower, so the deadline stretches threefold there,
//! never past five minutes
def guard(seconds : float) : float {
    return shared_module_extension() == "_debug.shared_module" ? min(seconds * 3.0, 300.0) : seconds
}

// drain until the child's ready file appears; the file event drives it, the clock only guards
def wait_for_ready(t : T?; var p : process; ready : string) {
    let started = ref_time_ticks()
    while (!fexist(ready)) {
        unsafe(process_drain(p) $(_line) {})
        if (float(get_time_usec(started)) > guard(15.0) * 1000000.0) {
            t |> failure("child never signaled ready")
            return
        }
        sleep(5u)
    }
}

// src/builtin/ARCHITECTURE.md#child-cannot-start: a binary that cannot start is an exited child
// on every platform, never a throw - POSIX gives that through the child's own _exit(127), Windows
// builds the same handle at CreateProcess
[test]
def test_process_cannot_start(t : T?) {
    t |> run("an unrunnable binary hands back an exited child: exit 127, no output, no throw") @(t : T?) {
        let noenv : array<string>
        var p = unsafe(spawn_process(["/nonexistent/daslang-that-is-not-there"], "", noenv))
        t |> success(p != null, "the spawn returns a handle rather than throwing")
        return if (p == null)
        var lines = 0
        unsafe(process_drain(p) $(_line) { lines++ })
        t |> equal(lines, 0, "a child that never ran printed nothing")
        t |> equal(unsafe(process_wait(p, guard(5.0))), 127, "wait reads the exec failure as exit 127")
        t |> equal(unsafe(process_poll(p)), 127, "and poll re-reads it from the reaped child")
        if (get_running_platform_name() == "windows") {
            t |> equal(unsafe(process_pid(p)), 0, "no process ever existed on Windows, so the pid is 0")
        } else {
            t |> success(unsafe(process_pid(p)) > 0, "POSIX forked a real child that then failed to exec")
        }
        unsafe(close_process(p))
    }
}

[test]
def test_process_lifecycle(t : T?) {
    t |> run("spawn, drain lines, poll running, wait exit code, apply env + cwd") @(t : T?) {
        let tmp = create_temp_directory_result("das process test ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root_dir = tmp as value
        let ready = path_join(root_dir, "ready")
        let release = path_join(root_dir, "release")
        let fixture = path_join(resolved_das_root(), "tests/fio/_fixture_process_child.das")
        var p = unsafe(spawn_process(child_argv(fixture, ready, release), root_dir, ["DAS_PROC_TEST=marker42"]))
        t |> success(p != null, "spawn_process returned a handle")
        // Drain the child's stdout until it signals ready; the file event drives completion,
        // the elapsed check is only a guard.
        var lines : array<string>
        let started = ref_time_ticks()
        while (!fexist(ready)) {
            unsafe(process_drain(p) $(line) {
                lines |> push(clone_string(line))
            })
            if (float(get_time_usec(started)) > guard(15.0) * 1000000.0) {
                t |> failure("child never signaled ready")
                break
            }
            sleep(5u)
        }
        unsafe(process_drain(p) $(line) {
            lines |> push(clone_string(line))
        })
        t |> success(has_line(lines, "env=marker42"), "env override reached the child: {lines}")
        t |> success(has_line(lines, "cwd={base_name(root_dir)}"), "cwd applied to the child: {lines}")
        t |> success(has_line(lines, "hello one") && has_line(lines, "hello two"),
            "both stdout lines drained one per line: {lines}")
        t |> equal(unsafe(process_poll(p)), process_running, "child is still running while parked on release")
        let pid = unsafe(process_pid(p))
        t |> success(pid > 0, "pid is positive: {pid}")
        t |> success(unsafe(process_alive(pid)), "process_alive is true while running")
        t |> success(fwrite(release, "go"), "wrote the release event")
        t |> equal(unsafe(process_wait(p, guard(5.0))), 7, "child exited with its own code")
        t |> success(!unsafe(process_alive(pid)), "process_alive is false after exit")
        unsafe(close_process(p))
        let cleanup = rmdir_rec_result(root_dir)
        t |> success(cleanup is value && cleanup as value, "temp dir removed")
    }
}

[test]
def test_close_kills_a_running_child(t : T?) {
    t |> run("closing the handle - explicitly, or leaving a with_process block - kills a child still running") @(t : T?) {
        let root_dir = make_temp_root(t, "das process close ")
        if (empty(root_dir)) return
        let fixture = path_join(resolved_das_root(), "tests/fio/_fixture_process_child.das")
        let noenv : array<string>
        var explicit_pid = 0
        var p = unsafe(spawn_process(child_argv(fixture, path_join(root_dir, "ready1"), path_join(root_dir, "never1")), "", noenv))
        wait_for_ready(t, p, path_join(root_dir, "ready1"))
        explicit_pid = unsafe(process_pid(p))
        t |> success(unsafe(process_alive(explicit_pid)), "the child is alive before close")
        unsafe(close_process(p))
        t |> success(!unsafe(process_alive(explicit_pid)), "close_process killed the running child")
        var scoped_pid = 0
        with_process(child_argv(fixture, path_join(root_dir, "ready2"), path_join(root_dir, "never2"))) $(var q) {
            wait_for_ready(t, q, path_join(root_dir, "ready2"))
            scoped_pid = unsafe(process_pid(q))
            t |> success(unsafe(process_alive(scoped_pid)), "the child is alive inside the block")
        }
        t |> success(!unsafe(process_alive(scoped_pid)), "leaving the with_process block killed the running child")
        var removed = false
        for (_attempt in range(40)) {
            let cleanup = rmdir_rec_result(root_dir)
            removed = cleanup is value && cleanup as value
            if (removed || !fexist(root_dir)) {
                removed = true
                break
            }
            sleep(50u)
        }
        t |> success(removed, "temp dir removed once the killed children released their handles")
    }
}

[test]
def test_process_terminate(t : T?) {
    t |> run("terminate stops a child parked on a release that never comes") @(t : T?) {
        let tmp = create_temp_directory_result("das process kill ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root_dir = tmp as value
        let ready = path_join(root_dir, "ready")
        let release = path_join(root_dir, "release")   // deliberately never written
        let fixture = path_join(resolved_das_root(), "tests/fio/_fixture_process_child.das")
        let noenv : array<string>
        var p = unsafe(spawn_process(child_argv(fixture, ready, release), "", noenv))
        t |> success(p != null, "spawn_process returned a handle")
        let started = ref_time_ticks()
        while (!fexist(ready)) {
            unsafe(process_drain(p) $(_line) {})
            if (float(get_time_usec(started)) > guard(15.0) * 1000000.0) {
                t |> failure("child never signaled ready")
                break
            }
            sleep(5u)
        }
        let pid = unsafe(process_pid(p))
        t |> equal(unsafe(process_wait(p, 0.2)), process_running, "wait answers process_running when the timeout passes first")
        unsafe(process_terminate(p))
        let rc = unsafe(process_wait(p, guard(5.0)))
        t |> success(rc != process_running, "child stopped after terminate (rc={rc})")
        t |> success(!unsafe(process_alive(pid)), "process_alive is false after terminate")
        unsafe(close_process(p))
        let cleanup = rmdir_rec_result(root_dir)
        t |> success(cleanup is value && cleanup as value, "temp dir removed")
    }
}

[test]
def test_exit_now_carries_its_code(t : T?) {
    t |> run("exit_now ends a child with the code it was given, and the line printed before it arrives") @(t : T?) {
        let fixture = path_join(get_das_root(), "tests/fio/_fixture_exit_now_child.das")
        var output : string
        let rc = run_and_capture([das_exe(), "-dasroot", get_das_root(), fixture], output, guard(15.0))
        t |> equal(rc, 7, "the child's exit code, got:\n{output}")
        t |> success(output |> contains("before exit_now"), "the flushed line reached the parent, got:\n{output}")
        t |> success(!(output |> contains("[daslang atexit]")), "the runtime's atexit audit must stay quiet on a deliberate exit, got:\n{output}")
    }
}
