options gen2
options rtti
options indenting = 4
options no_global_variables = false

module commands shared public

require daslib/fio
require strings
require daslib/strings_boost
require daslib/json
require daslib/json_boost

require daslib/ansi_colors
require daslib/clargs public
require pugixml/PUGIXML_boost

require lockfile
require utils
require index
require package_runner


[CommandLineArgs]
struct DaspkgArgs {
    @clarg_positional
    @clarg_doc = "Subcommand (install / remove / list / search / ...)"
    command : Option<string>

    @clarg_positional
    @clarg_doc = "Argument to the subcommand (package name, search query, ...)"
    command_arg : Option<string>

    @clarg_doc = "Project root (default: current directory)"
    root : string = "."

    @clarg_doc = "Force reinstall"
    force : bool

    @clarg_name = "global"
    @clarg_short = "g"
    @clarg_doc = "Operate on global modules (das_root/modules/)"
    global_flag : bool

    @clarg_short = "v"
    @clarg_doc = "Print detailed progress"
    verbose : bool

    @clarg_doc = "Machine-readable JSON output (list, check, search)"
    json : bool

    @clarg_short = "b"
    @clarg_doc = "Install from a branch instead of a tag"
    branch : string

    @clarg_mutex_group = "color"
    @clarg_doc = "Enable colored output"
    color : bool

    @clarg_name = "no-color"
    @clarg_mutex_group = "color"
    @clarg_doc = "Disable colored output"
    no_color : bool

    @clarg_doc = "Output directory for `release` (default: current directory)"
    out : string = "."

    @clarg_name = "paranoid"
    @clarg_doc = "During `release`, tune with the paranoid budget (3x rounds, 1% noise gate)"
    paranoid : bool

    @clarg_name = "quick"
    @clarg_doc = "During `release`, accept a complete existing sidecar instead of re-minting — the ONLY mode that inherits"
    quick : bool

    @clarg_name = "wasm"
    @clarg_doc = "Target wasm64 (memory64): `build --wasm` builds the wasm64 runtime + module archives; `release wasm` ships compiled .wasm artifacts"
    wasm : bool

    @clarg_name = "wasm-lib-dir"
    @clarg_doc = "Directory holding the wasm64 archives (liblibDaScript_runtime.a + liblibDasModule*.a); default <das_root>/web/output64/lib"
    wasm_lib_dir : string
}

// Apply parsed flags that flip module-level globals.  Called by main after
// a successful parse so the rest of daspkg sees the right state.
def public apply_global_flags(args : DaspkgArgs) {
    if (args.verbose) {
        log_verbose_flag = true
    }
    if (args.no_color) {
        use_tty_colors = false
    } elif (args.color) {
        use_tty_colors = true
    }
}

def cmd_list(root : string; json : bool = false; is_global : bool = false) {
    var lf : LockFile
    let have_lock = is_global ? read_global_lock_file(lf) : read_lock_file(root, lf)
    if (!have_lock) {
        if (json) {
            print("[]\n")
        } else {
            print("No packages installed{is_global ? " globally" : ""} (no lock file found)\n")
        }
        return 0
    }
    if (empty(lf.packages)) {
        if (json) {
            print("[]\n")
        } else {
            print("No packages installed{is_global ? " globally" : ""}\n")
        }
        return 0
    }
    if (json) {
        print(sprint_json(lf.packages, false))
        print("\n")
        return 0
    }
    print("{is_global ? "Global packages" : "Installed packages"}:\n")
    for (entry in lf.packages) {
        var flags : string
        if (entry.local) {
            flags = " (local)"
        }
        if (entry.global && !is_global) {
            flags = "{flags} (global)"
        }
        if (!entry.root) {
            flags = "{flags} (dependency)"
        }
        if (!empty(entry.tag)) {
            print("  {entry.name} @ {entry.tag}{flags}\n")
            print("    {entry.source}\n")
        } else {
            print("  {entry.name}{flags}\n")
            print("    {entry.source}\n")
        }
    }
    return 0
}

def install_local(root, pkg_name, source : string; var lf : LockFile; is_global : bool = false) {
    var visiting : table<string; bool>
    return install_local(root, pkg_name, source, lf, visiting, [is_global = is_global])
}

def install_local(root, pkg_name, source : string; var lf : LockFile; var visiting : table<string; bool>; is_global : bool = false; is_root : bool = true) {
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let target = "{modules_dir}/{pkg_name}"
    // relative "./" or "../" paths are resolved against `root`, not the current working directory
    let is_relative = starts_with(source, "./") || starts_with(source, "../") || starts_with(source, ".\\") || starts_with(source, "..\\")
    let full_source = is_relative ? get_full_file_name("{root}/{source}") : get_full_file_name(source)
    log_debug("install_local: '{pkg_name}' from '{full_source}' -> '{target}'")

    if (!fexist(full_source)) {
        to_log(LOG_ERROR, "Error: local path '{full_source}' does not exist\n")
        return 1
    }

    mkdir(modules_dir)

    if (fexist(target)) {
        log("Removing existing {target}...\n")
        force_rmdir(target)
    }

    log("Copying {full_source} -> {target}\n")
    if (!copy_dir_rec(full_source, target)) {
        to_log(LOG_ERROR, "Error: copy failed\n")
        return 1
    }

    // install dependencies before build (matches install_git ordering)
    let dep_rc = install_dependencies(root, pkg_name, lf, visiting, is_global)
    if (dep_rc != 0) {
        log("Install failed for '{pkg_name}' — removing partial install\n")
        force_rmdir(target)
        if (has_package(lf, pkg_name)) {
            remove_package(lf, pkg_name)
            if (is_global) {
                write_global_lock_file(lf)
            } else {
                write_lock_file(root, lf)
            }
        }
        return dep_rc
    }

    let build_rc = try_build_package(target, pkg_name, is_global)
    if (build_rc != 0) {
        log("Install failed for '{pkg_name}' — removing partial install\n")
        force_rmdir(target)
        // drop any prior lock entry — the directory it referred to is gone
        if (has_package(lf, pkg_name)) {
            remove_package(lf, pkg_name)
            if (is_global) {
                write_global_lock_file(lf)
            } else {
                write_lock_file(root, lf)
            }
        }
        return build_rc
    }

    set_package(lf, PackageEntry(name = pkg_name, source = full_source, local = true, root = is_root, global = is_global))
    if (is_global) {
        write_global_lock_file(lf)
    } else {
        write_lock_file(root, lf)
    }
    log("Installed '{pkg_name}' (local{is_global ? ", global" : ""})\n")
    if (!is_global) {
        gitignore_add(root, "modules/{pkg_name}")
    }
    return 0
}

// Resolve a dependency spec to a clone-able source.
// Bare index names (e.g. "dasImgui") are looked up in the package index.
// Local paths and git URLs are returned as-is.
// Returns empty string on index-lookup failure.
def resolve_dep_source(root, source : string) : string {
    if (is_local_path(source)) return source
    if (is_index_name(source)) {
        let url = resolve_from_index(root, source)
        if (empty(url)) {
            to_log(LOG_ERROR, "Error: dependency '{source}' not found in the index\n")
            return ""
        }
        log_debug("resolved dep '{source}' -> {url}")
        return url
    }
    return source
}

def install_dependencies(root, pkg_name : string; var lf : LockFile; var visiting : table<string; bool>; is_global : bool = false) { // nolint:STYLE038
    // `visiting` is a STACK of pkgs currently in flight on the install chain, not
    // a "visited" set — we erase pkg_name before returning so a sibling dep
    // (diamond graph) sees it via has_package, not via the cycle path.
    if (key_exists(visiting, pkg_name)) {
        log("Warning: dependency cycle detected at '{pkg_name}' — skipping\n")
        return 0
    }
    visiting |> insert(pkg_name, true)
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let pkg_dir = "{modules_dir}/{pkg_name}"
    let das_package = "{pkg_dir}/.das_package"
    var rc = 0
    if (fexist(das_package)) {
        var pkg_deps : array<PackageDependency>
        if (run_das_package_deps(das_package, "", pkg_deps)) {
            log_debug("{pkg_name} has {length(pkg_deps)} dependencies")
            for (d in pkg_deps) {
                let dep_name = package_name_from_source(d.source)
                // cycle: dep is already being installed up the call stack — skip
                if (key_exists(visiting, dep_name)) {
                    log("Dependency cycle detected at '{dep_name}' — skipping\n")
                    continue
                }
                if (has_package(lf, dep_name)) {
                    let existing = get_package(lf, dep_name)
                    let dep_dir = existing.global ? "{get_das_root()}/modules/{dep_name}" : "{modules_dir}/{dep_name}"
                    if (fexist(dep_dir)) {
                        log_debug("dependency '{dep_name}' already installed — skipping")
                        continue
                    }
                    log("Found stale lock entry for dependency '{dep_name}' (directory missing) — reinstalling\n")
                    remove_package(lf, dep_name)
                    if (is_global) {
                        write_global_lock_file(lf)
                    } else {
                        write_lock_file(root, lf)
                    }
                }
                // when global, also skip if directory already exists (built-in SDK modules)
                if (is_global && fexist("{modules_dir}/{dep_name}")) {
                    log_debug("dependency '{dep_name}' already present in {modules_dir} — skipping")
                    continue
                }
                // An IN-TREE module (part of the daslang source/SDK tree, e.g.
                // dasImgui) satisfies the dep in any project — the compiled host
                // already carries it; a project-local clone would only shadow it.
                // Discriminated from a daspkg GLOBAL install at the same path by
                // the global lock file (global installs are recorded there;
                // tracked in-tree modules never are) — those keep the documented
                // local-shadows-global behavior.
                if (!is_global && fexist("{get_das_root()}/modules/{dep_name}")) {
                    var glf : LockFile
                    read_global_lock_file(glf)
                    if (!has_package(glf, dep_name)) {
                        log("  Dependency '{dep_name}' is part of this daslang tree — skipping install\n")
                        continue
                    }
                }
                log("  Installing dependency: {dep_name}...\n")
                let dep_source = resolve_dep_source(root, d.source)
                if (empty(dep_source)) {
                    rc = 1
                    continue
                }
                var drc = 0
                if (is_local_path(dep_source)) {
                    drc = install_local(root, dep_name, dep_source, lf, visiting, [is_global = is_global, is_root = false])
                } else {
                    drc = install_git(root, dep_name, dep_source, "", lf, visiting, [is_root = false, is_global = is_global])
                }
                if (drc != 0) {
                    rc = drc
                }
            }
        } else {
            log_debug("{pkg_name}: no dependencies declared")
        }
    }
    visiting |> erase(pkg_name)
    return rc
}


def private git_checkout_tag(dir, tag : string) : bool {
    var output : string
    var exit_code = run_cmd("git -C \"{dir}\" fetch --depth 1 origin tag \"{tag}\"", output)
    if (exit_code != 0) {
        to_log(LOG_ERROR, "Error: git fetch tag failed (exit {exit_code}):\n{output}\n")
        return false
    }
    exit_code = run_cmd("git -C \"{dir}\" checkout \"{tag}\"", output)
    if (exit_code != 0) {
        to_log(LOG_ERROR, "Error: git checkout failed (exit {exit_code}):\n{output}\n")
        return false
    }
    return true
}

def private git_checkout_branch(dir, branch : string) : bool {
    var output : string
    var exit_code = run_cmd("git -C \"{dir}\" fetch --depth 1 origin \"{branch}\"", output)
    if (exit_code != 0) {
        to_log(LOG_ERROR, "Error: git fetch branch failed (exit {exit_code}):\n{output}\n")
        return false
    }
    exit_code = run_cmd("git -C \"{dir}\" checkout -B \"{branch}\" FETCH_HEAD", output)
    if (exit_code != 0) {
        to_log(LOG_ERROR, "Error: git checkout failed (exit {exit_code}):\n{output}\n")
        return false
    }
    return true
}

def private git_clone(url, dir : string) : bool {
    var output : string
    let exit_code = run_cmd("git clone --depth 1 \"{url}\" \"{dir}\"", output)
    if (exit_code != 0) {
        to_log(LOG_ERROR, "Error: git clone failed (exit {exit_code}):\n{output}\n")
        return false
    }
    return true
}

def install_git(root, pkg_name, source, version : string; var lf : LockFile; is_root : bool = true; is_global : bool = false; branch : string = "") {
    var visiting : table<string; bool>
    return install_git(root, pkg_name, source, version, lf, visiting, [is_root = is_root, is_global = is_global, branch = branch])
}

def install_git(root, pkg_name, source, version : string; var lf : LockFile; var visiting : table<string; bool>; is_root : bool = true; is_global : bool = false; branch : string = "") { // nolint:STYLE037,STYLE038 — the install ladder: clone/resolve/checkout/move/lock/deps/build arms are one sequential flow
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let tmp_dir = "{modules_dir}/.daspkg_tmp/{pkg_name}"
    let target_dir = "{modules_dir}/{pkg_name}"

    mkdir(modules_dir)
    mkdir("{modules_dir}/.daspkg_tmp")

    // clean stale tmp dir from a previous failed install
    if (fexist(tmp_dir)) {
        force_rmdir(tmp_dir)
    }

    // step 1: shallow clone default branch
    let git_url = "https://{source}.git"
    log("Cloning {git_url}...\n")
    if (!git_clone(git_url, tmp_dir)) return 1

    // step 2: resolve version via .das_package (if present)
    log_debug("resolving version for '{pkg_name}' (requested: '{version}' branch: '{branch}')")
    var actual_source = source
    var actual_tag = ""
    var actual_branch = ""
    if (!empty(branch)) {
        // explicit --branch overrides version/resolve
        log("Switching to branch {branch}...\n")
        if (!git_checkout_branch(tmp_dir, branch)) {
            force_rmdir(tmp_dir)
            return 1
        }
        actual_branch = branch
    }
    let das_package = "{tmp_dir}/.das_package"
    if (empty(actual_branch) && fexist(das_package)) {
        log_debug("found .das_package — running resolve()")
        var res : ResolveResult
        if (run_das_package_resolve(das_package, "", version, res)) {
            log_debug("resolve result: tag='{res.tag}' branch='{res.branch}' source='{res.source}'")
            if (!empty(res.source)) {
                // redirect — re-clone from a different repo
                log("Redirecting to {res.source}...\n")
                force_rmdir(tmp_dir)
                actual_source = res.source
                let redirect_url = "https://{res.source}.git"
                if (!git_clone(redirect_url, tmp_dir)) return 1
                // checkout the ref from the redirect
                if (!empty(res.tag)) {
                    log("Fetching tag {res.tag}...\n")
                    if (!git_checkout_tag(tmp_dir, res.tag)) {
                        force_rmdir(tmp_dir)
                        return 1
                    }
                    actual_tag = res.tag
                } elif (!empty(res.branch)) {
                    log("Switching to branch {res.branch}...\n")
                    if (!git_checkout_branch(tmp_dir, res.branch)) {
                        force_rmdir(tmp_dir)
                        return 1
                    }
                    actual_branch = res.branch
                }
            } elif (!empty(res.tag)) {
                // tag checkout
                log("Fetching tag {res.tag}...\n")
                if (!git_checkout_tag(tmp_dir, res.tag)) {
                    force_rmdir(tmp_dir)
                    return 1
                }
                actual_tag = res.tag
            } elif (!empty(res.branch)) {
                // branch checkout
                log("Switching to branch {res.branch}...\n")
                if (!git_checkout_branch(tmp_dir, res.branch)) {
                    force_rmdir(tmp_dir)
                    return 1
                }
                actual_branch = res.branch
            } elif (!empty(version)) {
                // resolve returned nothing — fall back to version as tag
                log("Fetching tag {version}...\n")
                if (!git_checkout_tag(tmp_dir, version)) {
                    // try with v prefix
                    log("Fetching tag v{version}...\n")
                    if (!git_checkout_tag(tmp_dir, "v{version}")) {
                        force_rmdir(tmp_dir)
                        return 1
                    }
                    actual_tag = "v{version}"
                } else {
                    actual_tag = version
                }
            }
        }
    }
    if (empty(actual_tag) && empty(actual_branch) && !empty(version)) {
        // no .das_package or resolve failed — treat version as tag directly
        log("Fetching tag {version}...\n")
        if (!git_checkout_tag(tmp_dir, version)) {
            // try with v prefix
            log("Fetching tag v{version}...\n")
            if (!git_checkout_tag(tmp_dir, "v{version}")) {
                force_rmdir(tmp_dir)
                return 1
            }
            actual_tag = "v{version}"
        } else {
            actual_tag = version
        }
    }

    // step 3: move to final location
    log_debug("moving {tmp_dir} -> {target_dir}")
    if (fexist(target_dir)) {
        log("Removing existing {target_dir}...\n")
        force_rmdir(target_dir)
    }
    if (!rename(tmp_dir, target_dir)) {
        to_log(LOG_ERROR, "Error: failed to move {tmp_dir} to {target_dir}\n")
        return 1
    }

    // step 4: install dependencies
    log_debug("checking dependencies for '{pkg_name}'")
    let dep_rc = install_dependencies(root, pkg_name, lf, visiting, is_global)
    if (dep_rc != 0) {
        log("Install failed for '{pkg_name}' — removing partial install\n")
        force_rmdir(target_dir)
        // only touch the lock file if we'd actually be removing a prior entry
        if (has_package(lf, pkg_name)) {
            remove_package(lf, pkg_name)
            if (is_global) {
                write_global_lock_file(lf)
            } else {
                write_lock_file(root, lf)
            }
        }
        return dep_rc
    }

    // step 5: build
    log_debug("checking build for '{pkg_name}'")
    let build_rc = try_build_package(target_dir, pkg_name, is_global)
    if (build_rc != 0) {
        log("Install failed for '{pkg_name}' — removing partial install\n")
        force_rmdir(target_dir)
        if (has_package(lf, pkg_name)) {
            remove_package(lf, pkg_name)
            if (is_global) {
                write_global_lock_file(lf)
            } else {
                write_lock_file(root, lf)
            }
        }
        return build_rc
    }

    // step 6: record in lock file
    set_package(lf, PackageEntry(name = pkg_name, source = actual_source, version = version,
        tag = actual_tag, branch = actual_branch, root = is_root, global = is_global))
    if (is_global) {
        write_global_lock_file(lf)
    } else {
        write_lock_file(root, lf)
    }
    log("Installed '{pkg_name}'{is_global ? " (global)" : ""}\n")

    // step 7: add to .gitignore (local installs only)
    if (!is_global) {
        gitignore_add(root, "modules/{pkg_name}")
    }

    return 0
}

def cmd_install(root, spec : string; force : bool; is_global : bool = false; version_constraint : string = ""; branch : string = "") { // nolint:STYLE037,STYLE038 — flat source-kind dispatch (index name / url / local path / global) + its error arms
    // parse spec: name[@version]
    var source = spec
    var version = ""
    let at_pos = find(spec, "@")
    if (at_pos >= 0) {
        source = slice(spec, 0, at_pos)
        version = slice(spec, at_pos + 1)
        // strip leading "v"/"V" — version is a semver number, not a git tag
        // the .das_package resolve() function maps version -> tag (e.g. "1.0" -> "v1.0")
        if (starts_with(version, "v") || starts_with(version, "V")) {
            version = slice(version, 1)
        }
    }

    let pkg_name = package_name_from_source(source)
    if (empty(pkg_name)) {
        to_log(LOG_ERROR, "Error: cannot determine package name from '{source}'\n")
        return 1
    }

    log_debug("cmd_install: pkg='{pkg_name}' source='{source}' version='{version}' global={is_global}")

    if (is_global) {
        // global install — read global lock file, install to das_root/modules/
        var glf : LockFile
        read_global_lock_file(glf)
        if (!force && has_package(glf, pkg_name)) {
            let global_dir = "{get_das_root()}/modules/{pkg_name}"
            if (fexist(global_dir)) {
                log("'{pkg_name}' is already installed globally (use --force to reinstall)\n")
                return 0
            }
            log("Found stale global lock entry for '{pkg_name}' (directory missing) — reinstalling\n")
            remove_package(glf, pkg_name)
            write_global_lock_file(glf)
        }
        // also skip if directory already exists (built-in SDK modules)
        let global_dir = "{get_das_root()}/modules/{pkg_name}"
        if (!force && fexist(global_dir) && !has_package(glf, pkg_name)) {
            log("'{pkg_name}' already present in {get_das_root()}/modules/ — skipping\n")
            return 0
        }
        if (is_local_path(source)) {
            return install_local(root, pkg_name, source, glf, [is_global = true])
        } elif (is_index_name(source)) {
            let url = resolve_from_index(root, source)
            if (empty(url)) {
                to_log(LOG_ERROR, "Error: package '{source}' not found in the index\n")
                return 1
            }
            log("Resolved '{source}' -> {url}\n")
            var visiting : table<string; bool>
            return install_git(root, pkg_name, url, version, glf, visiting, [is_root = true, is_global = true, branch = branch])
        } else {
            var visiting : table<string; bool>
            return install_git(root, pkg_name, source, version, glf, visiting, [is_root = true, is_global = true, branch = branch])
        }
    }

    // local install — check global first
    var lf : LockFile
    read_lock_file(root, lf)
    if (!force && has_package(lf, pkg_name)) {
        let existing = get_package(lf, pkg_name)
        let pkg_dir = existing.global ? "{get_das_root()}/modules/{pkg_name}" : "{root}/modules/{pkg_name}"
        if (fexist(pkg_dir)) {
            log("'{pkg_name}' is already installed (use --force to reinstall)\n")
            return 0
        }
        log("Found stale lock entry for '{pkg_name}' (directory missing) — reinstalling\n")
        remove_package(lf, pkg_name)
        write_lock_file(root, lf)
    }

    // An IN-TREE module (part of the daslang source/SDK tree, e.g. dasImgui)
    // needs no install anywhere — the compiled host already carries it.
    // Discriminated from a daspkg global install by the global lock file
    // (see the same rule in install_dependencies).
    var glf0 : LockFile
    read_global_lock_file(glf0)
    if (!force && fexist("{get_das_root()}/modules/{pkg_name}") && !has_package(glf0, pkg_name)) {
        log("'{pkg_name}' is part of this daslang tree — nothing to install\n")
        return 0
    }

    // check if a compatible global version exists
    var glf : LockFile
    if (read_global_lock_file(glf) && has_package(glf, pkg_name)) {
        let ge = get_package(glf, pkg_name)
        // use version_constraint (from .das_package deps) if available, otherwise version (from @spec)
        let constraint = !empty(version_constraint) ? version_constraint : version
        if (empty(constraint) || satisfies_constraint(ge.tag, constraint)) {
            // compatible global version — use it
            set_package(lf, PackageEntry(name = pkg_name, source = ge.source, version = ge.version,
                tag = ge.tag, branch = ge.branch, root = true, global = true))
            write_lock_file(root, lf)
            log("Using global '{pkg_name}' @ {ge.tag}\n")
            return 0
        } else {
            // version mismatch
            to_log(LOG_ERROR, "Error: global '{pkg_name}' is {ge.tag}, but project requires {constraint}\n")
            log("  Use --force to install locally, or: daspkg install --global {source}@{constraint}\n")
            if (!force) return 1
            // --force: fall through to local install
        }
    }

    if (is_local_path(source)) {
        return install_local(root, pkg_name, source, lf)
    } elif (is_index_name(source)) {
        // resolve from index
        log("Installing {source} from {source}...\n")
        let url = resolve_from_index(root, source)
        if (empty(url)) {
            to_log(LOG_ERROR, "Error: package '{source}' not found in the index\n")
            return 1
        }
        log("Resolved '{source}' -> {url}\n")
        var visiting : table<string; bool>
        return install_git(root, pkg_name, url, version, lf, visiting, [is_root = true, branch = branch])
    } else {
        var visiting : table<string; bool>
        return install_git(root, pkg_name, source, version, lf, visiting, [is_root = true, branch = branch])
    }
}

def cmd_install_all(root : string; force : bool; is_global : bool = false) {
    let das_package = "{root}/.das_package"
    if (!fexist(das_package)) {
        to_log(LOG_ERROR, "No .das_package found\n")
        return 1
    }
    var pkg_deps : array<PackageDependency>
    if (!run_das_package_deps(das_package, "", pkg_deps) || empty(pkg_deps)) {
        to_log(LOG_ERROR, "No dependencies listed in .das_package\n")
        return 1
    }
    var rc = 0
    for (d in pkg_deps) {
        // append version constraint as @version if present (e.g. "=1.0" -> "@1.0")
        var spec = d.source
        if (!empty(d.version_constraint)) {
            // strip leading operator for git version (=1.0 -> 1.0, >=1.0 -> 1.0)
            var ver = d.version_constraint
            if (starts_with(ver, ">=") || starts_with(ver, "<=")) {
                ver = slice(ver, 2)
            } elif (starts_with(ver, ">") || starts_with(ver, "<") || starts_with(ver, "=")) {
                ver = slice(ver, 1)
            }
            spec = "{spec}@{ver}"
        }
        let drc = cmd_install(root, spec, force, is_global, d.version_constraint)
        if (drc != 0) {
            rc = drc
        }
    }
    return rc
}

def cmd_remove(root, name : string; is_global : bool = false) {
    if (is_global) {
        var glf : LockFile
        if (!read_global_lock_file(glf)) {
            to_log(LOG_ERROR, "Error: no global lock file found\n")
            return 1
        }
        if (!has_package(glf, name)) {
            to_log(LOG_ERROR, "Error: package '{name}' is not installed globally\n")
            return 1
        }
        let pkg_dir = "{get_das_root()}/modules/{name}"
        if (fexist(pkg_dir)) {
            log("Removing {pkg_dir}...\n")
            force_rmdir(pkg_dir)
        }
        remove_package(glf, name)
        write_global_lock_file(glf)
        log("Removed '{name}' (global)\n")
        return 0
    }
    var lf : LockFile
    if (!read_lock_file(root, lf)) {
        to_log(LOG_ERROR, "Error: no lock file found\n")
        return 1
    }
    if (!has_package(lf, name)) {
        to_log(LOG_ERROR, "Error: package '{name}' is not installed\n")
        return 1
    }
    let entry = get_package(lf, name)
    if (entry.global) {
        // only remove the project reference, not the global directory
        remove_package(lf, name)
        write_lock_file(root, lf)
        log("Removed global reference to '{name}'. Global package still installed.\n")
        return 0
    }

    let pkg_dir = "{root}/modules/{name}"
    if (fexist(pkg_dir)) {
        log("Removing {pkg_dir}...\n")
        force_rmdir(pkg_dir)
    }

    remove_package(lf, name)
    write_lock_file(root, lf)
    gitignore_remove(root, "modules/{name}")
    log("Removed '{name}'\n")
    return 0
}

def cmd_update(root, name : string; is_global : bool = false) { // nolint:STYLE037,STYLE038 — flat per-entry update arms (root/dep/local/global) mirror the lock schema
    var lf : LockFile
    if (is_global) {
        if (!read_global_lock_file(lf)) {
            to_log(LOG_ERROR, "Error: no global lock file found\n")
            return 1
        }
    } else {
        if (!read_lock_file(root, lf)) {
            to_log(LOG_ERROR, "Error: no lock file found\n")
            return 1
        }
    }
    if (!has_package(lf, name)) {
        to_log(LOG_ERROR, "Error: package '{name}' is not installed{is_global ? " globally" : ""}\n")
        return 1
    }
    let entry = get_package(lf, name)
    if (!is_global && entry.global) {
        log("'{name}' is a global package — use 'daspkg update --global {name}'\n")
        return 0
    }
    if (entry.local) {
        // re-copy from original source path
        let source = entry.source
        if (empty(source) || !fexist(source)) {
            to_log(LOG_ERROR, "Error: local source '{source}' not found\n")
            return 1
        }
        let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
        let target = "{modules_dir}/{name}"
        log("Updating '{name}' from {source}...\n")
        if (fexist(target)) {
            force_rmdir(target)
        }
        if (!copy_dir_rec(source, target)) {
            to_log(LOG_ERROR, "Error: copy failed\n")
            return 1
        }
        log("Updated '{name}'\n")
        return try_build_package(target, name, is_global)
    }
    // re-install from source with same version — re-resolves via .das_package
    let source = entry.source
    let version = entry.version
    let is_root = entry.root
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let pkg_dir = "{modules_dir}/{name}"
    let backup_dir = "{modules_dir}/.daspkg_backup/{name}"
    log("Updating '{name}'{is_global ? " (global)" : ""}...\n")
    // backup before deleting
    log_debug("backing up '{name}' to {backup_dir}")
    if (fexist(pkg_dir)) {
        mkdir("{modules_dir}/.daspkg_backup")
        if (fexist(backup_dir)) {
            force_rmdir(backup_dir)
        }
        if (!rename(pkg_dir, backup_dir)) {
            to_log(LOG_ERROR, "Error: failed to backup '{name}'\n")
            return 1
        }
    }
    remove_package(lf, name)
    var visiting : table<string; bool>
    let install_rc = install_git(root, name, source, version, lf, visiting, [is_root = is_root, is_global = is_global])
    // check if install succeeded
    if (!has_package(lf, name)) {
        log_debug("install failed — restoring from {backup_dir}")
        log("Update failed — restoring backup\n")
        if (fexist(backup_dir)) {
            rename(backup_dir, pkg_dir)
        }
        set_package(lf, entry)
        if (is_global) {
            write_global_lock_file(lf)
        } else {
            write_lock_file(root, lf)
        }
        return 1
    }
    // cleanup backup
    if (fexist(backup_dir)) {
        force_rmdir(backup_dir)
    }
    return install_rc
}

// resolve latest version for a git package by cloning default branch and running .das_package
// returns the resolved tag/branch, or empty if resolution fails
def private resolve_latest(source : string; var res : ResolveResult) : bool {
    let tmp_dir = "{get_das_root()}/_daspkg_resolve_tmp"
    if (fexist(tmp_dir)) {
        force_rmdir(tmp_dir)
    }
    let git_url = "https://{source}.git"
    if (!git_clone(git_url, tmp_dir)) return false
    let das_package = "{tmp_dir}/.das_package"
    var ok = false
    if (fexist(das_package)) {
        ok = run_das_package_resolve(das_package, "", "", res)
    }
    force_rmdir(tmp_dir)
    return ok
}

def cmd_upgrade(root, name : string; is_global : bool = false) {
    var lf : LockFile
    if (is_global) {
        if (!read_global_lock_file(lf)) {
            to_log(LOG_ERROR, "Error: no global lock file found\n")
            return 1
        }
    } else {
        if (!read_lock_file(root, lf)) {
            to_log(LOG_ERROR, "Error: no lock file found\n")
            return 1
        }
    }
    if (!has_package(lf, name)) {
        to_log(LOG_ERROR, "Error: package '{name}' is not installed{is_global ? " globally" : ""}\n")
        return 1
    }
    let entry = get_package(lf, name)
    if (!is_global && entry.global) {
        log("'{name}' is a global package — use 'daspkg upgrade --global {name}'\n")
        return 0
    }
    if (entry.local) {
        log("'{name}' is a local package — use 'update' instead\n")
        return 0
    }
    // resolve latest from default branch
    log_debug("resolving latest version for '{name}' from {entry.source}")
    log("Checking '{name}' for upgrades...\n")
    var res : ResolveResult
    if (!resolve_latest(entry.source, res)) {
        log("  Could not resolve latest version for '{name}'\n")
        return 0
    }
    // determine resolved ref
    let new_tag = res.tag
    let new_branch = res.branch
    // compare with current
    let current_ref = !empty(entry.tag) ? entry.tag : entry.branch
    let new_ref = !empty(new_tag) ? new_tag : new_branch
    if (current_ref == new_ref) {
        log("  '{name}' is already at {current_ref}\n")
        return 0
    }
    log("  {current_ref} -> {new_ref}\n")
    // re-install at the new version (empty version = unpinned)
    let is_root = entry.root
    let source = entry.source
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let pkg_dir = "{modules_dir}/{name}"
    if (fexist(pkg_dir)) {
        force_rmdir(pkg_dir)
    }
    remove_package(lf, name)
    var visiting : table<string; bool>
    let install_rc = install_git(root, name, source, "", lf, visiting, [is_root = is_root, is_global = is_global])
    // check if deps need upgrading
    upgrade_dependencies(root, name, lf, is_global)
    return install_rc
}

def upgrade_dependencies(root, pkg_name : string; var lf : LockFile; is_global : bool = false) {
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    let pkg_dir = "{modules_dir}/{pkg_name}"
    let das_package = "{pkg_dir}/.das_package"
    if (!fexist(das_package)) return
    var pkg_deps : array<PackageDependency>
    if (!run_das_package_deps(das_package, "", pkg_deps)) return
    for (d in pkg_deps) {
        let dep_name = package_name_from_source(d.source)
        if (!has_package(lf, dep_name)) {
            // new dependency — install it
            log("  Installing new dependency: {dep_name}...\n")
            let dep_source = resolve_dep_source(root, d.source)
            if (empty(dep_source)) continue
            var visiting : table<string; bool>
            let drc = install_git(root, dep_name, dep_source, "", lf, visiting, [is_root = false, is_global = is_global])
            if (drc != 0) {
                to_log(LOG_ERROR, "Error: failed to install new dependency '{dep_name}' (rc={drc})\n")
            }
            continue
        }
        // check if installed version satisfies constraint
        if (empty(d.version_constraint)) continue
        let dep_entry = get_package(lf, dep_name)
        let dep_version = dep_entry.tag
        if (!satisfies_constraint(dep_version, d.version_constraint)) {
            log("  Dependency '{dep_name}' @ {dep_version} does not satisfy {d.version_constraint} — upgrading...\n")
            cmd_upgrade(root, dep_name, is_global)
        }
    }
}

def cmd_upgrade_all(root : string; is_global : bool = false) {
    var lf : LockFile
    if (is_global) {
        if (!read_global_lock_file(lf)) {
            print("No packages installed globally\n")
            return 0
        }
    } else {
        if (!read_lock_file(root, lf)) {
            print("No packages installed (no lock file found)\n")
            return 0
        }
    }
    // upgrade root packages only — deps get pulled along
    let names <- [for (entry in lf.packages); entry.name; where entry.root && !entry.local]
    if (empty(names)) {
        print("No upgradeable packages\n")
        return 0
    }
    var rc = 0
    for (name in names) {
        let drc = cmd_upgrade(root, name, is_global)
        if (drc != 0) {
            rc = drc
        }
    }
    return rc
}

def cmd_update_all(root : string; is_global : bool = false) {
    var lf : LockFile
    if (is_global) {
        if (!read_global_lock_file(lf)) {
            print("No packages installed globally\n")
            return 0
        }
    } else {
        if (!read_lock_file(root, lf)) {
            print("No packages installed (no lock file found)\n")
            return 0
        }
    }
    // collect names first — cmd_update modifies lf.packages
    var names : array<string>
    names |> reserve(length(lf.packages))
    for (entry in lf.packages) {
        names |> push(entry.name)
    }
    var rc = 0
    for (name in names) {
        let drc = cmd_update(root, name, is_global)
        if (drc != 0) {
            rc = drc
        }
    }
    return rc
}

def build_package(pkg_dir : string) : bool {
    let build_dir = "{pkg_dir}/_build"
    let das_root = get_das_root()

    // Configure — pass DASLANG_DIR so the package can find headers and libs
    var output : string
    var exit_code = run_cmd("cmake -B \"{build_dir}\" -S \"{pkg_dir}\" -DDASLANG_DIR=\"{das_root}\" -DCMAKE_BUILD_TYPE=Release", output)
    if (exit_code != 0) {
        log("  CMake configure FAILED:\n{output}\n")
        return false
    }

    // Build
    exit_code = run_cmd("cmake --build \"{build_dir}\" --config Release --parallel", output)
    if (exit_code != 0) {
        log("  CMake build FAILED:\n{output}\n")
        return false
    }
    return true
}

def cmd_build(root : string; is_global : bool = false) {
    let modules_dir = is_global ? "{get_das_root()}/modules" : "{root}/modules"
    if (!fexist(modules_dir)) {
        print("No modules directory found\n")
        return 0
    }
    var built = 0
    var failed = 0
    dir(modules_dir) $(name) {
        if (name == "." || name == ".." || starts_with(name, ".")) return
        let pkg_dir = "{modules_dir}/{name}"
        if (fexist("{pkg_dir}/CMakeLists.txt")) {
            log("Building '{name}'...\n")
            if (build_package(pkg_dir)) {
                built ++
            } else {
                failed ++
            }
        }
    }
    if (built == 0 && failed == 0) {
        print("No packages with CMakeLists.txt found\n")
    } else {
        print("Build complete: {built} succeeded, {failed} failed\n")
    }
    if (failed > 0) return 1
    return 0
}

// `daspkg build --wasm` — build the wasm64 (memory64) runtime + module archives
// that `daspkg release wasm` links against, and consolidate them into <lib_dir>
// (default <das_root>/web/output64/lib). Requires an activated emscripten SDK
// (emcc + cmake on PATH); CI activates emsdk_env before invoking.
def cmd_build_wasm(_root : string; lib_dir_override : string) : int { // nolint:STYLE038 — the wasm build recipe: flat tool-invocation steps, one per stage
    let das_root = get_das_root()
    let lib_dir = empty(lib_dir_override) ? "{das_root}/web/output64/lib" : lib_dir_override
    let build_dir = "{das_root}/web/build64"
    let out_dir = "{das_root}/web/output64"

    if (!check_tool("emcc", get_platform_name() == "windows" ? "emcc.bat --version" : "emcc --version")) {
        to_log(LOG_ERROR, "build --wasm: emcc not found on PATH. Activate the emscripten SDK (emsdk_env) first, then retry.\n")
        return 1
    }

    // Configure web/build64 for memory64 + threads if it isn't already. DAS_WASM_PTHREADS makes
    // new_thread / JobQue / parallel_for map to real Web Workers (the runtime caps the pool at
    // DAS_MAX_HW_JOBS so a 64-core box does not spawn 64 workers). The released app is linked
    // -pthread to match, and must be served cross-origin-isolated (COOP/COEP) to instantiate.
    if (!fexist("{build_dir}/CMakeCache.txt")) {
        log("build --wasm: configuring {build_dir} (memory64 + threads)...\n")
        let cfg = "emcmake cmake -S \"{das_root}/web\" -B \"{build_dir}\" -G Ninja -DCMAKE_BUILD_TYPE=Release -DDAS_WEB_OUTPUT_DIR=\"{out_dir}\" -DDAS_WASM_MEMORY64=ON -DDAS_WASM_PTHREADS=ON -DCMAKE_CXX_FLAGS=-sMEMORY64=1 -DCMAKE_C_FLAGS=-sMEMORY64=1"
        var cfg_out : string
        let cfg_rc = run_cmd(cfg, cfg_out)
        if (cfg_rc != 0) {
            to_log(LOG_ERROR, "build --wasm: cmake configure failed (rc={cfg_rc}):\n{cfg_out}\n")
            return 1
        }
    }

    // Build the wasm64 runtime + the module archives release wasm links against.
    log("build --wasm: building wasm64 archives...\n")
    let targets = "libDaScript_runtime libDasModuleOpenGL libDasModuleGlfw libDasModuleStbImage libDasModuleAudio libDasModuleMinfft libDasModuleLiveHost libDasModuleClipboard"
    var bld_out : string
    let bld_rc = run_cmd("cmake --build \"{build_dir}\" --target {targets}", bld_out)
    if (bld_rc != 0) {
        to_log(LOG_ERROR, "build --wasm: archive build failed (rc={bld_rc}):\n{bld_out}\n")
        return 1
    }

    // Consolidate every produced archive into one canonical lib_dir so
    // `release wasm` resolves them in one place. The web build splits outputs:
    // the runtime lands in web/output64/lib (DAS_WEB_OUTPUT_DIR), the module
    // archives in <das_root>/lib. Copying both into lib_dir also insulates the
    // release inputs from a later desktop build that overwrites <das_root>/lib
    // with native-ABI archives.
    mkdir_rec(lib_dir)
    var copied = 0
    var copy_failed = false
    // Runtime archive (web/output64/lib via DAS_WEB_OUTPUT_DIR).
    let rt_src = "{out_dir}/lib/liblibDaScript_runtime.a"
    if (fexist(rt_src)) {
        let rt_dst = path_join(lib_dir, "liblibDaScript_runtime.a")
        if (get_full_file_name(rt_src) == get_full_file_name(rt_dst)) {
            copied ++
        } else {
            var err : string
            if (copy_file(rt_src, rt_dst, true, err)) {
                copied ++
                log("  staged liblibDaScript_runtime.a\n")
            } else {
                to_log(LOG_ERROR, "build --wasm: copy_file({rt_src} -> {rt_dst}) failed: {err}\n")
                copy_failed = true
            }
        }
    }
    // Module archives (<das_root>/lib).
    glob("{das_root}/lib", "**/liblibDasModule*.a") $(rel_path, is_dir) {
        if (is_dir) return
        let base = base_name(rel_path)
        let src = path_join("{das_root}/lib", rel_path)
        let dst = path_join(lib_dir, base)
        if (get_full_file_name(src) == get_full_file_name(dst)) {
            copied ++
            return
        }
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "build --wasm: copy_file({src} -> {dst}) failed: {err}\n")
            copy_failed = true
            return
        }
        copied ++
        log("  staged {base}\n")
    }
    if (copy_failed) return 1
    if (copied == 0) {
        to_log(LOG_ERROR, "build --wasm: build succeeded but no archives found under {build_dir}\n")
        return 1
    }
    log("build --wasm: {copied} archive(s) -> {lib_dir}\n")
    return 0
}

def check_tool(name, cmd : string) : bool {
    var output : string
    let exit_code = run_cmd("{cmd}", output)
    if (exit_code == 0) {
        let trimmed = strip(output)
        if (!empty(trimmed)) {
            let nl = find(trimmed, "\n")
            let version_line = nl >= 0 ? slice(trimmed, 0, nl) : trimmed
            print("  OK: {name} — {version_line}\n")
        } else {
            print("  OK: {name}\n")
        }
        return true
    } else {
        print("  MISSING: {name} — not found in PATH\n")
        return false
    }
}

def cmd_doctor() {
    print("Checking environment...\n")
    var ok = true
    if (!check_tool("git", "git --version")) {
        ok = false
    }
    if (!check_tool("cmake", "cmake --version")) {
        ok = false
    }
    if (!check_tool("rm", "rm --version")) {
        // rm is required for force_rmdir (removing git repos with read-only objects)
        print("    (requires MSYS2, Git Bash, or Unix-like shell)\n")
        ok = false
    }
    if (!check_tool("gh", "gh --version")) {
        // gh is optional — only needed for introduce/withdraw
        print("    (optional — only needed for introduce/withdraw)\n")
    }
    if (ok) {
        print("Environment OK\n")
    } else {
        print("Some required tools are missing\n")
        return 1
    }
    return 0
}

def try_build_package(pkg_dir, name : string; is_global : bool = false) : int {
    var did_build = false
    var rc = 0
    let das_package = "{pkg_dir}/.das_package"
    if (fexist(das_package)) {
        var info : PackageBuildInfo
        if (run_das_package_build(das_package, info)) {
            if (info.is_cmake) {
                log("Building '{name}'...\n")
                if (!build_package(pkg_dir)) {
                    to_log(LOG_ERROR, "Error: build failed for '{name}'\n")
                    rc = 1
                } else {
                    did_build = true
                }
            } elif (info.is_custom && !empty(info.command)) {
                log("Building '{name}' (custom)...\n")
                var output : string
                let exit_code = run_cmd("cd \"{pkg_dir}\" && {info.command}", output)
                if (exit_code != 0) {
                    to_log(LOG_ERROR, "Error: custom build failed for '{name}':\n{output}\n")
                    rc = 1
                } else {
                    did_build = true
                }
            }
        }
    } else {
        // fallback: check for CMakeLists.txt directly
        if (fexist("{pkg_dir}/CMakeLists.txt")) {
            log("Building '{name}'...\n")
            if (!build_package(pkg_dir)) {
                to_log(LOG_ERROR, "Error: build failed for '{name}'\n")
                rc = 1
            } else {
                did_build = true
            }
        }
    }
    // mark standalone-build modules so main CMake skips them
    if (is_global && did_build) {
        fopen("{pkg_dir}/.daspkg_standalone", "wb") $(f) {
            if (f != null) {
                fwrite(f, "")
            }
        }
    }
    return rc
}

struct CheckResult {
    name : string
    status : string
    message : string
}

def cmd_check(root : string; json : bool = false; is_global : bool = false) {
    var lf : LockFile
    if (is_global) {
        if (!read_global_lock_file(lf)) {
            if (json) {
                print("[]\n")
            } else {
                print("No packages installed globally\n")
            }
            return 0
        }
    } else {
        if (!read_lock_file(root, lf)) {
            if (json) {
                print("[]\n")
            } else {
                print("No packages installed\n")
            }
            return 0
        }
    }
    var results : array<CheckResult>
    var ok = true
    for (entry in lf.packages) {
        // global entries live at das_root/modules/, local at root/modules/
        let pkg_dir = (entry.global || is_global) ? "{get_das_root()}/modules/{entry.name}" : "{root}/modules/{entry.name}"
        if (!fexist(pkg_dir)) {
            let location = entry.global ? " (global)" : ""
            results |> emplace(CheckResult(name = entry.name, status = "missing", message = "directory not found{location}"))
            ok = false
            continue
        }
        let has_module = fexist("{pkg_dir}/.das_module")
        if (!has_module) {
            results |> emplace(CheckResult(name = entry.name, status = "warning", message = "no .das_module"))
        } else {
            results |> emplace(CheckResult(name = entry.name, status = "ok", message = ""))
        }
    }
    if (json) {
        print(sprint_json(results, false))
        print("\n")
        return ok ? 0 : 1
    }
    for (r in results) {
        if (r.status == "ok") {
            print("  OK: {r.name}\n")
        } elif (r.status == "missing") {
            print("  MISSING: {r.name} ({r.message})\n")
        } else {
            print("  WARNING: {r.name} {r.message}\n")
        }
    }
    if (ok) {
        print("All packages present\n")
    } else {
        print("Some packages are missing — run 'daspkg install' to fix\n")
        return 1
    }
    return 0
}

def private is_safe_pkg_name(name : string) : bool {
    if (empty(name) || name == "." || name == "..") return false
    return !(find(name, "/") >= 0 || find(name, "\\") >= 0)
}

def cmd_cleanup(root : string; force : bool; is_global : bool = false) : int { // nolint:STYLE038 — flat removal checklist (modules/, lock, caches), one arm per artifact
    if (is_global) {
        if (!force) {
            to_log(LOG_ERROR, "Error: --global cleanup requires --force (removes every globally-installed daspkg package)\n")
            return 1
        }
        var glf : LockFile
        let have_lock = read_global_lock_file(glf)
        if (!have_lock) {
            log("No global lock file — nothing to clean\n")
            return 0
        }
        var removed_dirs = 0
        var removed_files = 0
        for (entry in glf.packages) {
            if (!is_safe_pkg_name(entry.name)) {
                to_log(LOG_ERROR, "Error: refusing to clean unsafe package name '{entry.name}' — skipping\n")
                continue
            }
            let pkg_dir = "{get_das_root()}/modules/{entry.name}"
            if (fexist(pkg_dir)) {
                log("Removing {pkg_dir}...\n")
                force_rmdir(pkg_dir)
                removed_dirs ++
            }
        }
        let lock_path = global_lock_file_path()
        if (fexist(lock_path)) {
            remove(lock_path)
            log("Removed {lock_path}\n")
            removed_files ++
        }
        log("Cleaned global: {removed_dirs} directories, {removed_files} files\n")
        return 0
    }
    let pkg_path = "{root}/.das_package"
    let lock_path = lock_file_path(root)
    let modules_dir = "{root}/modules"
    if (!fexist(pkg_path) && !fexist(lock_path)) {
        to_log(LOG_ERROR, "Error: '{root}' has no .das_package or daspkg.lock — refusing to clean arbitrary directory\n")
        return 1
    }
    var lf : LockFile
    let have_lock = read_lock_file(root, lf)
    if (!force) {
        print("Would remove (pass --force to proceed):\n")
        if (fexist(modules_dir)) {
            print("  {modules_dir}/  (recursively)\n")
        }
        if (fexist(lock_path)) {
            print("  {lock_path}\n")
        }
        if (have_lock) {
            for (entry in lf.packages) {
                if (!entry.global) {
                    print("  .gitignore entry: modules/{entry.name}\n")
                }
            }
        }
        return 0
    }
    var removed_dirs = 0
    var removed_files = 0
    if (fexist(modules_dir)) {
        log("Removing {modules_dir}...\n")
        force_rmdir(modules_dir)
        removed_dirs ++
    }
    if (fexist(lock_path)) {
        remove(lock_path)
        log("Removed {lock_path}\n")
        removed_files ++
    }
    if (have_lock) {
        for (entry in lf.packages) {
            if (!entry.global) {
                gitignore_remove(root, "modules/{entry.name}")
            }
        }
    }
    log("Cleaned: {removed_dirs} directories, {removed_files} files\n")
    return 0
}

// ── release ────────────────────────────────────────────────────────────

// JSON shape produced by `daslang -exe --list-shared-modules <path>` — see
// `modules/dasLLVM/daslib/llvm_exe.das write_release_deps()`. Field names +
// types must stay in sync with the writer's structs.

struct private SharedModuleEntry {
    rel : string                    //!< relative bundle path (modules/<X>/<X>.shared_module); "" if path lacks /modules/
    abs : string                    //!< absolute path on the build machine
    das_name : string               //!< daslang-side module name
}

struct private DasModuleEntry {
    name : string                   //!< daslang module name
    file : string                   //!< absolute path to the .das source file
    package_dir : string            //!< absolute path to the enclosing daspkg package
}

struct private TuneScopeDep {
    name : string                   //!< [tune_scope] name
    manifest : string               //!< the app tune sidecar the winners live in
    tuner : string                  //!< tuner harness path ("" = none declared)
    complete : bool                 //!< every [tune] kernel of this scope has a fresh sidecar entry
    fallbacks : string              //!< fallback stamps selected by this same compile/host
}

struct private ReleaseDeps {
    shared_modules : array<SharedModuleEntry>
    das_modules : array<DasModuleEntry>
    tune_scopes : array<TuneScopeDep>
}

def private read_release_deps(path : string; var deps : ReleaseDeps) : bool {
    if (!fexist(path)) return false
    var content : string
    fopen(path, "rb") $(f) {
        if (f != null) {
            content = fread(f)
        }
    }
    var error : string
    var parsed = read_json(content, error)
    if (parsed == null) {
        to_log(LOG_ERROR, "release: cannot parse deps JSON: {error}\n")
        return false
    }
    deps = from_JV(parsed, type<ReleaseDeps>)   // nolint:PERF030 — out-param; every caller hands in a freshly declared ReleaseDeps
    return true
}

// the exact env+cmd shape llvm_tune's policy rail uses for scope tuners (the env prefix
// leads so the line never starts with a quote — cmd.exe strips a LEADING one)
// the one-line provenance of what a scope ships: noise verdict, mode, engine sha, when, box
def private sidecar_provenance_line(manifest : string) : string {
    var err = ""
    var doc = read_json(fread(manifest), err)
    if (doc == null) {
        return "(no readable provenance at {manifest})"
    }
    let line = "noise={doc?["provenance"]?["noise"] ?? "?"} mode={doc?["provenance"]?["mode"] ?? "?"} sha={doc?["provenance"]?["engine_sha"] ?? "?"} written={doc?["provenance"]?["written"] ?? "?"} box={doc?["provenance"]?["box"] ?? "?"}"
    delete_json(doc)
    return line
}

def private release_tuner_cmd(daslang : string; ts : TuneScopeDep; paranoid : bool) : string {
    let win = get_platform_name() == "windows"
    let envs = (win ?
        "set \"DAS_TUNE_MODE=tune\"&& set \"DAS_TUNE_MANIFEST={ts.manifest}\"&& set \"DAS_TUNE_COMPILE_FALLBACKS={ts.fallbacks}\"&& " :
        "DAS_TUNE_MODE=tune DAS_TUNE_MANIFEST=\"{ts.manifest}\" DAS_TUNE_COMPILE_FALLBACKS=\"{ts.fallbacks}\" ")
    let tune_args = paranoid ? " -- --tune-paranoid" : ""
    return "{envs}\"{daslang}\" -jit \"{ts.tuner}\" -dasroot \"{get_das_root()}\"{tune_args}"
}

def private release_elapsed(t0 : int64) : string {
    return fmt(":.1f", float(double(get_time_usec(t0)) / 1000000.0lf))
}

// Bundle path for the standalone exe (daslang -exe appends `.exe` itself).
def private bundle_exe_path(bundle_dir, bundle_name : string) : string {
    return path_join(bundle_dir, bundle_name)
}

// Sanitize one component of a derived CFBundleIdentifier so it satisfies
// Apple's [A-Za-z0-9.-] rule. Lowercases, replaces every other byte with
// `-`, collapses runs of `-`, strips leading/trailing `-`. Returns "x"
// for inputs that yield nothing salvageable so the final id stays valid.
// Only used for the derived `com.<author>.<name>` default — user-supplied
// `release_bundle_id()` passes through untouched.
def private sanitize_bundle_id_part(s : string) : string {
    let lower = to_lower(s)
    let raw = build_string() $(var writer) {
        var prev_dash = false
        peek_data(lower) $(arr) {
            for (i in range(length(arr))) {
                let b = int(arr[i])
                // `lower` was lowercased earlier — is_alpha matches both cases,
                // but no uppercase can reach this branch, so it's equivalent.
                let is_alnum = is_number(b) || is_alpha(b)
                if (is_alnum) {
                    write_char(writer, b)
                    prev_dash = false
                } elif (!prev_dash) {
                    write_char(writer, '-')
                    prev_dash = true
                }
            }
        }
    }
    var result = raw
    if (!empty(result) && result |> ends_with("-")) {
        result = slice(result, 0, length(result) - 1)
    }
    if (!empty(result) && result |> starts_with("-")) {
        result = slice(result, 1)
    }
    return empty(result) ? "x" : result
}

// Minimal macOS .app Info.plist. Built with dasPUGIXML so values are
// XML-escaped automatically — inputs (bundle_name, exe, bundle_id) are
// otherwise unrestricted on the XML side. CFPropertyList accepts plist
// without a DOCTYPE, so we omit it.
def private build_info_plist(name, exe, bundle_id : string) : string {
    var out = ""
    with_doc() $(doc) {
        doc |> tag("plist") $(var pl) {
            pl |> attr("version", "1.0")
            pl |> tag("dict") $(var d) {
                d |> tag("key", "CFBundleName")
                d |> tag("string", name)
                d |> tag("key", "CFBundleExecutable")
                d |> tag("string", exe)
                d |> tag("key", "CFBundleIdentifier")
                d |> tag("string", bundle_id)
                d |> tag("key", "CFBundlePackageType")
                d |> tag("string", "APPL")
                d |> tag("key", "CFBundleVersion")
                d |> tag("string", "1.0")
                d |> tag("key", "CFBundleShortVersionString")
                d |> tag("string", "1.0")
                d |> tag("key", "NSPrincipalClass")
                d |> tag("string", "NSApplication")
                d |> tag("key", "NSHighResolutionCapable")
                d |> tag("true")
                d |> tag("key", "NSSupportsAutomaticGraphicsSwitching")
                d |> tag("true")
            }
        }
        out = to_string(doc)
    }
    return out
}

// Run release() on a dep package and copy its release_include files into the
// bundle at `<bundle>/modules/<DepName>/<rel>`. Returns 0 on success, non-zero
// on copy failure (missing release() is NOT an error — dep just contributes nothing).
def private ship_dep_assets(pkg_dir, bundle_modules_dir : string; var shipped : array<string>) : int {
    let dep_pkg_file = "{pkg_dir}/.das_package"
    if (!fexist(dep_pkg_file)) return 0
    var dep_spec : PackageReleaseInfo
    if (!run_das_package_release(dep_pkg_file, dep_spec) ||
        (empty(dep_spec.include_globs) && empty(dep_spec.include_if_missing_globs) &&
            empty(dep_spec.native_dlls))) return 0
    let dep_name = base_name(pkg_dir)
    let dep_dst_dir = "{bundle_modules_dir}/{dep_name}"
    var dep_files : array<string>
    glob_filtered(pkg_dir, dep_spec.include_globs, dep_spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            dep_files |> push(rel_path)
        }
    }
    shipped |> reserve(length(shipped) + length(dep_files))
    for (rel in dep_files) {
        let src = path_join(pkg_dir, rel)
        let dst = path_join(dep_dst_dir, rel)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
        shipped |> push("modules/{dep_name}/{rel}")
    }
    if (!empty(dep_files)) {
        log("  ship dep `{dep_name}`: {length(dep_files)} file(s)\n")
    }
    var dep_starter_files : array<string>
    glob_filtered(pkg_dir, dep_spec.include_if_missing_globs, dep_spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            dep_starter_files |> push(rel_path)
        }
    }
    var dep_starter_copies = 0
    for (rel in dep_starter_files) {
        let src = path_join(pkg_dir, rel)
        let dst = path_join(dep_dst_dir, rel)
        if (fexist(dst)) continue
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, false, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
        dep_starter_copies++
    }
    if (dep_starter_copies > 0) {
        log("  initialize dep `{dep_name}`: {dep_starter_copies} user-owned file(s)\n")
    }
    if (!empty(dep_spec.native_dlls)) {
        let rc = ship_native_dlls(dep_name, dep_dst_dir, dep_spec.native_dlls, shipped, "modules/{dep_name}/")
        if (rc != 0) return rc
    }
    return 0
}

// Cached `patchelf --version` result. -1 = unknown, 0 = missing (warned), 1 = present.
// Module-global mutable state: reset at the top of cmd_release (the sole
// entry point that wants a fresh probe per release) so a long-running daspkg
// process picks up a patchelf install between releases. Adding another caller
// for `have_patchelf` outside cmd_release? Either reset there too, or fold
// the reset into `have_patchelf` itself behind a one-shot flag — otherwise
// the new call site will see whatever value the last cmd_release left.
// Linux-only; macOS uses install_name_tool (ships with the OS) so no
// equivalent gate is needed.
var private patchelf_state : int = -1

def private have_patchelf() : bool {
    if (patchelf_state >= 0) return patchelf_state == 1
    // Probe by invoking the binary itself (`--version`) rather than the
    // `command -v` shell builtin — survives any future `run_cmd` refactor
    // that bypasses /bin/sh, and tests the same PATH resolution every
    // subsequent `patchelf` call will use.
    var out : string
    if (run_cmd("patchelf --version", out) == 0) {
        patchelf_state = 1
        return true
    }
    patchelf_state = 0
    to_log(LOG_WARNING, "release: patchelf not found on PATH; absolute RPATH/RUNPATH entries will be left in shipped binaries (bundle still works if dyld/ld.so resolves dylibs via @loader_path/$ORIGIN, but the build-machine paths are visible in `readelf -d`)\n")
    return false
}

// Recreate sibling symlinks at the destination so versioned dylib chains
// (`libglfw.dylib -> libglfw.3.dylib -> libglfw.3.4.dylib`) survive the bundle
// copy. dyld/ld.so look up the install_name verbatim, so if the loader asks
// for `@rpath/libglfw.3.dylib` but the bundle only contains the file named
// `libglfw.3.4.dylib`, lookup fails. The build dir already has the right
// symlink chain — daspkg just isn't preserving it.
//
// POSIX-only. Windows DLLs don't use this naming scheme.
//
// `equivalent()` follows symlinks transitively, so a sibling that links to
// the leaf file through any number of hops will compare equal to it.
// bundle-relative sibling path of `filename` next to the shipped file at `rel_dst`
def private sibling_rel(rel_dst, filename : string) : string {
    let rd = dir_name(rel_dst)
    return empty(rd) || rd == "." ? filename : "{rd}/{filename}"
}

def private preserve_dylib_symlinks(src_dir, dst_dir, src_name : string;
                                    var shipped : array<string>; rel_dst : string) {
    if (get_platform_name() == "windows") return
    let src_abs = path_join(src_dir, src_name)
    dir(src_dir) $(filename) {
        if (filename == src_name || filename == "." || filename == "..") return
        let cand = path_join(src_dir, filename)
        var err : string
        if (!is_symlink(cand, err)) return
        var eq_err : string
        if (!equivalent(cand, src_abs, eq_err)) return
        let link_path = path_join(dst_dir, filename)
        // Already shipped (e.g. dual call sites for the same dep) — leave alone.
        if (fexist(link_path)) return
        var ln_out : string
        let ln_rc = run_cmd("ln -s \"{src_name}\" \"{link_path}\"", ln_out)
        if (ln_rc == 0) {
            log("  ship symlink: {filename} -> {src_name}\n")
            shipped |> push(sibling_rel(rel_dst, filename))
        } else {
            to_log(LOG_WARNING, "release: ln -s {src_name} -> {link_path} failed (rc={ln_rc}): {ln_out}\n")
        }
    }
}

//! Parse `otool -l <path>` and return every LC_RPATH entry, in order.
//! macOS-only. Returns an empty array if otool fails or the binary has no
//! LC_RPATH blocks. Exported so the audit test in test_daspkg.das can reuse
//! the same parser the production scrubber uses.
def list_lc_rpaths(path : string) : array<string> {
    var rpaths : array<string>
    if (get_platform_name() != "darwin") return <- rpaths
    var lst_out : string
    if (run_cmd("otool -l \"{path}\"", lst_out) != 0) return <- rpaths
    var in_rpath = false
    for (line in split(lst_out, "\n")) {
        let s = strip(line)
        if (s == "cmd LC_RPATH") {
            in_rpath = true
        } elif (in_rpath && starts_with(s, "path ")) {
            let after = slice(s, 5)
            let off = find(after, " (offset")
            let p = (off >= 0 ? slice(after, 0, off) : after)
            rpaths |> push(p)
            in_rpath = false
        } elif (starts_with(s, "Load command ")) {
            in_rpath = false
        }
    }
    return <- rpaths
}

//! True when `rp` is NOT a loader-relative path (`@executable_path`,
//! `@loader_path`, or `@rpath`). Loader-relative entries are bundle-portable;
//! everything else is a build-tree leak and must be scrubbed before ship.
def is_external_rpath(rp : string) : bool {
    return !(starts_with(rp, "@executable_path") ||
             starts_with(rp, "@loader_path") ||
             starts_with(rp, "@rpath"))
}

struct private LinuxRpathTags {
    has_rpath : bool
    has_runpath : bool
    value : string
}

// Read RPATH / RUNPATH from `readelf -d <path>`. Lines look like:
//   0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN/lib:/abs]
//   0x000000000000000f (RPATH)              Library rpath: [/abs/path]
// In practice only one of the two appears, and modern toolchains prefer
// RUNPATH. When both are present (rare; older toolchains), `value`
// concatenates their bracketed contents with `:` so the downstream
// filter sees every entry from both tags. When neither is present, all
// fields are zero/empty.
def private read_linux_rpath_tags(path : string) : LinuxRpathTags {
    var tags : LinuxRpathTags
    var re_out : string
    if (run_cmd("readelf -d \"{path}\"", re_out) != 0) return tags
    for (line in split(re_out, "\n")) {
        let saw_rpath = find(line, "(RPATH)") >= 0
        let saw_runpath = find(line, "(RUNPATH)") >= 0
        if (!saw_rpath && !saw_runpath) continue
        let lb = find(line, "[")
        let rb = find(line, "]")
        if (lb < 0 || rb <= lb) continue
        if (saw_rpath) {
            tags.has_rpath = true
        }
        if (saw_runpath) {
            tags.has_runpath = true
        }
        let val = slice(line, lb + 1, rb)
        if (empty(tags.value)) {
            tags.value = val
        } else {
            tags.value = "{tags.value}:{val}"
        }
    }
    return tags
}

// Strip absolute RPATH/LC_RPATH entries from a shipped binary so the bundle
// can't dyld-fallback through build-machine paths on the recipient.
//
//   macOS: list LC_RPATH via `list_lc_rpaths`, drop every entry where
//          `is_external_rpath` is true via `install_name_tool -delete_rpath`.
//   Linux: read RPATH AND RUNPATH separately via `readelf -d`, keep only
//          `$ORIGIN`-relative entries, write back preserving the original
//          tag (DT_RPATH vs DT_RUNPATH have different lookup semantics —
//          RPATH is checked before LD_LIBRARY_PATH, RUNPATH after). Skipped
//          when patchelf isn't on PATH (warned once via `have_patchelf`).
//   Windows: no-op (PE has no rpath equivalent).
def private scrub_external_rpaths(path : string) {
    let platform = get_platform_name()
    if (platform == "windows") return
    if (platform == "darwin") {
        for (rp in list_lc_rpaths(path)) {
            if (!is_external_rpath(rp)) continue
            var del_out : string
            let drc = run_cmd("install_name_tool -delete_rpath \"{rp}\" \"{path}\"", del_out)
            if (drc != 0) {
                to_log(LOG_WARNING, "release: install_name_tool -delete_rpath \"{rp}\" \"{path}\" failed (rc={drc}): {del_out}\n")
            }
        }
        return
    }
    // Linux
    if (!have_patchelf()) return
    let tags = read_linux_rpath_tags(path)
    if (!tags.has_rpath && !tags.has_runpath) return
    var kept : array<string>
    for (entry in split(tags.value, ":")) {
        let e = strip(entry)
        if (empty(e)) continue
        if (starts_with(e, "$ORIGIN")) {
            kept |> push(e)
        }
    }
    var op_out : string
    if (empty(kept)) {
        if (run_cmd("patchelf --remove-rpath \"{path}\"", op_out) != 0) {
            to_log(LOG_WARNING, "release: patchelf --remove-rpath {path} failed: {op_out}\n")
        }
        return
    }
    let joined = join(kept, ":")
    // Escape `$` to `\$` so bash inside the double-quoted `--set-rpath "..."`
    // arg treats $ORIGIN / $LIB / $PLATFORM as literal dollar-prefixed tokens
    // instead of shell variable references (which would expand to empty —
    // wiping the rpath, exactly what made the Linux smoke fail on CI before
    // this fix).
    let joined_shell = replace(joined, "$", "\\$")
    // Preserve which tag was originally set. If the binary had DT_RPATH but
    // no DT_RUNPATH, patchelf would otherwise write DT_RUNPATH and change
    // loader semantics (RUNPATH defers to LD_LIBRARY_PATH first). When both
    // tags are present (rare; modern binutils rejects this since 2.36), or
    // only RUNPATH, the default `--set-rpath` writes DT_RUNPATH which
    // matches the original.
    let force_flag = (tags.has_rpath && !tags.has_runpath) ? "--force-rpath " : ""
    if (run_cmd("patchelf {force_flag}--set-rpath \"{joined_shell}\" \"{path}\"", op_out) != 0) {
        to_log(LOG_WARNING, "release: patchelf {force_flag}--set-rpath \"{joined}\" {path} failed: {op_out}\n")
    }
}

// Read the dylib's "self-reference name" — install_name on macOS (LC_ID_DYLIB
// via `otool -D`), SONAME on Linux (DT_SONAME via `patchelf --print-soname`
// with `readelf -d` as the more-universal fallback). Empty string means
// unreadable or none present.
def private read_dylib_self_name(dst : string) : string {
    let platform = get_platform_name()
    if (platform == "darwin") {
        var id_out : string
        if (run_cmd("otool -D \"{dst}\"", id_out) != 0) return ""
        // `otool -D` prints:  <filename>:\n<install_name>\n   (two lines)
        var name = ""
        for (line in split(id_out, "\n")) {
            let s = strip(line)
            if (empty(s) || ends_with(s, ":")) continue
            name = s
        }
        return name
    }
    if (platform != "linux") return ""
    // Try patchelf first (already required for the Linux scrub path).
    var pe_out : string
    if (run_cmd("patchelf --print-soname \"{dst}\"", pe_out) == 0) {
        let soname = strip(pe_out)
        if (!empty(soname)) return soname
    }
    // readelf fallback — universally available via binutils.
    //   0x000000000000000e (SONAME)             Library soname: [libglfw.so.3]
    var re_out : string
    if (run_cmd("readelf -d \"{dst}\"", re_out) != 0) return ""
    for (line in split(re_out, "\n")) {
        if (find(line, "SONAME") < 0) continue
        let lb = find(line, "[")
        let rb = find(line, "]")
        if (lb >= 0 && rb > lb) return slice(line, lb + 1, rb)
    }
    return ""
}

// dyld/ld.so looks up dylibs by the self-reference name baked into the
// loading binary, not by filename — so if libglfw.3.4.dylib's install_name
// is `@rpath/libglfw.3.dylib`, the bundle must contain a file (or symlink)
// literally named `libglfw.3.dylib`, no matter what the source file was
// named. CMake's install step into `<das_root>/lib` doesn't preserve the
// build dir's versioned-alias chain — so `preserve_dylib_symlinks` finds
// nothing to copy, and we have to reconstruct the alias from the dylib's
// own metadata.
//
// No-op when basename(self-name) == basename(dst), the alias already exists,
// or (macOS) the install_name isn't @-relative.
def private ensure_dylib_alias(dst : string; var shipped : array<string>; rel_dst : string) {
    let platform = get_platform_name()
    if (platform != "darwin" && platform != "linux") return
    let self_name = read_dylib_self_name(dst)
    // macOS install_names must be @-relative to be portable; absolute paths
    // shouldn't appear in shipped dylibs and we shouldn't create an alias
    // for them. Linux SONAMEs are bare basenames — no prefix gate needed.
    if (empty(self_name) ||
        (platform == "darwin" &&
         !(starts_with(self_name, "@rpath/") ||
           starts_with(self_name, "@executable_path/") ||
           starts_with(self_name, "@loader_path/")))) return
    let alias = base_name(self_name)
    if (alias == base_name(dst)) return
    let link_path = path_join(dir_name(dst), alias)
    if (fexist(link_path)) return
    var ln_out : string
    let ln_rc = run_cmd("ln -s \"{base_name(dst)}\" \"{link_path}\"", ln_out)
    if (ln_rc == 0) {
        log("  ship alias: {alias} -> {base_name(dst)}\n")
        shipped |> push(sibling_rel(rel_dst, alias))
    } else {
        to_log(LOG_WARNING, "release: ln -s {base_name(dst)} -> {link_path} failed (rc={ln_rc}): {ln_out}\n")
    }
}

// One-liner used at every ship site: preserve any sibling symlink chain from
// the source, materialize the install_name alias if it's still missing
// (macOS), then strip absolute rpath entries from the copy. Symlinks/aliases
// it creates are recorded in `shipped` (as siblings of `rel_dst`) so the
// next release's manifest cleanup removes them too.
// Destination for `release_include_symbols()`, set once per release; empty disables collection.
// Held as state because finalize_shipped_binary is reached from three call sites that do not
// otherwise carry the bundle root or the release spec.
var private _symbols_dir = ""

// Ship the debug symbols belonging to one binary. The PDB is looked up beside the SOURCE, not the
// destination: MSVC writes `<stem>.pdb` next to the linker output, and every artifact a release
// ships (the exe, libDaScriptDyn*.dll, each .shared_module) has one there. Looking beside the
// *deployed* binary is what fails — a released tree has no PDBs, which is why crash bundles that
// probe `<program>.pdb` come back empty.
def private ship_symbols_for(src : string; var shipped : array<string>) {
    if (empty(_symbols_dir) || get_platform_name() != "windows") {
        return
    }
    let pdb = path_join(dir_name(src), "{stem(base_name(src))}.pdb")
    if (!fexist(pdb)) {
        // Not fatal: a module built without /DEBUG simply has no symbols, and a release should
        // still complete. The log names the binary so a missing PDB is visible, not silent.
        to_log(LOG_WARNING, "release: no symbols for {base_name(src)} (expected {base_name(pdb)})\n")
        return
    }
    let rel_dst = "symbols/{base_name(pdb)}"
    let dst = path_join(_symbols_dir, base_name(pdb))
    var err : string
    if (!copy_file(pdb, dst, true, err)) {
        to_log(LOG_WARNING, "release: copy_file({pdb} -> {dst}) failed: {err}\n")
        return
    }
    log("  ship symbols: {base_name(pdb)}\n")
    shipped |> push(rel_dst)
}

def private finalize_shipped_binary(src, dst : string; var shipped : array<string>; rel_dst : string) {
    preserve_dylib_symlinks(dir_name(src), dir_name(dst), base_name(src), shipped, rel_dst)
    ensure_dylib_alias(dst, shipped, rel_dst)
    scrub_external_rpaths(dst)
    ship_symbols_for(src, shipped)
}

// Copy each declared native shared library from the daslang install tree
// (`<das_root>/bin/` on Windows, `<das_root>/lib/` on POSIX) into `dst_dir`.
// Authors declared these via `release_include_dll(...)`; missing files are an
// error — usually a typo or an out-of-sync install.
//
// Resolution order (deterministic so multi-config build trees can't ship the
// wrong binary nondeterministically):
//   1. Exact match at `<search_root>/<name>` (an installed layout has this).
//   2. Build-tree config dirs in fixed preference: Release > RelWithDebInfo >
//      MinSizeRel > Debug, e.g. `<search_root>/Release/<name>`.
//   3. Fall through to `**/<name>` and pick the first match.
// `release_include_from` entries, as "source@dest" (dest empty = source's file name). Source is
// relative to <das_root>; dest to the bundle root. A missing source fails the release rather than
// shipping a bundle that is quietly short a file.
def private ship_external_files(bundle_name, dst_dir : string; external_files : array<string>;
                                var shipped : array<string>) : int {
    let das_root = get_das_root()
    shipped |> reserve(length(shipped) + length(external_files))
    for (entry in external_files) {
        let at = find(entry, "@")
        if (at < 0) {
            to_log(LOG_ERROR, "release: `{bundle_name}` has a malformed release_include_from entry `{entry}` (expected \"source@dest\")\n")
            return 1
        }
        let source = slice(entry, 0, at)
        let requested = slice(entry, at + 1)
        let dest = empty(requested) ? base_name(source) : requested
        let src = path_join(das_root, source)
        if (!fexist(src)) {
            to_log(LOG_ERROR, "release: `{bundle_name}` declared release_include_from(\"{source}\") but `{src}` does not exist\n")
            return 1
        }
        let dst = path_join(dst_dir, dest)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
        shipped |> push(dest)
    }
    log("  ship external: {length(external_files)} file(s)\n")
    return 0
}


def private ship_native_dlls(dep_name, dst_dir : string; native_dlls : array<string>;
                             var shipped : array<string>; rel_prefix : string) : int {
    let platform = get_platform_name()
    let search_root = (platform == "windows"
        ? "{get_das_root()}/bin"
        : "{get_das_root()}/lib")
    mkdir_rec(dst_dir)
    shipped |> reserve(length(shipped) + length(native_dlls))
    for (name in native_dlls) {
        var found_src = ""
        // 1. exact path under search_root
        let exact = path_join(search_root, name)
        if (fexist(exact)) {
            found_src = exact
        }
        // 2. preferred config dirs
        if (empty(found_src)) {
            for (cfg in ["Release", "RelWithDebInfo", "MinSizeRel", "Debug"]) {
                let candidate = "{search_root}/{cfg}/{name}"
                if (fexist(candidate)) {
                    found_src = candidate
                    break
                }
            }
        }
        // 3. fall through to a recursive glob (handles nested layouts)
        if (empty(found_src)) {
            glob(search_root, "**/{name}") $(rel_path, is_dir) {
                if (!is_dir && empty(found_src)) {
                    found_src = path_join(search_root, rel_path)
                }
            }
        }
        if (empty(found_src)) {
            to_log(LOG_ERROR, "release: dep `{dep_name}` declared release_include_dll(\"{name}\") but no match under `{search_root}` (tried {search_root}/{name}, {search_root}/<config>/{name}, and **/{name})\n")
            return 1
        }
        // dir_name(dst) so a `name` like `subdir/lib.so` still works -- defensive,
        // not endorsed by the API docs which imply a leaf filename.
        let dst = path_join(dst_dir, name)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(found_src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({found_src} -> {dst}) failed: {err}\n")
            return 1
        }
        log("  ship dep `{dep_name}` native: {name}\n")
        finalize_shipped_binary(found_src, dst, shipped, "{rel_prefix}{name}")
        shipped |> push("{rel_prefix}{name}")
    }
    return 0
}

// Every release writes this manifest (each shipped file, bundle-relative) so the NEXT release
// deletes exactly what it produced. The bundle dir also holds USER files — the auto-loaded
// <name>.toml config, logs — that the old whole-dir wipe destroyed on every re-release.
let RELEASE_MANIFEST = ".daspkg_release.manifest"

def private release_path_is_preserved(rel : string; preserve_paths : array<string>) : bool {
    let generic_rel = to_generic_path(rel)
    for (p in preserve_paths) {
        if (to_generic_path(p) == generic_rel) return true
    }
    return false
}

def private clean_previous_release(bundle_dir : string; preserve_paths : array<string>) {
    let mpath = path_join(bundle_dir, RELEASE_MANIFEST)
    if (!fexist(mpath)) {
        return   // first release into this dir (or a pre-manifest bundle): nothing to clean
    }
    let body = fread(mpath)
    if (empty(body)) {
        to_log(LOG_WARNING, "release: {RELEASE_MANIFEST} exists but reads empty — skipping cleanup; stale files from a previous release may remain\n")
        return
    }
    for (line in split(body, "\n")) {
        let rel = strip(line)
        // A file may have been release-owned by an older package manifest and become
        // release_include_if_missing now. Preserve that deployed copy during migration.
        // skip blanks; stay inside the bundle even on a corrupt or hostile manifest —
        // reject `..`, drive colons, and both absolute spellings (`/x`, `\x` = drive root)
        if (empty(rel) || find(rel, "..") >= 0 || starts_with(rel, "/") || starts_with(rel, "\\")
                || find(rel, ":") >= 0 || release_path_is_preserved(rel, preserve_paths)) {
            continue
        }
        let p = path_join(bundle_dir, rel)
        if (fexist(p)) {
            remove(p)
        }
    }
    remove(mpath)
}

// Copy of `daslib/command_line.get_das_exe()`. We can't require command_line
// here because `commands` is a shared module and command_line is not.
// Returns the absolute path to the daslang interpreter, derived from argv[0]
// (works whether daspkg runs interpreted or as a future standalone exe).
def private get_daslang_path() : string {
    let args <- get_command_line_arguments()
    if (empty(args)) return ""
    var exe = replace(args[0], "\\", "/")
    if (find(exe, ":") < 0 && first_character(exe) != '/') {
        exe = "{get_das_root()}/{exe}"
    }
    let last_slash = rfind(exe, "/")
    let dir = last_slash >= 0 ? slice(exe, 0, last_slash + 1) : ""
    let interpreter = get_platform_name() == "windows" ? "daslang.exe" : "daslang"
    let joined = "{dir}{interpreter}"
    return get_platform_name() == "windows" ? replace(joined, "/", "\\") : joined
}

// remember the resolved scope manifest for the ship step; warn once on conflicting scopes
def private release_note_ship_manifest(manifest : string; var ship_manifest : string&) {
    if (empty(manifest) || !fexist(manifest)) {
        return
    }
    if (empty(ship_manifest)) {
        ship_manifest = manifest
    } elif (ship_manifest != manifest) {
        to_log(LOG_WARNING, "release: multiple tune-scope manifests ({ship_manifest} vs {manifest}) — shipping the first\n")
    }
}

def cmd_release(root : string; out_dir : string; paranoid = false; quick = false) : int { // nolint:STYLE037,STYLE038 — phased orchestrator sharing bundle state end to end; the real split belongs to the daspkg cleanup pass
    let release_t0 = ref_time_ticks()
    if (paranoid && quick) {
        to_log(LOG_ERROR, "release: --paranoid and --quick contradict — pick one\n")
        return 1
    }
    // Reset the patchelf availability cache so a daspkg process that releases
    // multiple bundles can pick up a patchelf install between calls.
    patchelf_state = -1
    let das_package = "{root}/.das_package"
    if (!fexist(das_package)) {
        to_log(LOG_ERROR, "release: no .das_package in {root}\n")
        return 1
    }
    var spec : PackageReleaseInfo
    if (!run_das_package_release(das_package, spec)) {
        to_log(LOG_ERROR, "release: cannot run release() in {das_package}\n")
        return 1
    }
    if (empty(spec.main_script)) {
        // run_das_package_release returns true with an empty spec both when
        // release() is missing entirely and when it's present but never calls
        // release_main(). Phrase the error to cover both.
        to_log(LOG_ERROR, "release: {das_package} has no release() hook, or release() didn't call release_main(<script.das>)\n")
        return 1
    }
    if (spec.requires_jit) {
        // release_requires_jit(): a baked -exe drops the per-box JIT kernels/tune (and can
        // mis-wire shared-module [init] globals), so it would ship a broken binary. Deploy the
        // app as `daslang -jit {spec.main_script}` instead.
        to_log(LOG_ERROR, "release: {das_package} declares release_requires_jit() — this app is JIT-only and cannot be baked into a standalone -exe. Deploy it as: daslang -jit {spec.main_script}\n")
        return 1
    }

    // Pick bundle name: explicit release_name() > package_name() > root dir name.
    var pkg : PackageMeta
    let have_pkg = run_das_package_meta(das_package, pkg)
    var bundle_name = spec.name
    if (empty(bundle_name)) {
        if (have_pkg && !empty(pkg.pkg_name)) {
            bundle_name = pkg.pkg_name
        } else {
            bundle_name = base_name(get_full_file_name(root))
        }
    }
    // bundle_name flows from .das_package metadata (release_name() /
    // package_name()) — possibly authored by a downloaded dep. Reject path
    // separators / `..` so a hostile or typo'd name can't escape out_dir
    // when path_join + force_rmdir run below.
    if (!is_safe_pkg_name(bundle_name)) {
        to_log(LOG_ERROR, "Error: unsafe bundle name `{bundle_name}` (must not contain path separators or be `.` / `..`)\n")
        return 1
    }
    // macOS: emit a clickable .app bundle. The exe + dylibs + modules go into
    // Contents/MacOS/, so all subsequent ship logic uses bundle_dir unchanged
    // and the rpath / module resolver still see the same <exe_dir> layout.
    let is_mac_app = get_platform_name() == "darwin"
    let app_root = is_mac_app ? path_join(out_dir, "{bundle_name}.app") : ""
    let bundle_dir = is_mac_app ? path_join(app_root, "Contents/MacOS") : path_join(out_dir, bundle_name)
    var preserve_project_files : array<string>
    glob_filtered(root, spec.include_if_missing_globs, spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            preserve_project_files |> push(rel_path)
        }
    }
    // The macOS .app is wholly daspkg-owned: full wipe. A flat bundle dir is NOT — it
    // accumulates user files next to daspkg's output — so it cleans via the previous
    // release's manifest instead (empty dirs from removed deps may linger; harmless).
    // User-owned starter files (release_include_if_missing — deployed configs the user may
    // have edited) survive the wipe: stash before, restore after, and the if-missing copy
    // below then sees them existing and leaves them alone.
    var preserved_snapshots : table<string; string>
    if (is_mac_app) {
        if (fexist(app_root)) {
            for (rel in preserve_project_files) {
                let p = path_join(bundle_dir, rel)
                if (fexist(p)) {
                    preserved_snapshots[rel] = fread(p)
                }
            }
            force_rmdir(app_root)
        }
    } elif (fexist(bundle_dir)) {
        clean_previous_release(bundle_dir, preserve_project_files)
    }
    mkdir_rec(bundle_dir)
    for (rel, content in keys(preserved_snapshots), values(preserved_snapshots)) {
        let p = path_join(bundle_dir, rel)
        mkdir_rec(dir_name(p))
        fwrite(p, content)
    }
    if (!empty(preserved_snapshots)) {
        log("  preserved {length(preserved_snapshots)} user-owned starter file(s) through the .app wipe\n")
    }
    // Arm symbol collection for every finalize_shipped_binary below. Windows-only for now: ELF
    // carries DWARF in the binary and macOS needs .dSYM handling, so neither has a .pdb to ship.
    _symbols_dir = ""
    if (spec.include_symbols) {
        if (get_platform_name() == "windows") {
            _symbols_dir = path_join(bundle_dir, "symbols")
            mkdir_rec(_symbols_dir)
        } else {
            log("  symbols: skipped ({get_platform_name()} has no .pdb; DWARF/dSYM not handled yet)\n")
        }
    }
    var shipped : array<string>   // bundle-relative paths -> RELEASE_MANIFEST at the end

    // 1. Build the standalone exe; `--list-shared-modules` writes the deps JSON
    //    (auto-detected dylibs + transitive .das_package list) as a side effect.
    //    `--force-shared-module <name>` passes each release_shared_module(name)
    //    to daslang; lookup happens there so the full registry is available
    //    (when daspkg itself is compiled standalone, its registry would only
    //    contain dylibs daspkg's own code references — too narrow).
    let exe_path = bundle_exe_path(bundle_dir, bundle_name)
    run_cmd_counter ++
    let deps_file = "{get_das_root()}/_daspkg_release_deps_{run_cmd_counter}.json"
    let main_path = path_join(root, spec.main_script)
    var output : string
    let daslang = get_daslang_path()
    let force_args = build_string() $(var writer) {
        for (name in spec.forced_modules) {
            writer |> write(" --force-shared-module \"")
            writer |> write(name)
            writer |> write("\"")
        }
    }
    let cmd = "\"{daslang}\" -exe -output \"{exe_path}\" --list-shared-modules \"{deps_file}\"{force_args} \"{main_path}\""
    let build_t0 = ref_time_ticks()
    let rc = run_cmd(cmd, output)
    if (rc != 0) {
        to_log(LOG_ERROR, "release: daslang -exe failed (rc={rc}):\n{output}\n")
        if (fexist(deps_file)) {
            remove(deps_file)
        }
        return 1
    }
    log("  build: standalone exe in {release_elapsed(build_t0)}s\n")
    // 1.5 Build-time tuning: release ALWAYS mints — a mint is minutes, and an inherited
    //     sidecar of unknown provenance is how a noisy race poisons every measurement that
    //     follows. `--quick` is the ONLY mode that accepts an existing sidecar, and only a
    //     COMPLETE one (an incomplete/stale scope mints even under --quick: the frozen exe
    //     itself never tunes, so shipping it unmeasured is not an option).
    var ship_manifest = ""   // the resolved scope manifest step 2.6 ships (see there)
    {
        var deps0 : ReleaseDeps
        if (read_release_deps(deps_file, deps0)) {
            var tuned_any = false
            for (ts in deps0.tune_scopes) {
                if (empty(ts.tuner)) {
                    if (!ts.complete) {
                        to_log(LOG_WARNING, "release: tune scope `{ts.name}` is untuned and declares no tuner — shipping fallback stamps\n")
                    }
                    continue
                }
                if (quick && ts.complete) {
                    log("  tune: scope `{ts.name}` inherited (--quick): {sidecar_provenance_line(ts.manifest)}\n")
                    release_note_ship_manifest(ts.manifest, ship_manifest)
                    continue
                }
                if (quick && !ts.complete) {
                    log("  tune: scope `{ts.name}` is incomplete — minting despite --quick\n")
                } elif (ts.complete) {
                    log("  tune: scope `{ts.name}` re-mints (release always mints; --quick inherits)\n")
                }
                log("  tune: scope `{ts.name}` -> {ts.manifest}\n")
                let tune_t0 = ref_time_ticks()
                var tout : string
                let trc = run_cmd(release_tuner_cmd(daslang, ts, paranoid), tout)
                let tune_seconds = release_elapsed(tune_t0)
                if (trc != 0) {
                    // a nonzero tuner is a REFUSAL (noise gate, validation) or a failure —
                    // either way the winners were not measured, and unmeasured winners do not ship
                    to_log(LOG_ERROR, "release: tuner for scope `{ts.name}` refused/failed after {tune_seconds}s (rc={trc}) — not shipping unmeasured winners; quiet the box and re-run, --quick to inherit a complete sidecar, or DAS_TUNE_NOISE_OVERRIDE=1 to mint anyway (stamped in provenance)\n{tout}\n")
                    if (fexist(deps_file)) {
                        remove(deps_file)
                    }
                    return 1
                }
                log("  tune: scope `{ts.name}` finished in {tune_seconds}s — ships {sidecar_provenance_line(ts.manifest)}\n")
                release_note_ship_manifest(ts.manifest, ship_manifest)
                tuned_any = true
            }
            if (tuned_any) {
                log("  rebuild: baking the fresh tune winners into the exe\n")
                let rebuild_t0 = ref_time_ticks()
                var out2 : string
                let rc2 = run_cmd(cmd, out2)
                if (rc2 != 0) {
                    to_log(LOG_ERROR, "release: daslang -exe rebuild failed (rc={rc2}):\n{out2}\n")
                    if (fexist(deps_file)) {
                        remove(deps_file)
                    }
                    return 1
                }
                log("  rebuild: standalone exe in {release_elapsed(rebuild_t0)}s\n")
            }
        }
    }
    // daslang -exe writes <exe_path>.exe (the runnable binary) and <exe_path>.o
    // (a build-time object file). Drop the .o — it's not needed at runtime.
    let obj_path = "{exe_path}.o"
    if (fexist(obj_path)) {
        remove(obj_path)
    }
    shipped |> push(is_mac_app ? bundle_name : "{bundle_name}.exe")
    if (fexist("{exe_path}.lib")) {   // MSVC linker side artifact lands next to the exe
        shipped |> push("{bundle_name}.lib")
    }
    // macOS .app: drop the .exe extension (Mac convention). Info.plist's
    // CFBundleExecutable matches the renamed binary.
    if (is_mac_app) {
        let actual_exe = "{exe_path}.exe"
        var rerr : string
        if (!rename(actual_exe, exe_path, rerr)) {
            to_log(LOG_ERROR, "release: cannot rename {actual_exe} -> {exe_path}: {rerr}\n")
            return 1
        }
    }
    // Scrub absolute LC_RPATH / RUNPATH entries the daslang build leaks into
    // the exe (build-tree `<das_root>/lib` etc.). The `@executable_path`/`$ORIGIN`
    // entries that PR #2588 added are kept — those are what makes the bundle
    // portable in the first place. Target the runnable binary: `<exe_path>.exe`
    // everywhere except a mac .app, where the rename above already dropped it.
    scrub_external_rpaths(is_mac_app ? exe_path : "{exe_path}.exe")

    // 2. Parse deps JSON, ship dylibs at the relative path the exe expects.
    //    The writer has already merged force-included modules into shared_modules,
    //    so we just iterate.
    var deps : ReleaseDeps
    if (!read_release_deps(deps_file, deps)) {
        to_log(LOG_ERROR, "release: cannot read deps file at {deps_file}\n")
        remove(deps_file)
        return 1
    }
    remove(deps_file)
    for (e in deps.shared_modules) {
        if (empty(e.rel)) {
            to_log(LOG_WARNING, "release: skipping module `{e.das_name}` (no /modules/ in `{e.abs}`)\n")
            continue
        }
        let dst = path_join(bundle_dir, e.rel)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(e.abs, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({e.abs} -> {dst}) failed: {err}\n")
            return 1
        }
        log("  ship: {e.rel}\n")
        finalize_shipped_binary(e.abs, dst, shipped, e.rel)
        shipped |> push(e.rel)
    }

    // 2.5 Ship daslang runtime dylibs next to the exe. Without these the bundle
    //     dyld-loads via the build-machine fallback rpath and crashes on relocation.
    //     The exe's primary rpath is @executable_path / $ORIGIN, so any .{dylib,so,dll}
    //     named libDaScriptDyn* (or its libliblib... cmake variant) shipped here
    //     resolves at runtime. CMake puts the RUNTIME artifact (Windows .dll) in
    //     `<das_root>/bin`, while LIBRARY artifacts (.dylib / .so) go in
    //     `<das_root>/lib` — search the correct dir per platform.
    let platform = get_platform_name()
    // Source the runtime libs from the EXACT build that compiled the exe. On Windows the DLLs
    // sit next to the daslang binary used above (get_daslang_path) — bin/ or bin/<config>/. A
    // recursive bin/** glob crosses config subdirs (Release/Debug/RelWithDebInfo) and, copying
    // same-named files last-wins, ships a MISMATCHED build: a stale DLL missing a symbol the exe
    // imports then fails at load with STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139). Globbing the
    // codegen binary's own dir non-recursively pins the matching pair. On Unix the .so/.dylib
    // live in a single lib/, so the recursive walk there is safe.
    let daslang_dir = dir_name(replace(get_daslang_path(), "\\", "/"))
    let runtime_lib_dir = (platform == "windows"
        ? daslang_dir
        : "{get_das_root()}/lib")
    let runtime_glob = (platform == "windows" ? "libDaScriptDyn*.dll"
        : platform == "darwin" ? "**/*libDaScriptDyn*.dylib"
        : "**/*libDaScriptDyn*.so")
    var runtime_copy_failed = false
    var runtime_copies = 0
    glob(runtime_lib_dir, runtime_glob) $(rel_path, is_dir) {
        if (is_dir) return
        let src = path_join(runtime_lib_dir, rel_path)
        let dst = path_join(bundle_dir, base_name(rel_path))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            runtime_copy_failed = true
            return
        }
        runtime_copies++
        log("  ship runtime: {base_name(rel_path)}\n")
        finalize_shipped_binary(src, dst, shipped, base_name(rel_path))
        shipped |> push(base_name(rel_path))
    }
    if (runtime_copy_failed) return 1

    // 2.6 Ship the app tune sidecar beside the exe as <bundle>.tune.json — the exe's own
    //     runtime resolution (`<exe-stem>.tune.json`) finds it there: the "runtime" knob
    //     section travels with the artifact and the file documents the baked winners
    //     (provenance included). Touch it NEWER than the exe: the sidecar predates the
    //     build by construction, and a stale-looking sidecar would be ignored at runtime.
    // prefer the tune scopes' RESOLVED manifest (honors DAS_TUNE_MANIFEST redirects) over
    // the next-to-root guess — shipping the guess put a stale July sidecar beside a
    // freshly-minted exe on M4's first standalone run
    let sidecar_default = "{slice(main_path, 0, length(main_path) - 4)}.tune.json"
    let sidecar_src = (!empty(ship_manifest) && fexist(ship_manifest)) ? ship_manifest : sidecar_default
    if (fexist(sidecar_src)) {
        let sidecar_rel = "{bundle_name}.tune.json"
        let sidecar_dst = path_join(bundle_dir, sidecar_rel)
        var serr : string
        if (!copy_file(sidecar_src, sidecar_dst, true, serr)) {
            to_log(LOG_ERROR, "release: copy_file({sidecar_src} -> {sidecar_dst}) failed: {serr}\n")
            return 1
        }
        var terr : string
        // the ship contract is STRICTLY newer than the exe; a whole-second "now" stamp can
        // only tie a same-second exe write, so anchor to the exe's final mtime instead —
        // future-dating is harmless (staleness only checks older-than-exe)
        var exe_fs : FStat
        let ahead = clock(int64(stat(exe_path, exe_fs) ? exe_fs.mtime : get_clock()) + 5l)
        if (!set_mtime(sidecar_dst, ahead, terr)) {
            to_log(LOG_WARNING, "release: cannot touch {sidecar_dst}: {terr} — the shipped sidecar may read as stale\n")
        }
        log("  ship: {sidecar_rel} (tune sidecar)\n")
        shipped |> push(sidecar_rel)
    }
    if (runtime_copies == 0) {
        to_log(LOG_ERROR, "release: no daslang runtime libs matched `{runtime_glob}` under `{runtime_lib_dir}`; bundle would not load on a relocated machine\n")
        return 1
    }

    // 3. Transitive dep traversal — for each unique dep .das_package (excluding
    //    the project's own root), run release() and copy its release_include
    //    files to <bundle>/modules/<DepName>/<rel>.
    let project_root_abs = get_full_file_name(root)
    var seen_pkg_dirs : table<string; bool>
    seen_pkg_dirs[project_root_abs] = true
    let bundle_modules_dir = path_join(bundle_dir, "modules")
    for (dm in deps.das_modules) {
        let pkg_abs = get_full_file_name(dm.package_dir)
        if (key_exists(seen_pkg_dirs, pkg_abs)) continue
        seen_pkg_dirs[pkg_abs] = true
        let rc2 = ship_dep_assets(pkg_abs, bundle_modules_dir, shipped)
        if (rc2 != 0) return rc2
    }

    // 4. Glob the project's own assets per spec, copy preserving relative paths.
    var asset_files : array<string>
    glob_filtered(root, spec.include_globs, spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            asset_files |> push(rel_path)
        }
    }
    shipped |> reserve(length(shipped) + length(asset_files))
    for (rel_path in asset_files) {
        let src = path_join(root, rel_path)
        let dst = path_join(bundle_dir, rel_path)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
        shipped |> push(rel_path)
    }
    if (!empty(asset_files)) {
        log("  shipped {length(asset_files)} project asset file(s)\n")
    }
    // Mutable starter files are copied only for a new deployment. They are
    // intentionally not added to `shipped`, so the next manifest cleanup
    // treats the deployed copy as user-owned and leaves it alone.
    var starter_files : array<string>
    glob_filtered(root, spec.include_if_missing_globs, spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            starter_files |> push(rel_path)
        }
    }
    var starter_copies = 0
    for (rel_path in starter_files) {
        let src = path_join(root, rel_path)
        let dst = path_join(bundle_dir, rel_path)
        if (fexist(dst)) continue
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, false, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
        starter_copies++
    }
    if (starter_copies > 0) {
        log("  initialized {starter_copies} user-owned project file(s)\n")
    }
    // Project-declared native deps land next to the exe (the project has no
    // .shared_module to colocate with — its deps are loaded directly by the exe).
    if (!empty(spec.native_dlls)) {
        let rc3 = ship_native_dlls(bundle_name, bundle_dir, spec.native_dlls, shipped, "")
        if (rc3 != 0) return rc3
    }
    if (!empty(spec.external_files)) {
        let rc4 = ship_external_files(bundle_name, bundle_dir, spec.external_files, shipped)
        if (rc4 != 0) return rc4
    }

    // 5. macOS .app: write Contents/Info.plist. Bundle ID source priority:
    //    explicit release_bundle_id() > derived com.<sanitized-author>.<sanitized-name>
    //    > com.daspkg.<sanitized-name>. Apple requires CFBundleIdentifier to use
    //    only [A-Za-z0-9.-], so the derived default sanitizes both pieces;
    //    user-supplied release_bundle_id() passes through untouched.
    if (is_mac_app) {
        var bundle_id = spec.bundle_id
        if (empty(bundle_id)) {
            let author_raw = (have_pkg && !empty(pkg.author)) ? pkg.author : "daspkg"
            bundle_id = "com.{sanitize_bundle_id_part(author_raw)}.{sanitize_bundle_id_part(bundle_name)}"
        }
        let plist_path = path_join(app_root, "Contents/Info.plist")
        if (!fwrite(plist_path, build_info_plist(bundle_name, bundle_name, bundle_id))) {
            to_log(LOG_ERROR, "release: cannot write {plist_path}\n")
            return 1
        }
        log("  wrote: Contents/Info.plist (CFBundleIdentifier={bundle_id})\n")
    }
    // every release writes the ownership manifest — the .app too: the wipe path does not
    // consult it, but tooling and the migration semantics read one record everywhere
    let mbody = build_string() $(var writer) {
        for (s in shipped) {
            writer |> write(s)
            writer |> write("\n")
        }
    }
    if (!fwrite(path_join(bundle_dir, RELEASE_MANIFEST), mbody)) {
        to_log(LOG_ERROR, "release: cannot write {RELEASE_MANIFEST}\n")
        return 1
    }
    log("Released to: {is_mac_app ? app_root : bundle_dir} ({release_elapsed(release_t0)}s)\n")
    return 0
}

// ── release wasm (wasm64 compiled artifacts) ───────────────────────────
//
// `daspkg release wasm` cross-compiles each app to a wasm64 object (daslang
// -exe --jit-target=wasm64 --jit-emit-object), discovers the modules it links
// via --list-shared-modules, then runs emcc with the wasm64 module archives +
// release defaults + each module's own emcc contributions → <app>.{html,js,wasm}.

// Derive the wasm64 static archive name from a shared module's deps-JSON rel
// path: modules/dasGlfw/dasModuleGlfw.shared_module → liblibDasModuleGlfw.a
// (the name the web/ build emits with -DDAS_WASM_MEMORY64=ON).
def private wasm_archive_name(rel : string) : string {
    var base = base_name(rel)
    let suffix = ".shared_module"
    if (base |> ends_with(suffix)) {
        base = slice(base, 0, length(base) - length(suffix))
    }
    // CMake emits the archive after its target case (libDasModule*); the file is
    // lowercase dasModule*. Capitalize so the lookup hits on case-sensitive FS (Linux CI).
    if (!empty(base)) {
        base = "{to_upper(slice(base, 0, 1))}{slice(base, 1)}"
    }
    return "liblib{base}.a"
}

def private wasm_path_key(path : string) : string {
    let full_path = get_full_file_name(path)
    return get_platform_name() == "windows" ? to_lower(full_path) : full_path
}

def private push_unique_archive(var archives : array<string>; archive : string) {
    let archive_key = wasm_path_key(archive)
    for (existing in archives) {
        if (wasm_path_key(existing) == archive_key) return
    }
    archives |> push(archive)
}

// Run a referenced module's .das_package release() (if present) and accumulate
// every archive, emcc flag, and MEMFS embed it contributes. The module archive
// discovered from deps JSON is only the package's public entry point; declared
// archives also carry its private static-link closure (for example FreeType
// behind dasImgui). Deduped by package dir and archive path.
def private gather_wasm_module(pkg_dir, wasm_lib_dir : string;
                               var archives, emcc_args, embeds : array<string>;
                               var seen : table<string>) : bool {
    let key = wasm_path_key(pkg_dir)
    if (key_exists(seen, key)) return true
    seen |> insert(key)
    let pkg_file = "{pkg_dir}/.das_package"
    if (!fexist(pkg_file)) return true
    var info : PackageReleaseInfo
    if (!run_das_package_release(pkg_file, info)) return true
    for (a in info.wasm_archives) {
        let staged = path_join(wasm_lib_dir, base_name(a))
        if (!fexist(staged)) {
            to_log(LOG_ERROR, "release wasm: declared wasm archive was not staged: {staged}\n")
            return false
        }
        push_unique_archive(archives, staged)
    }
    emcc_args |> push_from(info.emcc_args)
    // release_embed_file() src is module-relative (like release_main / release_wasm_archive); resolve it
    // against the module dir HERE, where pkg_dir is known, into an absolute path. An external module
    // lives under <project>/modules/<dep>, NOT <das_root>/modules/<dep>, so a das_root-relative resolve
    // at link time would miss its embeds (e.g. dasImgui's font).
    for (e in info.embed_paths) {
        let at = find(e, "@")
        if (at < 0) continue
        let abs_src = get_full_file_name(path_join(pkg_dir, slice(e, 0, at)))
        embeds |> push("{abs_src}@{slice(e, at + 1)}")
    }
    return true
}

// An EXTERNAL module (not part of daslang's in-tree web build, so `daspkg build --wasm`
// never produced its archives) can declare in its .das_package how to build them:
// release_wasm_build(command) + release_wasm_archive(path). When a released app links
// such a module and any declared archive is not already staged, run that build once
// (cwd = the module dir) and copy the declared archives into wasm_lib_dir. Deduped by
// module dir, so a module that emits several archives builds only once.
def private ensure_external_wasm_archives(pkg_dir, wasm_lib_dir : string; var built : table<string>) {
    let key = wasm_path_key(pkg_dir)
    return if (key_exists(built, key))
    built |> insert(key)
    let pkg_file = "{pkg_dir}/.das_package"
    return if (!fexist(pkg_file))
    var info : PackageReleaseInfo
    return if (!run_das_package_release(pkg_file, info))
    // A complete cache needs every declared archive, not just the native module's
    // one-to-one liblibDasModule*.a. Rebuild a partial external package cache so
    // private dependency archives cannot disappear from a later link.
    var all_staged = true
    for (a in info.wasm_archives) {
        if (!fexist(path_join(wasm_lib_dir, base_name(a)))) {
            all_staged = false
        }
    }
    return if (all_staged || empty(info.wasm_build_command))
    log("release wasm: building wasm archives for external module at {pkg_dir}...\n")
    let cd = get_platform_name() == "windows" ? "cd /d" : "cd"
    var bld_out : string
    let bld_rc = run_cmd("{cd} \"{pkg_dir}\" && {info.wasm_build_command}", bld_out)
    if (bld_rc != 0) {
        to_log(LOG_ERROR, "release wasm: wasm build failed for external module at {pkg_dir} (rc={bld_rc}):\n{bld_out}\n")
        return
    }
    mkdir_rec(wasm_lib_dir)
    for (a in info.wasm_archives) {
        let src = is_absolute(a) ? a : path_join(pkg_dir, a)
        let dst = path_join(wasm_lib_dir, base_name(a))
        if (!fexist(src)) {
            // A bare archive name may declare an SDK archive already staged by
            // `daspkg build --wasm` (for example dasImgui -> clipboard).
            if (fexist(dst)) continue
            to_log(LOG_ERROR, "release wasm: declared wasm archive not found after build: {src}\n")
            continue
        }
        continue if (wasm_path_key(src) == wasm_path_key(dst))
        var err : string
        if (copy_file(src, dst, true, err)) {
            log("  staged {base_name(a)}\n")
        } else {
            to_log(LOG_ERROR, "release wasm: copy_file({src} -> {dst}) failed: {err}\n")
        }
    }
}

// Build one app: cross-compile → discover modules → emcc-link → stage assets.
def private release_one_wasm_app(root, out_dir, app_name, main_script : string; // nolint:STYLE037,STYLE038 — flat per-app wasm release steps + their error arms
                                 wasm_lib_dir, runtime_archive, shell_path : string;
                                 include_globs, exclude_globs : array<string>) : int {
    if (!is_safe_pkg_name(app_name)) {
        to_log(LOG_ERROR, "release wasm: unsafe app name `{app_name}` (no path separators / `.` / `..`)\n")
        return 1
    }
    let main_path = path_join(root, main_script)
    if (!fexist(main_path)) {
        to_log(LOG_ERROR, "release wasm: app `{app_name}` main script not found: {main_path}\n")
        return 1
    }
    let app_out_dir = path_join(out_dir, app_name)
    if (fexist(app_out_dir)) {
        force_rmdir(app_out_dir)
    }
    mkdir_rec(app_out_dir)

    let daslang = get_daslang_path()
    // Intermediates live beside the output, NOT under das_root: the SDK root may be
    // read-only (installed SDK, or the buildd sandbox's ro-mounted worktree), and
    // app_out_dir was just created so it is always writable.
    run_cmd_counter ++
    let obj_path = path_join(app_out_dir, "_daspkg_wasm_{run_cmd_counter}.o")
    let deps_file = path_join(app_out_dir, "_daspkg_wasm_deps_{run_cmd_counter}.json")

    // 1. Cross-compile to a wasm64 object (link skipped via --jit-emit-object),
    //    with --list-shared-modules writing the referenced-module set.
    //    --disable-module dashv: libhv is native-only (no wasm archive), so keep it
    //    unloaded on the cross-compile host — a guarded `require ?dashv` then resolves
    //    as absent instead of pulling in a module whose wasm archive can't exist.
    //    -project-root {root}: dyn-module discovery roots at the PACKAGE dir, not the
    //    main script's dir. They differ when release_main() points outside the package
    //    (e.g. an example whose .das_package sits beside the source) — without this the
    //    cross-compile scans the script's folder and misses the package's modules/<dep>.
    // --jit-threads: emit +atomics,+bulk-memory so the object's ABI matches the -pthread runtime
    // archive (the wasm build is threaded — see cmd_build_wasm). The link below adds -pthread.
    let xc_cmd = "\"{daslang}\" -exe -output \"{obj_path}\" --list-shared-modules \"{deps_file}\" -project-root \"{root}\" --disable-module dashv \"{main_path}\" -- --jit-target=wasm64-unknown-emscripten --jit-emit-object --jit-threads --jit-runtime-lib=\"{runtime_archive}\""
    var xc_out : string
    let xc_rc = run_cmd(xc_cmd, xc_out)
    if (xc_rc != 0 || !fexist(obj_path)) {
        to_log(LOG_ERROR, "release wasm: cross-compile failed for `{app_name}` (rc={xc_rc}):\n{xc_out}\n")
        if (fexist(deps_file)) {
            remove(deps_file)
        }
        return 1
    }

    // 2. Read the referenced-module set.
    var deps : ReleaseDeps
    if (!read_release_deps(deps_file, deps)) {
        to_log(LOG_ERROR, "release wasm: cannot read module deps for `{app_name}`\n")
        remove(obj_path)
        return 1
    }
    remove(deps_file)

    // 2.5 GL-fallback guard. A graphics program that links glfw but whose opengl
    //     did NOT resolve to the native module fell back to the broken dasbind GL
    //     late-extern path (cross-compiles fine, but black canvas at runtime).
    var uses_glfw = false
    var uses_opengl = false
    for (e in deps.shared_modules) {
        if (e.das_name == "glfw") {
            uses_glfw = true
        }
        if (e.das_name == "opengl") {
            uses_opengl = true
        }
    }
    if (uses_glfw && !uses_opengl) {
        to_log(LOG_ERROR, "release wasm: `{app_name}` links glfw but opengl did not bind as a native module — the cross-compile fell back to the broken dasbind GL path (black canvas). Ensure modules/dasOpenGL/dasModuleOpenGL.shared_module exists at cross-compile time, then rebuild.\n")
        remove(obj_path)
        return 1
    }

    // 3. Gather wasm64 archives + per-module emcc args / embeds.
    var archives : array<string>
    var emcc_args : array<string>
    var embeds : array<string>
    var seen : table<string>
    var built_external : table<string>
    for (e in deps.shared_modules) {
        if (empty(e.rel)) continue
        let arch = "{wasm_lib_dir}/{wasm_archive_name(e.rel)}"
        // In-tree archives come from `daspkg build --wasm`; an external module instead
        // declares its own wasm build in its .das_package (release_wasm_build).
        // Check the whole declaration even when this primary archive is cached:
        // a partial cache may be missing a private dependency archive.
        ensure_external_wasm_archives(dir_name(e.abs), wasm_lib_dir, built_external)
        if (!fexist(arch)) {
            to_log(LOG_ERROR, "release wasm: missing module archive {arch} (module `{e.das_name}`); for an in-tree module run `daspkg build --wasm`, for an external module declare release_wasm_build/release_wasm_archive in its .das_package\n")
            remove(obj_path)
            return 1
        }
        push_unique_archive(archives, arch)
    }
    // Primary module archives stay in dependency-discovery order. Append each
    // package's declared private closure afterward, preserving declaration order.
    for (e in deps.shared_modules) {
        if (empty(e.rel)) continue
        if (!gather_wasm_module(dir_name(e.abs), wasm_lib_dir, archives, emcc_args, embeds, seen)) {
            remove(obj_path)
            return 1
        }
    }
    for (dm in deps.das_modules) {
        ensure_external_wasm_archives(dm.package_dir, wasm_lib_dir, built_external)
        if (!gather_wasm_module(dm.package_dir, wasm_lib_dir, archives, emcc_args, embeds, seen)) {
            remove(obj_path)
            return 1
        }
    }

    // 4. emcc link → <app>.{html,js,wasm}. run_cmd handles the Windows
    //    cmd.exe outer-quote wrapping, so quoted paths survive on every host.
    let html_out = path_join(app_out_dir, "{app_name}.html")
    let emcc = get_platform_name() == "windows" ? "emcc.bat" : "emcc"
    let link_cmd = build_string() $(var w) {
        w |> write("\"{emcc}\" \"{obj_path}\"")
        for (a in archives) {
            w |> write(" \"{a}\"")
        }
        w |> write(" \"{runtime_archive}\"")
        // Release defaults: wasm64 + WebGL2/ES3 + modern wasm EH + filesystem + threads. -pthread
        // matches the threaded runtime archive (new_thread / JobQue map to Web Workers); the pool is
        // pre-sized for the DAS_MAX_HW_JOBS-capped worker count. A -pthread module only instantiates
        // cross-origin-isolated, so the host page must send COOP same-origin + COEP require-corp.
        // AUDIO_WORKLET + WASM_WORKERS: under -pthread, dasAudio builds its AudioWorklet backend
        // (MA_ENABLE_AUDIO_WORKLETS), which references emscripten_*_audio_worklet_* — those symbols
        // come from -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 at the link (matches web/CMakeLists' pthread block).
        // -Wl,--wrap=emscripten_futex_wait: the AudioWorklet is a Wasm Worker (cannot Atomics.wait,
        // not the main browser thread); emscripten's futex path asserts main-thread-only and ABORTS
        // when the worklet contends ANY pthread_mutex (dlmalloc's arena lock, the dasAudio mixer's
        // context/jobque mutexes). __wrap_emscripten_futex_wait (src/hal/wasm_thread_malloc.cpp)
        // busy-polls on the worklet instead, delegating all other threads to the real impl. No
        // emscripten fork needed — the wrap + shim live in libDaScript_runtime.a (like the malloc lock).
        w |> write(" -sMEMORY64=1 -sFULL_ES3 -sMAX_WEBGL_VERSION=2 -sGL_ENABLE_GET_PROC_ADDRESS=1 -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=4MB -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=0 -sEXIT_RUNTIME=0 -sFORCE_FILESYSTEM=1 -pthread -sPTHREAD_POOL_SIZE=16 -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -Wl,--wrap=emscripten_futex_wait")
        for (a in emcc_args) {
            w |> write(" {a}")
        }
        for (e in embeds) {
            let at = find(e, "@")
            if (at >= 0) {
                // src is already an absolute path resolved against the owning module dir (gather_wasm_module).
                let src = slice(e, 0, at)
                let dst = slice(e, at + 1)
                w |> write(" --embed-file \"{src}@{dst}\"")
            }
        }
        w |> write(" --shell-file \"{shell_path}\" -o \"{html_out}\"")
    }
    var link_out : string
    let link_rc = run_cmd(link_cmd, link_out)
    remove(obj_path)
    if (link_rc != 0) {
        to_log(LOG_ERROR, "release wasm: emcc link failed for `{app_name}` (rc={link_rc}):\n{link_out}\n")
        return 1
    }

    // 5. Stage package assets next to the artifact.
    var asset_files : array<string>
    glob_filtered(root, include_globs, exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            asset_files |> push(rel_path)
        }
    }
    for (rel_path in asset_files) {
        let src = path_join(root, rel_path)
        let dst = path_join(app_out_dir, rel_path)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release wasm: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
    }
    log("  released {app_name} -> {app_out_dir} ({app_name}.html / .js / .wasm)\n")
    return 0
}

// `daspkg release wasm` entry. Releases the single app the package's .das_package
// declares (release_main()) into <out>/<name>/. Each example is its own package
// (its .das_package declares both requirements and packaging) — CI loops over the
// example dirs, releasing each. Mirrors native cmd_release's single-app shape.
def cmd_release_wasm(root : string; out_dir : string; wasm_lib_dir_override : string) : int {
    let das_package = "{root}/.das_package"
    if (!fexist(das_package)) {
        to_log(LOG_ERROR, "release wasm: no .das_package in {root}\n")
        return 1
    }
    var spec : PackageReleaseInfo
    if (!run_das_package_release(das_package, spec)) {
        to_log(LOG_ERROR, "release wasm: cannot run release() in {das_package}\n")
        return 1
    }
    // wasm entry: release_wasm_main() (e.g. a live_stub variant that drops the
    // desktop live stack) overrides release_main() for the wasm64 build.
    let main_script = empty(spec.wasm_main_script) ? spec.main_script : spec.wasm_main_script
    if (empty(main_script)) {
        to_log(LOG_ERROR, "release wasm: {das_package} has no release() hook, or release() didn't call release_main()/release_wasm_main()\n")
        return 1
    }

    var pkg : PackageMeta
    let have_pkg = run_das_package_meta(das_package, pkg)
    var bundle_name = spec.name
    if (empty(bundle_name)) {
        bundle_name = (have_pkg && !empty(pkg.pkg_name)) ? pkg.pkg_name : base_name(get_full_file_name(root))
    }

    // Archive cache (`daspkg build --wasm` output) + the wasm64 runtime archive.
    let wasm_lib_dir = empty(wasm_lib_dir_override) ? "{get_das_root()}/web/output64/lib" : wasm_lib_dir_override
    let runtime_archive = "{wasm_lib_dir}/liblibDaScript_runtime.a"
    if (!fexist(runtime_archive)) {
        to_log(LOG_ERROR, "release wasm: no wasm64 runtime archive at {runtime_archive}. Build it with `daspkg build --wasm` (or pass --wasm-lib-dir <dir>).\n")
        return 1
    }

    // Shell: explicit release_web_shell() > daslang's default canvas shell.
    let shell_path = (!empty(spec.web_shell)
        ? path_join(root, spec.web_shell)
        : "{get_das_root()}/web/templates/wasm_canvas_shell.html")
    if (!fexist(shell_path)) {
        to_log(LOG_ERROR, "release wasm: shell file not found: {shell_path}. Set release_web_shell(<path>) in .das_package.\n")
        return 1
    }

    mkdir_rec(out_dir)
    return release_one_wasm_app(root, out_dir, bundle_name, main_script, wasm_lib_dir, runtime_archive, shell_path, spec.include_globs, spec.exclude_globs)
}
