options gen2

require strings
require daslib/strings_boost
require daslib/fio
require daslib/regex
require daslib/regex_boost
require dastest/review_gate

// The mechanical half of utils/REVIEW.md (contract: REVIEW_COMMON.md at the repo root).
// Run from the repo root: bin/daslang utils/REVIEW.das — exit 0 clean, 1 with findings.

// utils/ children that are not tool directories: the shared library hub
var private LIBRARY_DIRS <- { "common" }

// tools whose development runs interpreted under a supervisor - the watchdog's --stdio front
// for mcp and dap, its --lsp front for the lsp subtools: never built or shipped as an exe,
// since that form would ship without anyone having run it. An entry leaves when the front
// names the exe.
var private SUPERVISED_TOOLS <- { "mcp", "lsp", "dap" }

// Files licensed to name utils/internal outside internal/:
var private INTERNAL_REF_EXEMPT <- {
    "utils/REVIEW.md",              // states the rule
    "utils/REVIEW.das",             // carries the needle
    "utils/CMakeLists.txt",         // builds and CI-tests internal tools, never ships
    "utils/mcp/setup.das",          // wires the internal das-herd shim behind an exists-check; same exclusion as ci/smoke_test_bundle.sh
    "utils/mcp/test_setup.das"      // plants that shim in a scratch root to drive the exists-check; not installed
}

// Extensions that can carry a reference by require, include, or command line.
var private TEXT_EXTS <- { ".das", ".py", ".sh", ".ps1", ".psm1", ".bat", ".cmd", ".md",
                           ".rst", ".yml", ".yaml", ".json", ".txt", ".cmake", ".html",
                           ".css", ".js", ".ts" }

def private is_path_name_byte(b : int) : bool {
    return is_alnum(b) || b == '_' || b == '-'
}

// True when `text` mentions path `p` as a whole component prefix — `utils/aot` matches
// `utils/aot/main.das` and `DESTINATION utils/aot)`, never `utils/aot-extra`.
def private mentions_path(text : string; p : string) : bool {
    var found = false
    let plen = length(p)
    peek_data(text) $(d) {
        let n = length(d)
        var from = 0
        while (!found) {
            let idx = find(d, p, from)
            break if (idx < 0)
            let nxt = idx + plen
            if (nxt >= n || !is_path_name_byte(int(d[nxt]))) {
                found = true
            }
            from = idx + 1
        }
    }
    return found
}

def private list_dirs(parent : string) : array<string> {
    var out : array<string>
    dir(parent) $(name) {
        return if (name |> starts_with("."))      // `.`, `..`, and a JIT's `.jitted_scripts` cache
        let st = stat(path_join(parent, name))
        if (st.is_valid && st.is_dir) {
            out |> push(name)
        }
    }
    out |> sort
    return <- out
}

def private walk_files(root : string; var out : array<string>) {
    dir(root) $(name) {
        return if (name == "." || name == "..")
        let p = path_join(root, name)
        let st = stat(p)
        return if (!st.is_valid)
        if (st.is_dir) {
            // build/output trash present on a working tree but never committed
            return if (name == "node_modules" || name == "out" || name == "__pycache__" || name == "_build" || name == ".git" || name == "playwright-report" || name == "test-results")
            walk_files(p, out)
        } else {
            out |> push(to_generic_path(p))
        }
    }
}

def private check_kebab_case(externals, internals : array<string>) {
    for (n in externals) {
        if (!is_kebab_case(n)) {
            gate_finding("utils/{n}", "tool directory name is not kebab-case (like-this, never like_that or likeThis)")
        }
    }
    for (n in internals) {
        if (!is_kebab_case(n)) {
            gate_finding("utils/internal/{n}", "tool directory name is not kebab-case (like-this, never like_that or likeThis)")
        }
    }
}

def private install_blocks_of(text : string) : array<string> {
    return <- cmake_command_blocks(text, "install")
}

def private check_install_symmetry(externals : array<string>) {
    let cm = fread("CMakeLists.txt")
    var inscope blocks <- install_blocks_of(cm)
    for (n in externals) {
        let needle = "utils/{n}"
        var ok = false
        for (b in blocks) {
            if (mentions_path(b, needle)) {
                ok = true
                break
            }
        }
        if (!ok) {
            gate_finding("CMakeLists.txt", "external tool utils/{n} has no install rule — the folder is the audience decision: external installs, or the tool moves under utils/internal/")
        }
    }
    // internal never installs — scan the two files that own utils install rules
    for (f in ["CMakeLists.txt", "utils/CMakeLists.txt"]) {
        let text = fread(f)
        var inscope fblocks <- install_blocks_of(text)
        for (b in fblocks) {
            if (find(b, "utils/internal") >= 0) {
                gate_finding(f, find_line(text, b), "install rule references utils/internal — utils/internal/ never installs")
            }
        }
    }
}

def private check_registration_triple(tools, internals : array<string>) {
    let toctree = fread("doc/source/reference/utils.rst")
    let iclaude = fread("install/CLAUDE.md")
    for (n in tools) {
        let page = replace(n, "-", "_") + ".rst"
        if (!fexist("doc/source/reference/utils/{page}")) {
            gate_finding("utils/{n}", "no doc/source/reference/utils/{page} — every external tool lands its rst page")
        }
        if (find(toctree, "utils/{page}") < 0) {
            gate_finding("doc/source/reference/utils.rst", "no toctree row utils/{page} for external tool utils/{n}")
        }
        if (find(iclaude, "utils/{n}/") < 0) {
            gate_finding("install/CLAUDE.md", "no layout row for external tool utils/{n}/")
        }
    }
    for (n in internals) {
        let page = replace(n, "-", "_") + ".rst"
        if (fexist("doc/source/reference/utils/{page}")) {
            gate_finding("doc/source/reference/utils/{page}", "rst page for internal tool utils/internal/{n} — internal docs live in the tool's README or skills/internal/")
        }
    }
}

def private check_no_internal_refs {
    var files : array<string>
    walk_files("utils", files)
    for (p in files) {
        continue if (p |> starts_with("utils/internal/") || key_exists(INTERNAL_REF_EXEMPT, p) || !key_exists(TEXT_EXTS, extension(p)))
        let text = fread(p)
        for (needle in ["utils/internal", "utils\\internal"]) {
            let line = find_line(text, needle)
            if (line > 0) {
                gate_finding(p, line, "references utils/internal — external tools never depend on internal ones")
                break
            }
        }
    }
}

def private check_supervised_tools {
    let ucm = fread("utils/CMakeLists.txt")
    var inscope exes <- cmake_list_entries(ucm, "DAS_UTILS_SHIPPED_EXES")
    var inscope built <- cmake_list_entries(ucm, "DAS_UTILS")
    for (s in keys(SUPERVISED_TOOLS)) {
        var hit = find_index(exes, s) >= 0
        for (b in built) {
            hit ||= base_name(b) == s
        }
        if (hit) {
            gate_finding("utils/CMakeLists.txt", "{s} is in DAS_UTILS or DAS_UTILS_SHIPPED_EXES — its development runs interpreted under a supervisor, so an exe form ships without anyone having run it; point the supervisor's --program at the exe first")
        }
    }
}

var private BIN_EXE <- %regex~`([A-Za-z0-9_./-]+)\.exe`%%

def private check_shipped_exes(externals : array<string>) {
    let ucm = fread("utils/CMakeLists.txt")
    var inscope exes <- cmake_list_entries(ucm, "DAS_UTILS_SHIPPED_EXES")
    var inscope built <- cmake_list_entries(ucm, "DAS_UTILS")
    var inscope built_names : table<string>
    for (b in built) {
        continue if (b |> starts_with("$"))
        built_names |> insert(base_name(b))
    }
    let iclaude = fread("install/CLAUDE.md")
    for (e in exes) {
        continue if (e |> starts_with("$"))
        if (find(e, "/") >= 0) {
            gate_finding("utils/CMakeLists.txt", "DAS_UTILS_SHIPPED_EXES entry {e} is a path — shipped exes are external-tool basenames, and utils/internal/ never ships")
            continue
        }
        let readme_md = e == "dastest" ? "dastest/README.md" : "utils/{e}/README.md"
        if (!fexist(readme_md) || find(fread(readme_md), "DAS_UTILS_SHIPPED_EXES") < 0) {
            gate_finding(readme_md, "shipped exe {e}: its own README.md must record the DAS_UTILS_SHIPPED_EXES decision")
        }
        if (find(iclaude, "{e}.exe") < 0) {
            gate_finding("install/CLAUDE.md", "bin/ line does not name {e}.exe — shipped exes are listed there")
        }
        if (e != "dastest" && !key_exists(built_names, e)) {
            gate_finding("utils/CMakeLists.txt", "shipped exe {e} has no DAS_UTILS entry — nothing builds it")
        }
    }
    for (e in exes) {
        continue if (e |> starts_with("$") || find(e, "/") >= 0 || e == "dastest")
        if (find_index(externals, e) < 0) {
            gate_finding("utils/CMakeLists.txt", "shipped exe {e} is not an external tool directory under utils/")
        }
    }
    // reverse sweep: every exe the bin/ line names is a current entry — a removed
    // tool must not leave its .exe behind
    var inscope exe_set : table<string>
    for (e in exes) {
        continue if (e |> starts_with("$"))
        exe_set |> insert(e)
    }
    for (ln in split(iclaude, "\n")) {
        continue if (!starts_with(ln, "- `bin/`"))
        var from = 0
        while (true) {
            let m = regex_search(BIN_EXE, ln, from)
            break if (m.x < 0)
            let tok = base_name(regex_group(BIN_EXE, 1, ln))
            if (!key_exists(exe_set, tok)) {
                gate_finding("install/CLAUDE.md", "bin/ line names {tok}.exe — not a current DAS_UTILS_SHIPPED_EXES entry")
            }
            from = m.y
        }
    }
}

[export]
def main() : int {
    if (!fexist("utils/REVIEW.das") || !fexist("CMakeLists.txt")) {
        to_log(LOG_ERROR, "utils/REVIEW.das: run from the repo root\n")
        return 2
    }
    var inscope children <- list_dirs("utils")
    var inscope internals <- list_dirs("utils/internal")
    var externals : array<string>   // every utils/ child that answers the external-tool rules
    var tools : array<string>       // externals minus the shared library hub
    for (n in children) {
        // main.cpp marks a C++ binary source dir (daslang, daslang-live, gen1-to-gen2) — ships via bin/
        continue if (n == "internal" || fexist("utils/{n}/main.cpp"))
        externals |> push(n)
        if (!key_exists(LIBRARY_DIRS, n)) {
            tools |> push(n)
        }
    }
    check_kebab_case(externals, internals)
    check_install_symmetry(externals)
    check_registration_triple(tools, internals)
    check_no_internal_refs()
    check_shipped_exes(externals)
    check_supervised_tools()
    return gate_verdict("utils")
}
