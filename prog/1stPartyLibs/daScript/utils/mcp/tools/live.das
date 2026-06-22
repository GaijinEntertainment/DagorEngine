options gen2
options rtti

require dashv/dashv_boost
require common public
require daslib/logger
require strings

// --- Helpers ---

def live_get(path, port : string) : string {
    let p = empty(port) ? "9090" : port
    var result = ""
    var is_error = false
    with_http_request() $(var req) {
        req.method = http_method.GET
        req.url := "http://localhost:{p}{path}"
        request(req) $(resp) {
            if (resp != null) {
                result = clone_string(string(resp.body))
                if (int(resp.status_code) >= 400) {
                    is_error = true
                }
            }
        }
    }
    if (empty(result) && !is_error) return make_tool_result("Failed to connect to daslang-live on port {p}. Is it running?", true)
    return make_tool_result(result, is_error)
}

def live_post(path, port : string; body : string = "") : string {
    let p = empty(port) ? "9090" : port
    var result = ""
    var is_error = false
    with_http_request() $(var req) {
        req.method = http_method.POST
        req.url := "http://localhost:{p}{path}"
        if (!empty(body)) {
            req.body := body
            set_content_type(req, "application/json")
        }
        request(req) $(resp) {
            if (resp != null) {
                result = clone_string(string(resp.body))
                if (int(resp.status_code) >= 400) {
                    is_error = true
                }
            }
        }
    }
    if (empty(result) && !is_error) return make_tool_result("Failed to connect to daslang-live on port {p}. Is it running?", true)
    return make_tool_result(result, is_error)
}

// --- Tools ---

def do_live_status(port : string) : string {
    return live_get("/status", port)
}

def do_live_error(port : string) : string {
    return live_get("/error", port)
}

def do_live_reload(full, port : string) : string {
    let path = full == "true" ? "/reload/full" : "/reload"
    return live_post(path, port)
}

def do_live_pause(paused, port : string) : string {
    let path = paused == "true" ? "/pause" : "/unpause"
    return live_post(path, port)
}

def do_live_command(name, args, port : string) : string {
    var body : string
    if (empty(args)) {
        body = "\{\"name\":\"{name}\"\}"
    } else {
        body = "\{\"name\":\"{name}\",\"args\":{args}\}"
    }
    return live_post("/command", port, body)
}

def do_live_commands(commands, port : string) : string {
    if (empty(commands)) return make_tool_result("commands must be a non-empty JSON array of \{name,args\} objects", true)
    return live_post("/commands", port, commands)
}

def do_live_shutdown(port : string) : string {
    return live_post("/shutdown", port)
}

def live_is_running(port : string) : bool {
    let p = empty(port) ? "9090" : port
    var running = false
    with_http_request() $(var req) {
        req.method = http_method.GET
        req.url := "http://localhost:{p}/status"
        req.timeout = uint16(2)
        request(req) $(resp) {
            if (resp != null && int(resp.status_code) > 0) {
                running = true
            }
        }
    }
    return running
}

// Resolve each load_module entry to an absolute path. Same rationale as
// resolving file/project/project_root before composing daslang-live argv:
// the `-cwd` chdir runs before `require_dynamic_modules`, so any relative
// `-load_module <P>` would re-resolve against the new cwd. Public so the
// test suite can unit-test the abs-resolve without spawning daslang-live.
def public resolve_load_modules(load_modules : array<string>) : array<string> {
    var abs : array<string>
    abs |> reserve(length(load_modules))
    for (lm in load_modules) {
        abs |> push(resolve_path(lm))
    }
    return <- abs
}

// daslang-live script_args composition. Extracted so tests can pin the
// flag-ordering invariants without spawning a subprocess. Slash-replace on
// Windows happens here so the resulting string is shell-ready for Start-Process.
def public build_live_script_args(file, project, project_root : string;
                                  is_windows : bool;
                                  load_modules : array<string> = array<string>()) : string {
    var launch_file = clone_string(file)
    var launch_project = clone_string(project)
    var launch_project_root = clone_string(project_root)
    var launch_load_modules : array<string>
    launch_load_modules |> reserve(length(load_modules))
    for (lm in load_modules) {
        var lm_norm = clone_string(lm)
        if (is_windows) {
            lm_norm = replace(lm_norm, "/", "\\")
        }
        launch_load_modules |> push(lm_norm)
    }
    if (is_windows) {
        launch_file = replace(launch_file, "/", "\\")
        launch_project = replace(launch_project, "/", "\\")
        launch_project_root = replace(launch_project_root, "/", "\\")
    }
    var script_args = "-cwd \"{launch_file}\""
    if (!empty(launch_project)) {
        script_args = "-project \"{launch_project}\" {script_args}"
    }
    // Build the -load_module chain in source order, then prepend once. Input
    // order is preserved left-to-right in the output.
    let lm_args = build_string() $(var w) {
        var first = true
        for (lm in launch_load_modules) {
            if (!first) {
                w |> write(" ")
            }
            first = false
            w |> write("-load_module \"{lm}\"")
        }
    }
    if (!empty(lm_args)) {
        script_args = "{lm_args} {script_args}"
    }
    if (!empty(launch_project_root)) {
        script_args = "-project_root \"{launch_project_root}\" {script_args}"
    }
    return script_args
}

def private port_in_range(s : string) : bool {
    // Used before interpolating `port` into a shell-ready argv string (see
    // `script_args` below). Reject anything that isn't digits-only AND in
    // [1, 65535], so:
    //   - shell metacharacters (quotes / spaces / `$`) can never reach
    //     `system()`,
    //   - out-of-range ports ("0", "65536", "99999") fail fast here instead
    //     of through a 10s daslang-live "did not respond" timeout.
    if (empty(s) || length(s) > 5) return false
    var n = 0
    peek_data(s) $(bytes) {
        for (b in bytes) {
            let c = int(b)
            if (!is_number(c)) {
                n = -1
                return
            }
            n = n * 10 + (c - '0')
        }
    }
    return n > 0 && n <= 65535
}

def do_live_launch(file, project, project_root : string; port : string = ""; load_modules : array<string> = array<string>()) : string {
    if (!empty(port) && !port_in_range(port)) return make_tool_result("invalid port '{port}': expected 1-65535", true)
    let p = empty(port) ? "9090" : port
    // Check if already running
    if (live_is_running(p)) {
        // Already running — get status to show useful info
        let status = live_get("/status", p)
        return make_tool_result("daslang-live is already running on port {p}.\n{status}")
    }
    // Find daslang-live executable — check bin/Release then bin/ under das_root
    let root = get_das_root()
    let is_windows = get_platform_name() == "windows"
    let exe_name = is_windows ? "daslang-live.exe" : "daslang-live"
    var live_exe = "{root}/bin/Release/{exe_name}"
    if (!stat(live_exe).is_valid) {
        live_exe = "{root}/bin/{exe_name}"
    }
    if (!stat(live_exe).is_valid) return make_tool_result("daslang-live not found in {root}/bin/Release/ or {root}/bin/", true)
    // Build launch command — daslang-live -cwd handles working directory.
    // CRITICAL: resolve file / project / project_root / load_modules to
    // absolute paths BEFORE composing the args. daslang-live's `-cwd` flag
    // chdir's to the script's folder (utils/daslang-live/main.cpp:696-707)
    // BEFORE running require_dynamic_modules — so a relative `-project_root <P>`
    // or `-load_module <M>` would re-resolve against the new cwd and fail to
    // find the module folder.
    let abs_file = resolve_path(file)
    let abs_project = empty(project) ? "" : resolve_path(project)
    let abs_project_root = empty(project_root) ? "" : resolve_path(project_root)
    let abs_load_modules <- resolve_load_modules(load_modules)
    var script_args = build_live_script_args(abs_file, abs_project, abs_project_root, is_windows, abs_load_modules)
    if (!empty(port)) {
        // Make the spawned binary actually bind the requested port (matching the
        // poll URL above). Without this, the binary uses its own default and we'd
        // either get an "already running" false-positive or a never-responds timeout.
        script_args = "--live-port {port} {script_args}"
    }
    // Spawn the daslang-live process via a tiny launcher script that
    // detaches the child (Start-Process on Windows, nohup+& on POSIX).
    // CRITICAL: use popen_argv (not system()) — system() inherits MCP's
    // stdio, so any output from PowerShell / sh / chmod would land on the
    // JSON-RPC channel and break framing. popen_argv pipe-captures both
    // streams; we log any captured chatter for debugging.
    let launcher = "{root}/_mcp_live_launch"
    if (is_windows) {
        let launch_exe = replace(live_exe, "/", "\\")
        let ps1 = replace("{launcher}.ps1", "/", "\\")
        fopen(ps1, "wt") $(f) {
            if (f != null) {
                fprint(f, "Start-Process -FilePath \"{launch_exe}\" -ArgumentList '{script_args}'\n")
            }
        }
        var captured : string
        let exit_code = run_and_capture(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps1],
            captured)
        remove(ps1)
        if (!empty(captured)) {
            logger_warning("mcp.live.launch", "powershell output (would have corrupted JSON-RPC if not captured): {captured}")
        }
        return make_tool_result("Failed to launch daslang-live (exit {exit_code}): {captured}", true) if (exit_code != 0)
    } else {
        let sh = "{launcher}.sh"
        fopen(sh, "wt") $(f) {
            if (f != null) {
                fprint(f, "#!/bin/sh\nnohup \"{live_exe}\" {script_args} > /dev/null 2>&1 &\n")
            }
        }
        // chmod first (silent on success), then sh to launch the detached
        // daslang-live. Two popen_argv calls avoid invoking a shell to
        // parse `&&` — keeps the stdio capture clean either way.
        var chmod_out : string
        let chmod_rc = run_and_capture(["chmod", "+x", sh], chmod_out)
        if (chmod_rc != 0) {
            remove(sh)
            return make_tool_result("Failed to chmod launcher (exit {chmod_rc}): {chmod_out}", true)
        }
        var sh_out : string
        let sh_rc = run_and_capture(["sh", sh], sh_out)
        remove(sh)
        if (!empty(sh_out)) {
            logger_warning("mcp.live.launch", "launcher output (would have corrupted JSON-RPC if not captured): {sh_out}")
        }
        return make_tool_result("Failed to launch daslang-live (exit {sh_rc}): {sh_out}", true) if (sh_rc != 0)
    }
    // Poll for startup (up to 10 seconds)
    for (_ in range(20)) {
        sleep(500u)
        if (live_is_running(p)) {
            let status = live_get("/status", p)
            return make_tool_result("daslang-live started successfully on port {p}.\nReminder: to drive synthetic mouse/keyboard input, first post set_user_control with enabled=false, or the real OS cursor races GLFW's poll and clobbers synth IO (gestures miss; snapshots read the OS cursor). with_recording_app / the dastest harness do this for you; a manual probe host does not.\n{status}")
        }
    }
    return make_tool_result("daslang-live launched but did not respond on port {p} within 10 seconds. Check the console for errors.", true)
}
