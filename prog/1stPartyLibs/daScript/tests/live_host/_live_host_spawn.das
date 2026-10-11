options gen2
options no_aot
options no_unused_block_arguments = false

module _live_host_spawn public

require daslib/fio
require daslib/network
require daslib/command_line
require daslib/strings_boost
require dashv/dashv_boost
require strings

//! The spawn helpers the tests of this folder share for a daslang-live child: the host beside
//! the daslang running dastest, a free port of the span, the ready wait that drains stdout,
//! the reload endpoints with their generation wait, and the exit wait.

let PORT_FIRST = 19390
let PORT_SPAN = 10

//! one band per spawning test: two tests of a parallel suite scanning one span both find the same
//! port free, and then the loser's host dies on the single-instance lock while the winner's
//! /status answers the loser's ready wait
let PORT_BAND_READY = PORT_FIRST
let PORT_BAND_AUDIO = PORT_FIRST + PORT_SPAN

//! the daslang-live beside the daslang running dastest; empty where the tree has not built it
def live_exe() : string {
    let beside = path_join(dir_name(get_full_file_name(get_das_exe())), "daslang-live{get_platform_name() == "windows" ? ".exe" : ""}")
    return fexist(beside) ? beside : ""
}

//! the body of a 200 answer to GET /status, empty when nothing answers
def status_body(port : int) : string {
    var body = ""
    with_http_request() $(var req) {
        req.method = http_method.GET
        req.url := "http://127.0.0.1:{port}/status"
        request(req) $(resp) {
            if (resp != null && int(resp.status_code) == 200) {
                body = string(resp.body)
            }
        }
    }
    return body
}

def status_answers(port : int) : bool {
    return !empty(status_body(port))
}

//! the first port from `from` to the end of the span nothing listens on, so a /status answer is
//! this test's own child and its /shutdown reaches nothing else; -1 when every port is taken
def free_port_from(from, last : int) : int {
    for (port in range(from, last)) {
        if (probe_local_port("127.0.0.1", port) == port) return port
    }
    return -1
}

def free_port(band : int) : int {
    return free_port_from(band, band + PORT_SPAN)
}

//! spawns daslang-live on `script` with `extra_args` after the `--` on a free port and runs
//! the block once /status answers, with the port and the seconds it took; two tests of a
//! parallel suite can pick one free port at once and the loser's host ends at once on
//! "already running", so an attempt whose child ends before it answers moves to the next
//! free port, three attempts in all - the block then sees the last attempt, ready -1. The
//! child is closed after the block; stdout is drained into `lines` throughout. False, with
//! the block never run and a line in `lines`, when no port of the span is free
def with_ready_host(exe, script : string; band : int; extra_args : array<string>; var lines : array<string>; blk : block<(var host : process; port : int; ready : float) : void>) : bool {
    let no_env : array<string>
    var from = band
    for (attempt in range(3)) {
        let port = free_port_from(from, band + PORT_SPAN)
        if (port < 0) {
            lines |> push("no free port from {from} to {band + PORT_SPAN - 1}")
            return false
        }
        var args <- [exe, "--live-port={port}", script]
        args |> push_from(extra_args)
        var host = unsafe(spawn_process(args, get_das_root(), no_env))
        let ready = wait_ready(host, port, 30.0, lines)
        if (ready >= 0.0 || attempt == 2) {
            invoke(blk, host, port, ready)
            unsafe(close_process(host))   //! ends a child that never became ready
            return true
        }
        unsafe(close_process(host))
        from = port + 1
    }
    return false
}

def post(port : int; path : string) {
    with_http_request() $(var req) {
        req.method = http_method.POST
        req.url := "http://127.0.0.1:{port}{path}"
        request(req) $(_resp) {
        }
    }
}

def post_shutdown(port : int) {
    post(port, "/shutdown")
}

//! seconds until /status answers, draining the host's stdout meanwhile; -1 past the deadline
//! or once the host is gone
def wait_ready(var host : process; port : int; seconds : float; var lines : array<string>) : float {
    let started = ref_time_ticks()
    while (float(get_time_usec(started)) < seconds * 1000000.0) {
        unsafe(process_drain(host) $(line) {
            lines |> push(line)
        })
        if (unsafe(process_poll(host)) != process_running) return -1.0
        if (probe_local_port("127.0.0.1", port) < 0 && status_answers(port)) return float(get_time_usec(started)) / 1000000.0
        sleep(250u)
    }
    return -1.0
}

//! the reload generation /status reports, -1 when nothing answers
def status_generation(port : int) : int {
    let body = status_body(port)
    return -1 if (empty(body))
    let at = find(body, "\"generation\"")
    return -1 if (at < 0)
    let colon = find(body, ":", at)
    return -1 if (colon < 0)
    return to_int(strip(slice(body, colon + 1, find(body, "\n", colon))))
}

//! posts `path` and waits until /status reports `generation`, draining stdout meanwhile;
//! false past the deadline or once the host is gone
def post_and_wait_generation(var host : process; port : int; path : string; generation : int; seconds : float; var lines : array<string>) : bool {
    post(port, path)
    let started = ref_time_ticks()
    while (float(get_time_usec(started)) < seconds * 1000000.0) {
        unsafe(process_drain(host) $(line) {
            lines |> push(line)
        })
        if (unsafe(process_poll(host)) != process_running) return false
        if (status_generation(port) >= generation) return true
        sleep(100u)
    }
    return false
}

//! the exit code once the host ends, draining its stdout meanwhile and once more after the exit -
//! the lines the host writes last, its shutdown's exception and its leak census among them,
//! land between the last drain and the poll that sees it gone; process_running past the deadline
def wait_exit_draining(var host : process; seconds : float; var lines : array<string>) : int {
    let started = ref_time_ticks()
    while (float(get_time_usec(started)) < seconds * 1000000.0) {
        unsafe(process_drain(host) $(line) {
            lines |> push(line)
        })
        let rc = unsafe(process_poll(host))
        if (rc != process_running) {
            unsafe(process_drain(host) $(line) {
                lines |> push(line)
            })
            return rc
        }
        sleep(20u)
    }
    return process_running
}
