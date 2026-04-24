options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dashv/dashv_boost
require common public

// --- Helpers ---

def live_get(path, port : string) : string {
    let p = empty(port) ? "9090" : port
    var result = ""
    var is_error = false
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "http://localhost:{p}{path}"
        request(req) <| $(resp) {
            if (resp != null) {
                result = clone_string(string(resp.body))
                if (int(resp.status_code) >= 400) {
                    is_error = true
                }
            }
        }
    }
    if (empty(result) && !is_error) {
        return make_tool_result("Failed to connect to daslang-live on port {p}. Is it running?", true)
    }
    return make_tool_result(result, is_error)
}

def live_post(path, port : string; body : string = "") : string {
    let p = empty(port) ? "9090" : port
    var result = ""
    var is_error = false
    with_http_request() <| $(var req) {
        req.method = http_method.POST
        req.url := "http://localhost:{p}{path}"
        if (!empty(body)) {
            req.body := body
            set_content_type(req, "application/json")
        }
        request(req) <| $(resp) {
            if (resp != null) {
                result = clone_string(string(resp.body))
                if (int(resp.status_code) >= 400) {
                    is_error = true
                }
            }
        }
    }
    if (empty(result) && !is_error) {
        return make_tool_result("Failed to connect to daslang-live on port {p}. Is it running?", true)
    }
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

def do_live_shutdown(port : string) : string {
    return live_post("/shutdown", port)
}

def live_is_running(port : string) : bool {
    let p = empty(port) ? "9090" : port
    var running = false
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "http://localhost:{p}/status"
        req.timeout = uint16(2)
        request(req) <| $(resp) {
            if (resp != null && int(resp.status_code) > 0) {
                running = true
            }
        }
    }
    return running
}

def do_live_launch(file, project, port : string) : string {
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
    if (!stat(live_exe).is_valid) {
        return make_tool_result("daslang-live not found in {root}/bin/Release/ or {root}/bin/", true)
    }
    // Build launch command — daslang-live -cwd handles working directory
    var launch_exe = clone_string(live_exe)
    var launch_file = clone_string(file)
    var launch_project = clone_string(project)
    if (is_windows) {
        launch_exe = replace(launch_exe, "/", "\\")
        launch_file = replace(launch_file, "/", "\\")
        launch_project = replace(launch_project, "/", "\\")
    }
    var script_args = "-cwd \"{launch_file}\""
    if (!empty(launch_project)) {
        script_args = "-project \"{launch_project}\" {script_args}"
    }
    let launcher = "{root}/_mcp_live_launch"
    if (is_windows) {
        let ps1 = replace("{launcher}.ps1", "/", "\\")
        fopen(ps1, "wt") <| $(f) {
            if (f != null) {
                fprint(f, "Start-Process -FilePath \"{launch_exe}\" -ArgumentList '{script_args}'\n")
            }
        }
        var exit_code = 0
        unsafe {
            exit_code = system("powershell -NoProfile -ExecutionPolicy Bypass -File \"{ps1}\"")
        }
        remove(ps1)
        if (exit_code != 0) {
            return make_tool_result("Failed to launch daslang-live (exit {exit_code})", true)
        }
    } else {
        let sh = "{launcher}.sh"
        fopen(sh, "wt") <| $(f) {
            if (f != null) {
                fprint(f, "#!/bin/sh\nnohup \"{live_exe}\" {script_args} > /dev/null 2>&1 &\n")
            }
        }
        var exit_code = 0
        unsafe {
            exit_code = system("chmod +x {sh} && {sh}")
        }
        remove(sh)
        if (exit_code != 0) {
            return make_tool_result("Failed to launch daslang-live (exit {exit_code})", true)
        }
    }
    // Poll for startup (up to 10 seconds)
    for (i in range(20)) {
        sleep(500u)
        if (live_is_running(p)) {
            let status = live_get("/status", p)
            return make_tool_result("daslang-live started successfully on port {p}.\n{status}")
        }
    }
    return make_tool_result("daslang-live launched but did not respond on port {p} within 10 seconds. Check the console for errors.", true)
}
