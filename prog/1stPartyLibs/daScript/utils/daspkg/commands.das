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

def install_dependencies(root, pkg_name : string; var lf : LockFile; var visiting : table<string; bool>; is_global : bool = false) {
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

def install_git(root, pkg_name, source, version : string; var lf : LockFile; var visiting : table<string; bool>; is_root : bool = true; is_global : bool = false; branch : string = "") {
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

def cmd_install(root, spec : string; force : bool; is_global : bool = false; version_constraint : string = ""; branch : string = "") {
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

def cmd_update(root, name : string; is_global : bool = false) {
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
        names |> push_clone(entry.name)
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

def cmd_cleanup(root : string; force : bool; is_global : bool = false) : int {
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

struct private ReleaseDeps {
    shared_modules : array<SharedModuleEntry>
    das_modules : array<DasModuleEntry>
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
    deps = from_JV(parsed, type<ReleaseDeps>)
    return true
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
def private ship_dep_assets(pkg_dir, bundle_modules_dir : string) : int {
    let dep_pkg_file = "{pkg_dir}/.das_package"
    if (!fexist(dep_pkg_file)) return 0
    var dep_spec : PackageReleaseInfo
    if (!run_das_package_release(dep_pkg_file, dep_spec) ||
        (empty(dep_spec.include_globs) && empty(dep_spec.native_dlls))) return 0
    let dep_name = base_name(pkg_dir)
    let dep_dst_dir = "{bundle_modules_dir}/{dep_name}"
    var dep_files : array<string>
    glob_filtered(pkg_dir, dep_spec.include_globs, dep_spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            dep_files |> push_clone(rel_path)
        }
    }
    for (rel in dep_files) {
        let src = path_join(pkg_dir, rel)
        let dst = path_join(dep_dst_dir, rel)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
    }
    if (!empty(dep_files)) {
        log("  ship dep `{dep_name}`: {length(dep_files)} file(s)\n")
    }
    if (!empty(dep_spec.native_dlls)) {
        let rc = ship_native_dlls(dep_name, dep_dst_dir, dep_spec.native_dlls)
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
def private preserve_dylib_symlinks(src_dir, dst_dir, src_name : string) {
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
            rpaths |> push_clone(p)
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
            kept |> push_clone(e)
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
def private ensure_dylib_alias(dst : string) {
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
    } else {
        to_log(LOG_WARNING, "release: ln -s {base_name(dst)} -> {link_path} failed (rc={ln_rc}): {ln_out}\n")
    }
}

// One-liner used at every ship site: preserve any sibling symlink chain from
// the source, materialize the install_name alias if it's still missing
// (macOS), then strip absolute rpath entries from the copy.
def private finalize_shipped_binary(src, dst : string) {
    preserve_dylib_symlinks(dir_name(src), dir_name(dst), base_name(src))
    ensure_dylib_alias(dst)
    scrub_external_rpaths(dst)
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
def private ship_native_dlls(dep_name, dst_dir : string; native_dlls : array<string>) : int {
    let platform = get_platform_name()
    let search_root = (platform == "windows"
        ? "{get_das_root()}/bin"
        : "{get_das_root()}/lib")
    mkdir_rec(dst_dir)
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
        finalize_shipped_binary(found_src, dst)
    }
    return 0
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

def cmd_release(root : string; out_dir : string) : int {
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
    let cleanup_target = is_mac_app ? app_root : bundle_dir
    if (fexist(cleanup_target)) {
        force_rmdir(cleanup_target)
    }
    mkdir_rec(bundle_dir)

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
    let rc = run_cmd(cmd, output)
    if (rc != 0) {
        to_log(LOG_ERROR, "release: daslang -exe failed (rc={rc}):\n{output}\n")
        if (fexist(deps_file)) {
            remove(deps_file)
        }
        return 1
    }
    // daslang -exe writes <exe_path>.exe (the runnable binary) and <exe_path>.o
    // (a build-time object file). Drop the .o — it's not needed at runtime.
    let obj_path = "{exe_path}.o"
    if (fexist(obj_path)) {
        remove(obj_path)
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
    // portable in the first place.
    scrub_external_rpaths(exe_path)

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
        finalize_shipped_binary(e.abs, dst)
    }

    // 2.5 Ship daslang runtime dylibs next to the exe. Without these the bundle
    //     dyld-loads via the build-machine fallback rpath and crashes on relocation.
    //     The exe's primary rpath is @executable_path / $ORIGIN, so any .{dylib,so,dll}
    //     named libDaScriptDyn* (or its libliblib... cmake variant) shipped here
    //     resolves at runtime. CMake puts the RUNTIME artifact (Windows .dll) in
    //     `<das_root>/bin`, while LIBRARY artifacts (.dylib / .so) go in
    //     `<das_root>/lib` — search the correct dir per platform.
    let platform = get_platform_name()
    let runtime_lib_dir = (platform == "windows"
        ? "{get_das_root()}/bin"
        : "{get_das_root()}/lib")
    // Patterns are `**/...` so the recursive walk crosses config subdirs
    // (Visual Studio / Xcode put the artifact in `bin/Release/`, not `bin/`).
    let runtime_glob = (platform == "windows" ? "**/libDaScriptDyn*.dll"
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
        finalize_shipped_binary(src, dst)
    }
    if (runtime_copy_failed) return 1
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
        let rc2 = ship_dep_assets(pkg_abs, bundle_modules_dir)
        if (rc2 != 0) return rc2
    }

    // 4. Glob the project's own assets per spec, copy preserving relative paths.
    var asset_files : array<string>
    glob_filtered(root, spec.include_globs, spec.exclude_globs) $(rel_path, is_dir) {
        if (!is_dir) {
            asset_files |> push_clone(rel_path)
        }
    }
    for (rel_path in asset_files) {
        let src = path_join(root, rel_path)
        let dst = path_join(bundle_dir, rel_path)
        mkdir_rec(dir_name(dst))
        var err : string
        if (!copy_file(src, dst, true, err)) {
            to_log(LOG_ERROR, "release: copy_file({src} -> {dst}) failed: {err}\n")
            return 1
        }
    }
    if (!empty(asset_files)) {
        log("  shipped {length(asset_files)} project asset file(s)\n")
    }
    // Project-declared native deps land next to the exe (the project has no
    // .shared_module to colocate with — its deps are loaded directly by the exe).
    if (!empty(spec.native_dlls)) {
        let rc3 = ship_native_dlls(bundle_name, bundle_dir, spec.native_dlls)
        if (rc3 != 0) return rc3
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
        log("Released to: {app_root}\n")
    } else {
        log("Released to: {bundle_dir}\n")
    }
    return 0
}
