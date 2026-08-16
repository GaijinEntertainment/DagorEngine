options gen2

require daslib/clargs
require daslib/fio

// daslang MCP worktree bootstrap.
//
// Run from ANY daslang binary against a target worktree root:
//   daslang utils/mcp/setup.das -- --root <worktree-abs-path>
//
// A fresh `git worktree add` (or clone) carries none of the gitignored,
// build-derived files the MCP server needs — `.mcp.json`, `sgconfig.yml`, a
// built `bin/` binary, or the tree-sitter grammar shared lib — so a Claude
// session there has zero daslang tools. This tool fixes that: it builds a
// worktree-local daslang (+ tree-sitter grammar), copies the platform
// `sgconfig.yml`, and points `.mcp.json` at the stdio supervisor
// (mcp_supervisor.py, via `--emit-config`) — Claude Code spawns the supervisor,
// which forwards to a daslang child and respawns it on death/rebuild.
//
// It adds no NEW secrets; an existing .mcp.json is rewritten with any other
// server entries (e.g. github, including their env blocks) preserved as-is.
// Operates entirely on `--root` via filesystem + cmake, so it runs correctly
// even when the worktree has no binary of its own yet.

[CommandLineArgs]
struct SetupArgs {
    @clarg_doc = "Worktree root to set up (default: current directory)"
    root : string = "."

    @clarg_doc = "Skip building; only wire config to an already-built binary"
    no_build : bool

    @clarg_doc = "Shared OpenSSL cache dir (Windows/MSVC) so OpenSSL is built once, not per worktree. Default %LOCALAPPDATA%/daslang/openssl; pass '-' to disable (per-build-dir)."
    openssl_dir : string = ""

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

// Repo-root-relative binary candidates, most-specific first. Returns the first
// that exists under `root` (the relative path is ready to drop into .mcp.json),
// or "" when nothing is built yet.
def locate_binary(root : string) : string {
    let candidates = ["bin/Release/daslang.exe", "bin/daslang.exe", "bin/daslang", "build/daslang"]
    for (rel in candidates) {
        if (fexist(path_join(root, rel))) return rel
    }
    return ""
}

// Run a command with inherited stdio so cmake's progress streams live.
def run_visible(cmd : string) : int {
    return unsafe(system(cmd))
}

// Reject paths with characters that could break out of the double-quoted shell
// commands we hand to cmake via system() — the quote itself, POSIX command
// substitution (`$`, backtick), cmd.exe variable expansion (`%`), and newlines.
// A build-dir / worktree path never legitimately contains these.
def shell_unsafe(s : string) : bool {
    return (find(s, "\"") >= 0 || find(s, "`") >= 0 || find(s, "$") >= 0 ||
        find(s, "%") >= 0 || find(s, "\n") >= 0 || find(s, "\r") >= 0)
}

// Build the daslang target (required; the tree-sitter grammar is ensure_grammar's
// job — it must happen even when this build is skipped). Configures build/ first if
// needed. A non-empty
// `openssl_dir` is passed as -DOPENSSL_ROOT_DIR so dasHV reuses one OpenSSL build
// across worktrees instead of rebuilding it per build dir (~15 min each). The
// flag only takes effect on the INITIAL configure — a build/ that already has a
// CMakeCache keeps its cached OPENSSL_ROOT_DIR (wipe build/ to change it). This
// tool targets fresh worktrees, so that's the common case.
def build_worktree(root, openssl_dir : string) : bool {
    let build_dir = path_join(root, "build")
    if (!fexist(path_join(build_dir, "CMakeCache.txt"))) {
        print("Configuring CMake in {build_dir} ...\n")
        var cfg_cmd = "cmake -S \"{root}\" -B \"{build_dir}\" -DCMAKE_BUILD_TYPE=Release"
        if (!empty(openssl_dir)) {
            cfg_cmd += " -DOPENSSL_ROOT_DIR=\"{to_generic_path(openssl_dir)}\""
        }
        let rc = run_visible(cfg_cmd)
        if (rc != 0) {
            print("ERROR: cmake configure failed (exit {rc})\n")
            return false
        }
    }
    print("Building daslang (this can take a while) ...\n")
    let rc = run_visible("cmake --build \"{build_dir}\" --target daslang --config Release")
    if (rc != 0) {
        print("ERROR: building daslang failed (exit {rc})\n")
        return false
    }
    return true
}

// The tree-sitter grammar shared lib powers the ast-grep tools (grep_usage/outline);
// without it they silently match NOTHING. Runs unconditionally — the build step above is
// skipped whenever a daslang binary already exists (or --no-build), which used to leave
// worktrees grammar-less. Build the target when a configured build dir is present (two C
// files, seconds); else stage the lib from the tree running this setup, best-effort.
def ensure_grammar(root : string) {
    let plat = get_platform_name()
    let lib_name = plat == "windows" ? "daslang.dll" : (plat == "darwin" ? "daslang.dylib" : "daslang.so")
    let rel = path_join("tree-sitter-daslang", lib_name)
    let dst = path_join(root, rel)
    if (fexist(dst)) {
        return
    }
    let build_dir = path_join(root, "build")
    if (fexist(path_join(build_dir, "CMakeCache.txt"))) {
        print("Building tree-sitter grammar ...\n")
        let rc = run_visible("cmake --build \"{build_dir}\" --target tree_sitter_daslang --config Release")
        if (rc == 0 && fexist(dst)) {
            return
        }
        print("WARNING: grammar build did not produce {rel} (exit {rc}); staging from the source tree instead\n")
    }
    let src = path_join(get_das_root(), rel)
    if (!fexist(src) || get_full_file_name(src) == get_full_file_name(dst)) {
        print("WARNING: no grammar lib to stage ({rel}); ast-grep tools (grep_usage/outline) will return nothing\n")
        return
    }
    let cr = copy_file_result(src, dst, false)
    if (cr is error) {
        print("WARNING: failed to stage grammar lib: {cr as error}\n")
    } else {
        print("Staged tree-sitter grammar from {get_das_root()} — a grammar-changing branch should rebuild its own (cmake --build build --target tree_sitter_daslang)\n")
    }
}

// Copy the platform sgconfig.yml template if no sgconfig.yml exists yet.
def ensure_sgconfig(root : string) {
    let dst = path_join(root, "sgconfig.yml")
    if (fexist(dst)) {
        print("sgconfig.yml already present — left as is\n")
        return
    }
    let plat = get_platform_name()
    let tmpl_name = plat == "windows" ? "sgconfig.yml.windows" : (plat == "darwin" ? "sgconfig.yml.osx" : "sgconfig.yml.linux")
    let src = path_join(root, tmpl_name)
    if (!fexist(src)) {
        print("WARNING: {tmpl_name} not found under root — cannot set up sgconfig.yml\n")
        return
    }
    let cr = copy_file_result(src, dst, false)
    if (cr is error) {
        print("WARNING: failed to copy {tmpl_name}: {cr as error}\n")
    } else {
        print("Wrote sgconfig.yml (from {tmpl_name})\n")
    }
}

// Stage the JIT backend files that a plain `--target daslang` build does NOT produce:
// <root>/lib/LLVM.dll (the LLVM codegen backend, loaded by name from dasLLVM's bindings) and
// <root>/bin/lld-link.exe (links each JIT-emitted .dll). Only the dasLLVM shared-lib CMake step
// stages these, and building just the `daslang` target skips it — so without this a fresh
// worktree binary silently falls back to the interpreter (`-jit` fails: "can't load LLVM.dll").
// Copy them from the source tree of the daslang running this setup (get_das_root()), which is a
// JIT-capable build. When setup itself runs under the target worktree's binary, use DASLANG as a
// fallback SDK binary: a worktree-local root cannot supply a backend file that it is missing.
// Windows-only for now (the codegen dll name / linker differ on posix — staging there is TBD).
def root_from_daslang_binary(binary : string) : string {
    let executable_dir = dir_name(get_full_file_name(binary))
    let leaf = base_name(executable_dir)
    if (leaf == "Release" || leaf == "Debug" || leaf == "RelWithDebInfo" || leaf == "MinSizeRel") {
        return dir_name(dir_name(executable_dir))
    }
    return dir_name(executable_dir)
}

def stage_jit_backend(root : string) {
    if (get_platform_name() != "windows") {
        return
    }
    let src_root = get_das_root()
    var fallback_root = ""
    var fallback_source = "DASLANG is unset"
    if (has_env_variable("DASLANG")) {
        let sdk_binary = get_env_variable("DASLANG")
        if (fexist(sdk_binary)) {
            fallback_root = root_from_daslang_binary(sdk_binary)
            fallback_source = "DASLANG root {fallback_root}"
        } else {
            fallback_source = "DASLANG does not name an existing binary"
        }
    }
    let jit_files = ["lib/LLVM.dll", "bin/lld-link.exe"]
    for (rel in jit_files) {
        let dst = path_join(root, rel)
        if (fexist(dst)) {
            continue
        }
        var src = path_join(src_root, rel)
        if ((!fexist(src) || get_full_file_name(src) == get_full_file_name(dst)) && !empty(fallback_root)) {
            src = path_join(fallback_root, rel)
        }
        if (!fexist(src) || get_full_file_name(src) == get_full_file_name(dst)) {
            print("WARNING: JIT backend file '{rel}' has no external source (checked {src_root}; {fallback_source}); worktree -jit may be unavailable\n")
            continue
        }
        let dstdir = dir_name(dst)
        if (!fexist(dstdir)) {
            mkdir(dstdir)
        }
        let cr = copy_file_result(src, dst, false)
        if (cr is error) {
            print("WARNING: failed to stage JIT backend '{rel}': {cr as error}\n")
        } else {
            print("Staged JIT backend: {rel}\n")
        }
    }
}

// Point .mcp.json at the stdio supervisor (mcp_supervisor.py). The supervisor
// owns the .mcp.json merge (preserving other servers like github), so setup
// just invokes it. Probes `python3` first (a bare `python` can be Python 2 on
// legacy boxes, and the supervisor is Python 3), then `python` (python.org
// Windows installs ship no python3 alias); the supervisor emits whichever
// launcher resolves into .mcp.json.
def emit_mcp_config(root : string) : bool {
    let script = path_join(root, "utils/mcp/mcp_supervisor.py")
    if (!fexist(script)) {
        print("ERROR: {script} not found\n")
        return false
    }
    for (py in ["python3", "python"]) {
        continue if (run_visible("{py} --version") != 0)
        let rc = run_visible("{py} \"{script}\" --emit-config --repo-root \"{root}\"")
        if (rc != 0) {
            print("ERROR: emit-config failed (exit {rc})\n")
            return false
        }
        return true
    }
    print("ERROR: neither python3 nor python found on PATH\n")
    return false
}

[export]
def main() : int {
    let argv <- get_cli_arguments()
    var r <- parse_args(type<SetupArgs>, argv)
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<SetupArgs>), "mcp-setup")
        return 1
    }
    let cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<SetupArgs>), "mcp-setup")
        return 0
    }
    let root = get_full_file_name(cfg.root)
    if (!fexist(path_join(root, "utils/mcp/main.das"))) {
        print("ERROR: {root} does not look like a daslang checkout (no utils/mcp/main.das)\n")
        return 1
    }
    print("Setting up daslang MCP in: {root}\n")

    // Shared OpenSSL cache (Windows/MSVC): build OpenSSL once and reuse across
    // worktrees instead of rebuilding it per build dir. Honored by dasHV's
    // OPENSSL_ROOT_DIR override. "-" forces per-build-dir; otherwise empty
    // defaults to %LOCALAPPDATA% on Windows (non-Windows stays per-build-dir).
    var openssl_dir = cfg.openssl_dir
    if (openssl_dir == "-") {
        openssl_dir = ""  // explicit opt-out: per-build-dir OpenSSL
    } elif (empty(openssl_dir) && get_platform_name() == "windows") {
        let lad = get_env_variable("LOCALAPPDATA")
        if (!empty(lad)) {
            openssl_dir = path_join(lad, "daslang/openssl")
        }
    }

    if (shell_unsafe(root)) {
        print("ERROR: worktree root contains a shell-unsafe character: {root}\n")
        return 1
    }
    if (!empty(openssl_dir) && shell_unsafe(openssl_dir)) {
        print("ERROR: --openssl-dir contains a shell-unsafe character: {openssl_dir}\n")
        return 1
    }

    var binary = locate_binary(root)
    if (empty(binary) && !cfg.no_build) {
        if (!empty(openssl_dir)) {
            print("Using shared OpenSSL cache: {openssl_dir}\n")
        }
        if (!build_worktree(root, openssl_dir)) return 1
        binary = locate_binary(root)
    }
    if (empty(binary)) {
        if (cfg.no_build) {
            print("ERROR: no daslang binary found under {root} and --no-build was set\n")
        } else {
            print("ERROR: build completed but no daslang binary was found\n")
        }
        return 1
    }

    // grammar first: its build target's POST_BUILD step stamps sgconfig.yml from the platform
    // template, so ensure_sgconfig must report AFTER any such stamp to describe the final state
    ensure_grammar(root)
    ensure_sgconfig(root)
    stage_jit_backend(root)
    if (!emit_mcp_config(root)) {
        return 1
    }

    print("\nDone. Restart the Claude Code session in {root} to pick up the daslang MCP server.\n")
    print("Claude Code spawns the stdio supervisor (utils/mcp/mcp_supervisor.py); no persistent process to keep running.\n")
    print("The daslang LSP plugin (.claude/skills/daslang-lsp, checked in) also loads automatically\n")
    print("when the session starts at the worktree root — it uses the binary built above (needs python3 on PATH).\n")
    print("NOTE: the parse-aware tools (grep_usage/outline/cpp_*) also need the ast-grep `sg` CLI on PATH.\n")
    return 0
}
