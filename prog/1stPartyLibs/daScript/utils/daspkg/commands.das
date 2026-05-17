options gen2
options rtti
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false

module commands shared public

require daslib/fio
require strings
require daslib/json
require daslib/json_boost

require daslib/ansi_colors
require daslib/clargs public

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
    if (length(lf.packages) == 0) {
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
    if (is_local_path(source)) {
        return source
    }
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
    if (!git_clone(git_url, tmp_dir)) {
        return 1
    }

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
                if (!git_clone(redirect_url, tmp_dir)) {
                    return 1
                }
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
            if (!force) {
                return 1
            }
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
    if (!run_das_package_deps(das_package, "", pkg_deps) || length(pkg_deps) == 0) {
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
    if (!git_clone(git_url, tmp_dir)) {
        return false
    }
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
    if (!fexist(das_package)) {
        return
    }
    var pkg_deps : array<PackageDependency>
    if (!run_das_package_deps(das_package, "", pkg_deps)) {
        return
    }
    for (d in pkg_deps) {
        let dep_name = package_name_from_source(d.source)
        if (!has_package(lf, dep_name)) {
            // new dependency — install it
            log("  Installing new dependency: {dep_name}...\n")
            let dep_source = resolve_dep_source(root, d.source)
            if (empty(dep_source)) {
                continue
            }
            var visiting : table<string; bool>
            let drc = install_git(root, dep_name, dep_source, "", lf, visiting, [is_root = false, is_global = is_global])
            if (drc != 0) {
                to_log(LOG_ERROR, "Error: failed to install new dependency '{dep_name}' (rc={drc})\n")
            }
            continue
        }
        // check if installed version satisfies constraint
        if (empty(d.version_constraint)) {
            continue
        }
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
    var names : array<string>
    for (entry in lf.packages) {
        if (entry.root && !entry.local) {
            names |> push_clone(entry.name)
        }
    }
    if (length(names) == 0) {
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
    exit_code = run_cmd("cmake --build \"{build_dir}\" --config Release", output)
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
        if (name == "." || name == ".." || starts_with(name, ".")) {
            return
        }
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
    if (failed > 0) {
        return 1
    }
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
    if (empty(name)) {
        return false
    }
    if (name == "." || name == "..") {
        return false
    }
    if (find(name, "/") >= 0 || find(name, "\\") >= 0) {
        return false
    }
    return true
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
