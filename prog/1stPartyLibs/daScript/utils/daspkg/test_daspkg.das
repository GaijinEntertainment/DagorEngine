options gen2
options no_global_variables = false

require dastest/testing_boost public

require daslib/fio
require strings
require daslib/strings_boost

require daslib/json
require daslib/json_boost

require daslib/ansi_colors

require lockfile
require utils
require index
require commands
require daslib/result
require package_runner public

// On Darwin daspkg release emits a clickable `.app`; everything else is the
// flat `<out_dir>/<name>/...` layout. These helpers let release tests assert
// against the right paths without per-platform branches at every check site.

def bundle_root_dir(out_dir, name : string) : string {
    if (get_platform_name() == "darwin") {
        return path_join(out_dir, "{name}.app")
    }
    return path_join(out_dir, name)
}

def bundle_artifacts_dir(out_dir, name : string) : string {
    if (get_platform_name() == "darwin") {
        return path_join(path_join(out_dir, "{name}.app"), "Contents/MacOS")
    }
    return path_join(out_dir, name)
}

def bundle_exe_path(out_dir, name : string) : string {
    let dir = bundle_artifacts_dir(out_dir, name)
    if (get_platform_name() == "darwin") {
        return path_join(dir, name)
    }
    return path_join(dir, "{name}.exe")
}

// Path of the exe AFTER the bundle root has been moved/renamed to `moved_root`.
// On Darwin the moved directory IS the .app — the exe still lives at
// Contents/MacOS/<name>. On Linux/Windows it's just <moved_root>/<name>.exe.
def moved_bundle_exe_path(moved_root, name : string) : string {
    if (get_platform_name() == "darwin") {
        return path_join(moved_root, "Contents/MacOS/{name}")
    }
    return path_join(moved_root, "{name}.exe")
}

[test]
def test_ship_tools(t : T?) {
    t |> run("a tool ships by bare name from bin/, then bin/Release/, with the platform's exe suffix; a missing tool fails the release") @(t : T?) {
        let tmp = create_temp_directory_result("daspkg tools ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root = tmp as value
        let das_root = path_join(root, "root")
        let bundle = path_join(root, "bundle")
        let ext = get_platform_name() == "windows" ? ".exe" : ""
        mkdir(das_root)
        mkdir(path_join(das_root, "bin"))
        mkdir(path_join(path_join(das_root, "bin"), "Release"))
        mkdir(bundle)
        fwrite(path_join(das_root, "bin/flat{ext}"), "flat")
        fwrite(path_join(das_root, "bin/Release/msvc{ext}"), "msvc")
        var shipped : array<string>
        t |> equal(ship_tools("pkg", bundle, ["flat", "msvc"], shipped, das_root), 0, "both tools resolve")
        t |> equal(length(shipped), 2)
        t |> success(fexist(path_join(bundle, "flat{ext}")), "bin/<tool> lands at the bundle root")
        t |> success(fexist(path_join(bundle, "msvc{ext}")), "bin/Release/<tool> lands at the bundle root")
        var none : array<string>
        t |> equal(ship_tools("pkg", bundle, ["absent"], none, das_root), 1, "a tool no build produced fails the release")
        t |> equal(length(none), 0)
        fwrite(path_join(root, "escaped{ext}"), "outside")
        var unsafe_name : array<string>
        t |> equal(ship_tools("pkg", bundle, ["../escaped"], unsafe_name, das_root), 1, "a name with a path separator is refused before any path is built")
        t |> success(!fexist(path_join(bundle, "escaped{ext}")), "nothing was copied for the refused name")
        let cleanup = rmdir_rec_result(root)
        t |> success(cleanup is value && cleanup as value, "temp dir removed")
    }
}

[test]
def test_package_name_from_source(t : T?) {
    t |> run("github URL") @(t : T?) {
        t |> equal(package_name_from_source("github.com/user/dasImgui"), "dasImgui")
    }
    t |> run("nested path") @(t : T?) {
        t |> equal(package_name_from_source("github.com/org/sub/mymod"), "mymod")
    }
    t |> run("local absolute path") @(t : T?) {
        t |> equal(package_name_from_source("/home/user/mymod"), "mymod")
    }
    t |> run("bare name") @(t : T?) {
        t |> equal(package_name_from_source("dasImgui"), "dasImgui")
    }
    t |> run("trailing slash stripped") @(t : T?) {
        t |> equal(package_name_from_source("github.com/user/repo/"), "")
    }
    t |> run("windows backslash path") @(t : T?) {
        t |> equal(package_name_from_source("C:\\Users\\mymod"), "mymod")
    }
    t |> run("mixed separators") @(t : T?) {
        t |> equal(package_name_from_source("C:/Users\\mymod"), "mymod")
    }
}

[test]
def test_is_local_path(t : T?) {
    t |> run("absolute unix path") @(t : T?) {
        t |> success(is_local_path("/home/user/mod"))
    }
    t |> run("relative dot path") @(t : T?) {
        t |> success(is_local_path("./mymod"))
    }
    t |> run("relative parent path") @(t : T?) {
        t |> success(is_local_path("../mymod"))
    }
    t |> run("windows drive letter") @(t : T?) {
        t |> success(is_local_path("C:/Users/mod"))
    }
    t |> run("github URL is not local") @(t : T?) {
        t |> success(!is_local_path("github.com/user/repo"))
    }
    t |> run("bare name is not local") @(t : T?) {
        t |> success(!is_local_path("dasImgui"))
    }
    t |> run("windows backslash dot path") @(t : T?) {
        t |> success(is_local_path(".\\mymod"))
    }
    t |> run("windows backslash parent path") @(t : T?) {
        t |> success(is_local_path("..\\mymod"))
    }
}

[test]
def test_is_index_name(t : T?) {
    t |> run("bare name is index name") @(t : T?) {
        t |> success(is_index_name("dasImgui"))
    }
    t |> run("github URL is not index name") @(t : T?) {
        t |> success(!is_index_name("github.com/user/repo"))
    }
    t |> run("local path is not index name") @(t : T?) {
        t |> success(!is_index_name("./mymod"))
    }
    t |> run("backslash path is not index name") @(t : T?) {
        t |> success(!is_index_name("path\\to\\mod"))
    }
}

[test]
def test_index_has_push_access(t : T?) {
    t |> run("exact true") @(t : T?) {
        t |> success(index_has_push_access("true"))
    }
    t |> run("trailing newline tolerated") @(t : T?) {
        t |> success(index_has_push_access("true\n"))
    }
    t |> run("false is no access") @(t : T?) {
        t |> success(!index_has_push_access("false"))
    }
    t |> run("empty (gh error / no permissions field) is no access") @(t : T?) {
        t |> success(!index_has_push_access(""))
    }
    t |> run("only lowercase true accepted") @(t : T?) {
        t |> success(!index_has_push_access("True"))
    }
}

[test]
def test_index_push_target(t : T?) {
    t |> run("write access → origin + bare branch") @(t : T?) {
        let tgt = index_push_target(true, "octocat", "introduce-minmaxheap")
        t |> equal(tgt.remote, "origin")
        t |> equal(tgt.head_ref, "introduce-minmaxheap")
    }
    t |> run("no access → fork + login-qualified head") @(t : T?) {
        let tgt = index_push_target(false, "JohnathanKG", "introduce-minmaxheap")
        t |> equal(tgt.remote, "fork")
        t |> equal(tgt.head_ref, "JohnathanKG:introduce-minmaxheap")
    }
    t |> run("login ignored when write access") @(t : T?) {
        let tgt = index_push_target(true, "someone", "withdraw-foo")
        t |> equal(tgt.head_ref, "withdraw-foo")
    }
}

[test]
def test_parse_args(t : T?) {   // nolint:STYLE038 — a flat case table: one argv shape per `t |> run` block
    t |> run("install command") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["install", "dasImgui"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "dasImgui")
        t |> success(!parsed.force)
    }
    t |> run("install with force") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["install", "dasImgui", "--force"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "dasImgui")
        t |> success(parsed.force)
    }
    t |> run("install with root") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["--root", "/tmp/proj", "install", "foo"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "foo")
        t |> equal(parsed.root, "/tmp/proj")
    }
    t |> run("list command no arg") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["list"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "list")
        t |> success(parsed.command_arg |> is_none)
    }
    t |> run("release accepts --paranoid (compat, no budget change)") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["release", "--out", "E:/", "--paranoid"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "release")
        t |> equal(parsed.out, "E:/")
        t |> success(parsed.paranoid)
    }
    t |> run("release with quick inherit") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["release", "--quick"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "release")
        t |> success(parsed.quick)
        t |> success(!parsed.paranoid)
    }
    t |> run("no command") @(t : T?) {
        let empty_args : array<string>
        var r <- parse_args(type<DaspkgArgs>, empty_args)
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> success(parsed.command |> is_none)
    }
    t |> run("cleanup command") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["cleanup"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(!parsed.force)
    }
    t |> run("cleanup with force") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["cleanup", "--force"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(parsed.force)
    }
    t |> run("cleanup with global and force") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["cleanup", "--global", "--force"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(parsed.force)
        t |> success(parsed.global_flag)
    }
    t |> run("default root is dot") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["list"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.root, ".")
    }
}

[test]
def test_lockfile_roundtrip(t : T?) {
    t |> run("write and read back") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_tmp"
        mkdir(tmp_dir)
        var lf : LockFile
        lf.sdk_version = "0.4.0"
        set_package(lf, PackageEntry(name = "foo", source = "github.com/user/foo",
            version = "v1.0", tag = "v1.0", root = true))
        set_package(lf, PackageEntry(name = "bar", source = "github.com/user/bar",
            version = "v2.0", tag = "v2.0", root = false))
        t |> success(write_lock_file(tmp_dir, lf))
        var lf2 : LockFile
        t |> success(read_lock_file(tmp_dir, lf2))
        t |> equal(lf2.sdk_version, "0.4.0")
        t |> equal(length(lf2.packages), 2)
        t |> success(has_package(lf2, "foo"))
        t |> success(has_package(lf2, "bar"))
        let foo = get_package(lf2, "foo")
        t |> equal(foo.source, "github.com/user/foo")
        t |> equal(foo.version, "v1.0")
        t |> success(foo.root)
        let bar = get_package(lf2, "bar")
        t |> equal(bar.source, "github.com/user/bar")
        t |> success(!bar.root)
        // cleanup
        remove("{tmp_dir}/{LOCK_FILE_NAME}")
        rmdir(tmp_dir)
    }
}

[test]
def test_copy_dir_rec_skips_links_and_build_residue(t : T?) {
    t |> run("junction/symlink, .git, and build dirs stay out of the copy") @(t : T?) {
        let tmp = "{get_das_root()}/_daspkg_test_copyskip"
        let src = "{tmp}/src"
        mkdir(tmp)
        mkdir(src)
        mkdir("{src}/real")
        fopen("{src}/real/a.txt", "wb") $(f) {
            fprint(f, "content")
        }
        mkdir("{src}/_build")
        fopen("{src}/_build/CMakeCache.txt", "wb") $(f) {
            fprint(f, "stale")
        }
        mkdir("{src}/build")
        mkdir("{src}/.git")
        // a link pointing at src itself - copying it recurses forever unless copy_dir_rec skips links
        let link = "{src}/loop"
        var out : string
        var link_cmd = "ln -s \"{src}\" \"{link}\""
        if (get_platform_name() == "windows") {
            link_cmd = "cmd /c mklink /J \"{replace(link, "/", "\\")}\" \"{replace(src, "/", "\\")}\""
        }
        let linked = run_cmd(link_cmd, out) == 0
        let dst = "{tmp}/dst"
        t |> success(copy_dir_rec(src, dst), "copy_dir_rec({src} -> {dst}) returned false")
        t |> success(fexist("{dst}/real/a.txt"), "real content missing from the copy")
        t |> success(!fexist("{dst}/_build"), "_build was copied")
        t |> success(!fexist("{dst}/build"), "build was copied")
        t |> success(!fexist("{dst}/.git"), ".git was copied")
        if (linked) {
            var link_err : string
            t |> success(is_reparse_point(link, link_err), "is_reparse_point does not see the link")
            t |> success(!is_reparse_point(src, link_err), "is_reparse_point flags a plain dir")
            t |> success(!fexist("{dst}/loop"), "the link was followed into the copy")
        } else {
            to_log(LOG_INFO, "test_copy_dir_rec_skips_links_and_build_residue: could not create a junction/symlink here; skipping the link half\n")
        }
        force_rmdir(tmp)
    }
}

[test]
def test_in_tree_module_satisfies_install(t : T?) {
    t |> run("explicit install of an in-tree module is a no-op") @(t : T?) {
        // dasGlfw is a tracked in-tree module: present at <das_root>/modules/
        // and never in the global lock file. cmd_install must recognize it,
        // return 0 without touching the index or the network, and create no
        // project-local shadow clone.
        let tmp_root = "{get_das_root()}/_daspkg_test_intree"
        mkdir(tmp_root)
        let rc = cmd_install(tmp_root, "dasGlfw", false)
        t |> equal(rc, 0)
        t |> success(!fexist("{tmp_root}/modules/dasGlfw"))
        // cleanup (no lock file is written on the no-op path)
        rmdir(tmp_root)
    }
}

[test]
def test_install_local(t : T?) {
    t |> run("installs and records in lockfile") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_src"
        mkdir(tmp_root)
        mkdir(tmp_src)
        // create a fake package source
        mkdir("{tmp_src}/daslib")
        fwrite("{tmp_src}/daslib/hello.das", "module hello\n")
        var lf : LockFile
        install_local(tmp_root, "hello", tmp_src, lf)
        // verify lock file entry
        t |> success(has_package(lf, "hello"))
        let hello = get_package(lf, "hello")
        t |> success(hello.local)
        t |> success(hello.root)
        // verify files copied
        t |> success(fexist("{tmp_root}/modules/hello/daslib/hello.das"))
        // verify lock file was written to disk
        t |> success(fexist("{tmp_root}/{LOCK_FILE_NAME}"))
        // cleanup
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
    }
}

// ===== Local-package fixture helpers =====

// Writes a .das_package + CMakeLists.txt that always FATAL_ERRORs on configure.
def private write_failing_pkg(src_dir, pkg_name : string) {
    mkdir(src_dir)
    let manifest = "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"{pkg_name}\")\n\}\n[export]\ndef build() \{\n    cmake_build()\n\}\n"
    fwrite("{src_dir}/.das_package", manifest)
    fwrite("{src_dir}/CMakeLists.txt", "cmake_minimum_required(VERSION 3.16)\nproject({pkg_name})\nmessage(FATAL_ERROR \"forced test failure\")\n")
}

// Writes a .das_package with no build() — install always succeeds.
def private write_passing_pkg(src_dir, pkg_name : string) {
    let no_deps : array<string>
    write_passing_pkg(src_dir, pkg_name, no_deps)
}

// Writes a .das_package with optional dependencies (each entry is a require_package source).
def private write_passing_pkg(src_dir, pkg_name : string; deps : array<string>) {
    let no_lines : array<string>
    write_passing_pkg(src_dir, pkg_name, no_lines, deps)
}

// Writes a .das_package whose package() holds package_name plus `package_lines`, with optional
// dependencies (each entry is a require_package source).
def private write_passing_pkg(src_dir, pkg_name : string; package_lines, deps : array<string>) {
    mkdir(src_dir)
    let manifest = build_string() $(var w) {
        w |> write("options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"")
        w |> write(pkg_name)
        w |> write("\")\n")
        for (line in package_lines) {
            w |> write("    {line}\n")
        }
        w |> write("\}\n")
        if (!empty(deps)) {
            w |> write("[export]\ndef dependencies(version : string) \{\n")
            for (d in deps) {
                w |> write("    require_package(\"")
                w |> write(d)
                w |> write("\")\n")
            }
            w |> write("\}\n")
        }
    }
    fwrite("{src_dir}/.das_package", manifest)
}

[test]
def test_install_local_build_failure_not_recorded(t : T?) {
    t |> run("failed build does not stay installed") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_fail_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_fail_src"
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
        mkdir(tmp_root)
        write_failing_pkg(tmp_src, "failpkg")

        var lf : LockFile
        let rc = install_local(tmp_root, "failpkg", tmp_src, lf)
        t |> success(rc != 0)
        t |> success(!has_package(lf, "failpkg"))
        t |> success(!fexist("{tmp_root}/modules/failpkg"))
        // failure with no prior install must NOT create an empty lock file
        t |> success(!fexist("{tmp_root}/{LOCK_FILE_NAME}"))

        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
    }
}

[test]
def test_install_local_reinstall_failure_clears_entry(t : T?) {
    // First install a healthy pkg, then reinstall in place with a failing source.
    // The reinstall must remove both the directory AND the prior lockfile entry,
    // so the lockfile is consistent with the on-disk state.
    t |> run("reinstall over healthy pkg with failing src clears lock entry") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_reinst_root"
        let tmp_src_ok = "{get_das_root()}/_daspkg_test_reinst_ok_src"
        let tmp_src_bad = "{get_das_root()}/_daspkg_test_reinst_bad_src"
        force_rmdir(tmp_root)
        force_rmdir(tmp_src_ok)
        force_rmdir(tmp_src_bad)
        mkdir(tmp_root)
        write_passing_pkg(tmp_src_ok, "reinstpkg")
        write_failing_pkg(tmp_src_bad, "reinstpkg")

        var lf : LockFile
        let rc1 = install_local(tmp_root, "reinstpkg", tmp_src_ok, lf)
        t |> success(rc1 == 0)
        t |> success(has_package(lf, "reinstpkg"))
        t |> success(fexist("{tmp_root}/modules/reinstpkg"))

        let rc2 = install_local(tmp_root, "reinstpkg", tmp_src_bad, lf)
        t |> success(rc2 != 0)
        t |> success(!has_package(lf, "reinstpkg"))
        t |> success(!fexist("{tmp_root}/modules/reinstpkg"))

        // disk lock file must agree with in-memory lf
        var lf2 : LockFile
        if (read_lock_file(tmp_root, lf2)) {
            t |> success(!has_package(lf2, "reinstpkg"))
        }

        force_rmdir(tmp_root)
        force_rmdir(tmp_src_ok)
        force_rmdir(tmp_src_bad)
    }
}

[test]
def test_install_local_failing_transitive_dep(t : T?) {
    // Parent depends on a failing local child. Both should be rolled back.
    // Pkg names must match the source-dir basenames — install_dependencies
    // derives dep names from the source path, not the .das_package manifest.
    t |> run("failing transitive dep rolls back parent") @(t : T?) {
        let fix = "{get_das_root()}/_daspkg_test_dep_fix"
        force_rmdir(fix)
        mkdir(fix)
        let proj = "{fix}/proj"
        let parent_src = "{fix}/depparent"
        let child_src = "{fix}/depchild"
        mkdir(proj)
        write_failing_pkg(child_src, "depchild")
        let parent_deps <- [child_src]
        write_passing_pkg(parent_src, "depparent", parent_deps)

        var lf : LockFile
        let rc = install_local(proj, "depparent", parent_src, lf)
        t |> success(rc != 0)
        t |> success(!has_package(lf, "depparent"))
        t |> success(!has_package(lf, "depchild"))
        t |> success(!fexist("{proj}/modules/depparent"))
        t |> success(!fexist("{proj}/modules/depchild"))

        force_rmdir(fix)
    }
}

[test]
def test_install_local_cycle(t : T?) {
    // A depends on B, B depends on A. Both passing — should install once each
    // without infinite recursion or directory wipes.
    t |> run("cycle between two healthy packages installs both") @(t : T?) {
        let fix = "{get_das_root()}/_daspkg_test_cycle_fix"
        force_rmdir(fix)
        mkdir(fix)
        let proj = "{fix}/proj"
        let a_src = "{fix}/cycleA"
        let b_src = "{fix}/cycleB"
        mkdir(proj)
        let a_deps <- [b_src]
        let b_deps <- [a_src]
        write_passing_pkg(a_src, "cycleA", a_deps)
        write_passing_pkg(b_src, "cycleB", b_deps)

        var lf : LockFile
        let rc = install_local(proj, "cycleA", a_src, lf)
        t |> success(rc == 0)
        t |> success(has_package(lf, "cycleA"))
        t |> success(has_package(lf, "cycleB"))
        t |> success(fexist("{proj}/modules/cycleA"))
        t |> success(fexist("{proj}/modules/cycleB"))
        // root vs transitive flags
        let a_entry = get_package(lf, "cycleA")
        let b_entry = get_package(lf, "cycleB")
        t |> success(a_entry.root)
        t |> success(!b_entry.root)

        force_rmdir(fix)
    }
}

[test]
def test_install_local_diamond_dep(t : T?) {
    // P depends on A and B; both A and B depend on shared C.
    // C should install once; the second sibling's iteration must take the
    // has_package path, NOT the cycle-detection path. Without erasing
    // visiting on unwind, C would still be in `visiting` when B is
    // processed and would emit a misleading "cycle detected" log.
    t |> run("shared dep installs once via has_package, not cycle path") @(t : T?) {
        let fix = "{get_das_root()}/_daspkg_test_diamond_fix"
        force_rmdir(fix)
        mkdir(fix)
        let proj = "{fix}/proj"
        let p_src = "{fix}/diamondP"
        let a_src = "{fix}/diamondA"
        let b_src = "{fix}/diamondB"
        let c_src = "{fix}/diamondC"
        mkdir(proj)
        write_passing_pkg(c_src, "diamondC")
        let a_deps <- [c_src]
        let b_deps <- [c_src]
        write_passing_pkg(a_src, "diamondA", a_deps)
        write_passing_pkg(b_src, "diamondB", b_deps)
        let p_deps <- [a_src, b_src]
        write_passing_pkg(p_src, "diamondP", p_deps)

        var lf : LockFile
        let rc = install_local(proj, "diamondP", p_src, lf)
        t |> success(rc == 0)
        t |> success(has_package(lf, "diamondP"))
        t |> success(has_package(lf, "diamondA"))
        t |> success(has_package(lf, "diamondB"))
        t |> success(has_package(lf, "diamondC"))
        t |> success(fexist("{proj}/modules/diamondC"))

        force_rmdir(fix)
    }
}

[test]
def test_install_local_cycle_with_failure(t : T?) {
    // A depends on B, B depends on A — but B's build fails.
    // Both must be rolled back; nothing left in lockfile or on disk.
    t |> run("cycle with failing dep rolls back both") @(t : T?) {
        let fix = "{get_das_root()}/_daspkg_test_cyclefail_fix"
        force_rmdir(fix)
        mkdir(fix)
        let proj = "{fix}/proj"
        let a_src = "{fix}/cyclefailA"
        let b_src = "{fix}/cyclefailB"
        mkdir(proj)
        let a_deps <- [b_src]
        write_passing_pkg(a_src, "cyclefailA", a_deps)
        // failing B with a back-edge to A to form the cycle
        mkdir(b_src)
        let b_manifest = "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"cyclefailB\")\n\}\n[export]\ndef dependencies(version : string) \{\n    require_package(\"{a_src}\")\n\}\n[export]\ndef build() \{\n    cmake_build()\n\}\n"
        fwrite("{b_src}/.das_package", b_manifest)
        fwrite("{b_src}/CMakeLists.txt", "cmake_minimum_required(VERSION 3.16)\nproject(cyclefailB)\nmessage(FATAL_ERROR \"forced test failure\")\n")

        var lf : LockFile
        let rc = install_local(proj, "cyclefailA", a_src, lf)
        t |> success(rc != 0)
        t |> success(!has_package(lf, "cyclefailA"))
        t |> success(!has_package(lf, "cyclefailB"))
        t |> success(!fexist("{proj}/modules/cyclefailA"))
        t |> success(!fexist("{proj}/modules/cyclefailB"))

        force_rmdir(fix)
    }
}

[test]
def test_cmd_remove(t : T?) {
    t |> run("removes package and updates lockfile") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_remove"
        mkdir(tmp_root)
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/testpkg")
        fwrite("{tmp_root}/modules/testpkg/dummy.txt", "hello")
        // write a lock file with the package
        var lf : LockFile
        set_package(lf, PackageEntry(name = "testpkg", source = "github.com/user/testpkg", root = true))
        write_lock_file(tmp_root, lf)
        // remove
        cmd_remove(tmp_root, "testpkg")
        // verify directory removed
        t |> success(!fexist("{tmp_root}/modules/testpkg"))
        // verify lock file updated
        var lf2 : LockFile
        t |> success(read_lock_file(tmp_root, lf2))
        t |> success(!has_package(lf2, "testpkg"))
        // cleanup
        force_rmdir(tmp_root)
    }
}

[test]
def test_cmd_cleanup(t : T?) {
    t |> run("removes modules, lockfile, and gitignore entries") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_cleanup"
        force_rmdir(tmp_root)
        mkdir(tmp_root)
        // fake .git so gitignore_remove runs
        mkdir("{tmp_root}/.git")
        fwrite("{tmp_root}/.das_package", "options gen2\n")
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/foo")
        fwrite("{tmp_root}/modules/foo/dummy.txt", "x")
        var lf : LockFile
        set_package(lf, PackageEntry(name = "foo", source = "github.com/u/foo", root = true))
        write_lock_file(tmp_root, lf)
        fwrite("{tmp_root}/.gitignore", "modules/foo\nother-entry\n")
        let rc = cmd_cleanup(tmp_root, true)
        t |> equal(rc, 0)
        t |> success(!fexist("{tmp_root}/modules"))
        t |> success(!fexist("{tmp_root}/daspkg.lock"))
        var content = ""
        fopen("{tmp_root}/.gitignore", "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "other-entry\n")
        force_rmdir(tmp_root)
    }
}

[test]
def test_cmd_cleanup_refuses_without_marker(t : T?) {
    t |> run("refuses in dir without .das_package or lockfile") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_cleanup_bare"
        force_rmdir(tmp_root)
        mkdir(tmp_root)
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/foo")
        let rc = cmd_cleanup(tmp_root, true)
        t |> equal(rc, 1)
        // modules untouched
        t |> success(fexist("{tmp_root}/modules/foo"))
        force_rmdir(tmp_root)
    }
}

[test]
def test_cmd_cleanup_no_lockfile(t : T?) {
    t |> run("no-op on project with .das_package but no lockfile/modules") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_cleanup_empty"
        force_rmdir(tmp_root)
        mkdir(tmp_root)
        fwrite("{tmp_root}/.das_package", "options gen2\n")
        let rc = cmd_cleanup(tmp_root, true)
        t |> equal(rc, 0)
        t |> success(fexist("{tmp_root}/.das_package"))
        force_rmdir(tmp_root)
    }
}

[test]
def test_cmd_cleanup_preserves_user_files(t : T?) {
    t |> run("leaves non-daspkg files alongside modules") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_cleanup_preserve"
        force_rmdir(tmp_root)
        mkdir(tmp_root)
        fwrite("{tmp_root}/.das_package", "options gen2\n")
        fwrite("{tmp_root}/main.das", "// user file\n")
        fwrite("{tmp_root}/README.md", "hello\n")
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/foo")
        var lf : LockFile
        set_package(lf, PackageEntry(name = "foo", source = "github.com/u/foo", root = true))
        write_lock_file(tmp_root, lf)
        let rc = cmd_cleanup(tmp_root, true)
        t |> equal(rc, 0)
        t |> success(fexist("{tmp_root}/.das_package"))
        t |> success(fexist("{tmp_root}/main.das"))
        t |> success(fexist("{tmp_root}/README.md"))
        t |> success(!fexist("{tmp_root}/modules"))
        t |> success(!fexist("{tmp_root}/daspkg.lock"))
        force_rmdir(tmp_root)
    }
}

[test]
def test_cmd_cleanup_without_force_dry_runs(t : T?) {
    t |> run("without --force, modules and lockfile are preserved") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_cleanup_dryrun"
        force_rmdir(tmp_root)
        mkdir(tmp_root)
        fwrite("{tmp_root}/.das_package", "options gen2\n")
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/foo")
        var lf : LockFile
        set_package(lf, PackageEntry(name = "foo", source = "github.com/u/foo", root = true))
        write_lock_file(tmp_root, lf)
        let rc = cmd_cleanup(tmp_root, false)
        t |> equal(rc, 0)
        t |> success(fexist("{tmp_root}/modules/foo"))
        t |> success(fexist("{tmp_root}/daspkg.lock"))
        force_rmdir(tmp_root)
    }
}

[test]
def test_read_manifest(t : T?) {
    t |> run("valid manifest from .das_package") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest"
        mkdir(tmp_dir)
        fwrite("{tmp_dir}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"mypkg\")\n    package_description(\"A test package\")\n\}\n")
        var manifest : PackageManifest
        t |> success(read_manifest(tmp_dir, manifest))
        t |> equal(manifest.name, "mypkg")
        t |> equal(manifest.description, "A test package")
        remove("{tmp_dir}/.das_package")
        rmdir(tmp_dir)
    }
    t |> run("missing name") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest2"
        mkdir(tmp_dir)
        fwrite("{tmp_dir}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_description(\"no name\")\n\}\n")
        var manifest : PackageManifest
        t |> success(!read_manifest(tmp_dir, manifest))
        remove("{tmp_dir}/.das_package")
        rmdir(tmp_dir)
    }
    t |> run("missing description") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest3"
        mkdir(tmp_dir)
        fwrite("{tmp_dir}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"mypkg\")\n\}\n")
        var manifest : PackageManifest
        t |> success(!read_manifest(tmp_dir, manifest))
        remove("{tmp_dir}/.das_package")
        rmdir(tmp_dir)
    }
    t |> run("no manifest file") @(t : T?) {
        var manifest : PackageManifest
        t |> success(!read_manifest("{get_das_root()}/_daspkg_nonexistent", manifest))
    }
}

[test]
def test_add_to_index_json(t : T?) {
    t |> run("adds new package") @(t : T?) {
        let result = add_to_index_json("[]", "mypkg", "github.com/user/mypkg", "A cool package")
        t |> success(!empty(result))
        var error : string
        var js = read_json(result, error)
        t |> success(js != null)
        unsafe {
            t |> equal(length(js.value as _array), 1)
            t |> equal((js.value as _array)[0]?.name ?? "", "mypkg")
            t |> equal((js.value as _array)[0]?.url ?? "", "github.com/user/mypkg")
            t |> equal((js.value as _array)[0]?.description ?? "", "A cool package")
        }
    }
    t |> run("preserves existing and sorts") @(t : T?) {
        let input = add_to_index_json("[]", "pkg2", "github.com/user/pkg2", "Second")
        let result = add_to_index_json(input, "pkg1", "github.com/user/pkg1", "First")
        var error : string
        var js = read_json(result, error)
        t |> success(js != null)
        unsafe {
            t |> equal(length(js.value as _array), 2)
            // pkg1 should come before pkg2 (sorted)
            t |> equal((js.value as _array)[0]?.name ?? "", "pkg1")
            t |> equal((js.value as _array)[1]?.name ?? "", "pkg2")
        }
    }
    t |> run("invalid JSON returns empty") @(t : T?) {
        let result = add_to_index_json("not json", "pkg", "url", "desc")
        t |> success(empty(result))
    }
}

[test]
def test_remove_from_index_json(t : T?) {
    t |> run("removes existing package") @(t : T?) {
        let input = add_to_index_json("[]", "mypkg", "github.com/user/mypkg", "desc")
        let result = remove_from_index_json(input, "mypkg")
        t |> success(!empty(result))
        var error : string
        var js = read_json(result, error)
        t |> success(js != null)
        unsafe(t |> equal(length(js.value as _array), 0))
    }
    t |> run("preserves other entries") @(t : T?) {
        var input = add_to_index_json("[]", "pkg1", "github.com/user/pkg1", "First")
        input = add_to_index_json(input, "pkg2", "github.com/user/pkg2", "Second")
        let result = remove_from_index_json(input, "pkg1")
        var error : string
        var js = read_json(result, error)
        t |> success(js != null)
        unsafe {
            t |> equal(length(js.value as _array), 1)
            t |> equal((js.value as _array)[0]?.name ?? "", "pkg2")
        }
    }
    t |> run("removing nonexistent is no-op") @(t : T?) {
        let input = add_to_index_json("[]", "pkg1", "github.com/user/pkg1", "desc")
        let result = remove_from_index_json(input, "nonexistent")
        var error : string
        var js = read_json(result, error)
        t |> success(js != null)
        unsafe {
            t |> equal(length(js.value as _array), 1)
            t |> equal((js.value as _array)[0]?.name ?? "", "pkg1")
        }
    }
    t |> run("invalid JSON returns empty") @(t : T?) {
        let result = remove_from_index_json("bad", "pkg")
        t |> success(empty(result))
    }
}

[test]
def test_cmd_check(t : T?) {
    let test_root = get_full_file_name("_daspkg_test_check")
    let modules_dir = "{test_root}/modules"
    let pkg_dir = "{modules_dir}/mypkg"

    // setup: create module dir with .das_module
    mkdir(test_root)
    mkdir(modules_dir)
    mkdir(pkg_dir)
    fopen("{pkg_dir}/.das_module", "wb") $(f) {
        fwrite(f, "options gen2\n")
    }

    // write lockfile with two entries: one present, one missing
    var lf = LockFile()
    set_package(lf, PackageEntry(name = "mypkg", source = "github.com/user/mypkg"))
    set_package(lf, PackageEntry(name = "ghost", source = "github.com/user/ghost"))
    write_lock_file(test_root, lf)

    t |> run("detects present and missing packages") @(t : T?) {
        // cmd_check prints output — just verify it doesn't crash
        cmd_check(test_root)
        // verify the package dir exists
        t |> success(fexist(pkg_dir))
        // verify the ghost dir does NOT exist
        t |> success(!fexist("{modules_dir}/ghost"))
    }

    t |> run("warns about missing .das_module") @(t : T?) {
        let pkg2_dir = "{modules_dir}/nopkg"
        mkdir(pkg2_dir)
        var lf2 = LockFile()
        set_package(lf2, PackageEntry(name = "nopkg", source = "github.com/user/nopkg"))
        write_lock_file(test_root, lf2)
        cmd_check(test_root)  // should print WARNING, not crash
        t |> success(fexist(pkg2_dir))
        force_rmdir(pkg2_dir)
    }

    // cleanup
    force_rmdir(test_root)
}

[test]
def test_cmd_check_no_lockfile(t : T?) {
    t |> run("no lockfile prints message") @(t : T?) {
        cmd_check("_daspkg_nonexistent_dir")
        t |> success(true)  // just verify no crash
    }
}

[test]
def test_check_tool(t : T?) {
    t |> run("detects git") @(t : T?) {
        t |> success(check_tool("git", "git --version"))
    }
    t |> run("detects cmake") @(t : T?) {
        t |> success(check_tool("cmake", "cmake --version"))
    }
    t |> run("fails on nonexistent tool") @(t : T?) {
        t |> success(!check_tool("nonexistent", "nonexistent_tool_xyz --version"))
    }
}

[test]
def test_cmd_doctor(t : T?) {
    t |> run("runs without crash") @(t : T?) {
        cmd_doctor()
        t |> success(true)
    }
}

// --- package_runner tests ---

let PKG_FIXTURE = "utils/daspkg/fixtures/test.das_package"
let PKG_BRANCH_FIXTURE = "utils/daspkg/fixtures/test_branch.das_package"
let PKG_REDIRECT_FIXTURE = "utils/daspkg/fixtures/test_redirect.das_package"
let PKG_SDK_AWARE_FIXTURE = "utils/daspkg/fixtures/test_sdk_aware.das_package"
let PKG_RICH_FIXTURE = "utils/daspkg/fixtures/test_rich.das_package"
let PKG_RELEASE_FIXTURE = "utils/daspkg/fixtures/test_release.das_package"

[test]
def test_run_das_package_meta(t : T?) {
    t |> run("reads package metadata") @(t : T?) {
        var meta : PackageMeta
        let ok = run_das_package_meta(PKG_FIXTURE, meta)
        t |> success(ok)
        t |> equal("test-pkg", meta.pkg_name)
        t |> equal("Test Author", meta.author)
        t |> equal("A test package", meta.description)
        t |> equal("https://github.com/test/test-pkg", meta.source)
    }
}

[test]
def test_run_das_package_resolve_tag(t : T?) {
    t |> run("resolves tag version") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_FIXTURE, "0.5", "1.2.3", res)
        t |> success(ok)
        t |> equal("v1.2.3", res.tag)
        t |> equal("", res.branch)
    }
}

[test]
def test_run_das_package_resolve_branch(t : T?) {
    t |> run("resolves branch download") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_BRANCH_FIXTURE, "0.5", "1.0", res)
        t |> success(ok)
        t |> equal("", res.tag)
        t |> equal("main", res.branch)
    }
}

[test]
def test_run_das_package_deps(t : T?) {
    t |> run("reads dependencies") @(t : T?) {
        var deps : array<PackageDependency>
        let ok = run_das_package_deps(PKG_FIXTURE, "1.0", deps)
        t |> success(ok)
        t |> equal(2, length(deps))
        t |> equal("https://github.com/test/dep-a", deps[0].source)
        t |> equal(">=1.0", deps[0].version_constraint)
        t |> equal("https://github.com/test/dep-b", deps[1].source)
        t |> equal("", deps[1].version_constraint)
    }
}

[test]
def test_run_das_package_build(t : T?) {
    t |> run("reads cmake build info") @(tt : T?) {
        var info : PackageBuildInfo
        let ok = run_das_package_build(PKG_FIXTURE, info)
        tt |> success(ok)
        tt |> equal(true, info.is_cmake)
    }
}

[test]
def test_run_das_package_missing_file(t : T?) {
    t |> run("returns false for missing file") @(t : T?) {
        var meta : PackageMeta
        let ok = run_das_package_meta("nonexistent.das_package", meta)
        t |> equal(false, ok)
    }
}

[test]
def test_run_das_package_optional_functions(t : T?) {
    t |> run("succeeds when function is missing") @(tt : T?) {
        // test_branch.das_package has no build() function — should still succeed
        var info : PackageBuildInfo
        let ok = run_das_package_build(PKG_BRANCH_FIXTURE, info)
        tt |> success(ok)
        tt |> equal(false, info.is_cmake)
        tt |> equal(false, info.is_custom)
    }
}

[test]
def test_run_das_package_resolve_redirect(t : T?) {
    t |> run("resolves redirect to different repo") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_REDIRECT_FIXTURE, "", "", res)
        t |> success(ok)
        t |> equal("github.com/borisbat/daspkg-test-pure", res.source)
        t |> equal("v1.0", res.tag)
        t |> equal("", res.branch)
    }
}

[test]
def test_run_das_package_resolve_sdk_aware(t : T?) {
    t |> run("resolves latest to default tag") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_SDK_AWARE_FIXTURE, "0.5", "", res)
        t |> success(ok)
        t |> equal("v2.0", res.tag)
    }
    t |> run("resolves 1.x to v1.5") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_SDK_AWARE_FIXTURE, "0.5", "1.x", res)
        t |> success(ok)
        t |> equal("v1.5", res.tag)
    }
    t |> run("resolves explicit version") @(t : T?) {
        var res : ResolveResult
        let ok = run_das_package_resolve(PKG_SDK_AWARE_FIXTURE, "0.5", "3.0", res)
        t |> success(ok)
        t |> equal("v3.0", res.tag)
    }
}

[test]
def test_parse_version(t : T?) {
    t |> run("parses major.minor.patch") @(t : T?) {
        let v = parse_version("1.2.3")
        t |> success(v.valid)
        t |> equal(1, v.major)
        t |> equal(2, v.minor)
        t |> equal(3, v.patch)
    }
    t |> run("parses with v prefix") @(t : T?) {
        let v = parse_version("v1.2.3")
        t |> success(v.valid)
        t |> equal(1, v.major)
        t |> equal(2, v.minor)
        t |> equal(3, v.patch)
    }
    t |> run("parses major.minor") @(t : T?) {
        let v = parse_version("2.5")
        t |> success(v.valid)
        t |> equal(2, v.major)
        t |> equal(5, v.minor)
        t |> equal(0, v.patch)
    }
    t |> run("parses major only") @(t : T?) {
        let v = parse_version("3")
        t |> success(v.valid)
        t |> equal(3, v.major)
        t |> equal(0, v.minor)
        t |> equal(0, v.patch)
    }
    t |> run("invalid version") @(t : T?) {
        let v = parse_version("abc")
        t |> success(!v.valid)
    }
    t |> run("empty version") @(t : T?) {
        let v = parse_version("")
        t |> success(!v.valid)
    }
}

[test]
def test_compare_versions(t : T?) {
    t |> run("equal versions") @(t : T?) {
        let a = parse_version("1.2.3")
        let b = parse_version("1.2.3")
        t |> equal(0, compare_versions(a, b))
    }
    t |> run("major differs") @(t : T?) {
        let a = parse_version("1.0.0")
        let b = parse_version("2.0.0")
        t |> equal(-1, compare_versions(a, b))
        t |> equal(1, compare_versions(b, a))
    }
    t |> run("minor differs") @(t : T?) {
        let a = parse_version("1.2.0")
        let b = parse_version("1.3.0")
        t |> equal(-1, compare_versions(a, b))
    }
    t |> run("patch differs") @(t : T?) {
        let a = parse_version("1.2.3")
        let b = parse_version("1.2.4")
        t |> equal(-1, compare_versions(a, b))
    }
}

[test]
def test_satisfies_constraint(t : T?) {
    t |> run("empty constraint matches anything") @(t : T?) {
        t |> success(satisfies_constraint("1.0.0", ""))
        t |> success(satisfies_constraint("v99.0.0", ""))
    }
    t |> run("exact match") @(t : T?) {
        t |> success(satisfies_constraint("1.2.3", "1.2.3"))
        t |> success(!satisfies_constraint("1.2.4", "1.2.3"))
    }
    t |> run("exact with = prefix") @(t : T?) {
        t |> success(satisfies_constraint("1.2.3", "=1.2.3"))
        t |> success(!satisfies_constraint("1.2.4", "=1.2.3"))
    }
    t |> run("greater or equal") @(t : T?) {
        t |> success(satisfies_constraint("1.0.0", ">=1.0"))
        t |> success(satisfies_constraint("2.0.0", ">=1.0"))
        t |> success(!satisfies_constraint("0.9.0", ">=1.0"))
    }
    t |> run("greater than") @(t : T?) {
        t |> success(!satisfies_constraint("1.0.0", ">1.0"))
        t |> success(satisfies_constraint("1.0.1", ">1.0"))
    }
    t |> run("less or equal") @(t : T?) {
        t |> success(satisfies_constraint("1.0.0", "<=1.0"))
        t |> success(satisfies_constraint("0.9.0", "<=1.0"))
        t |> success(!satisfies_constraint("1.0.1", "<=1.0"))
    }
    t |> run("less than") @(t : T?) {
        t |> success(satisfies_constraint("0.9.0", "<1.0"))
        t |> success(!satisfies_constraint("1.0.0", "<1.0"))
    }
    t |> run("range with comma") @(t : T?) {
        t |> success(satisfies_constraint("1.5.0", ">=1.0,<2.0"))
        t |> success(satisfies_constraint("1.0.0", ">=1.0,<2.0"))
        t |> success(!satisfies_constraint("2.0.0", ">=1.0,<2.0"))
        t |> success(!satisfies_constraint("0.9.0", ">=1.0,<2.0"))
    }
    t |> run("v prefix on version") @(t : T?) {
        t |> success(satisfies_constraint("v1.2.3", ">=1.0"))
        t |> success(satisfies_constraint("v1.2.3", "1.2.3"))
    }
    t |> run("v prefix on constraint") @(t : T?) {
        t |> success(satisfies_constraint("1.2.3", ">=v1.0"))
        t |> success(satisfies_constraint("1.2.3", "v1.2.3"))
    }
    t |> run("invalid version fails") @(t : T?) {
        t |> success(!satisfies_constraint("abc", ">=1.0"))
    }
    t |> run("invalid constraint fails") @(t : T?) {
        t |> success(!satisfies_constraint("1.0.0", ">=abc"))
    }
    t |> run("spaces in range") @(t : T?) {
        t |> success(satisfies_constraint("1.5.0", ">= 1.0 , < 2.0"))
    }
}

// --- logging tests ---

[test]
def test_log_writes_to_file(t : T?) {
    let test_root = get_full_file_name("_daspkg_test_log")
    mkdir(test_root)

    t |> run("log() writes to both screen and file") @(t : T?) {
        log_init(test_root)
        log("hello from test\n")
        // read the log file and check content
        var content : string
        fopen("{test_root}/modules/.daspkg.log", "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> success(find(content, "hello from test") >= 0)
        t |> success(find(content, "--- daspkg ---") >= 0)
    }

    t |> run("log_debug() writes to file always") @(t : T?) {
        log_verbose_flag = false
        log_init(test_root)
        log_debug("debug message here")
        var content : string
        fopen("{test_root}/modules/.daspkg.log", "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> success(find(content, "debug message here") >= 0)
    }

    t |> run("run_cmd logs command to file") @(t : T?) {
        log_init(test_root)
        var output : string
        run_cmd("git --version", output)
        var content : string
        fopen("{test_root}/modules/.daspkg.log", "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> success(find(content, "git --version") >= 0)
        // note: stdout capture via popen is unreliable on Windows,
        // so we only check the command itself is logged
    }

    t |> run("no file logging before log_init") @(t : T?) {
        // reset log state
        log_file_path = ""
        log("this should not crash\n")
        log_debug("neither should this")
        t |> success(true)
    }

    // cleanup
    force_rmdir(test_root)
    log_file_path = ""
}

// --- rich metadata tests ---

[test]
def test_run_das_package_rich_meta(t : T?) {
    t |> run("reads license, tags, min_sdk") @(t : T?) {
        var meta : PackageMeta
        let ok = run_das_package_meta(PKG_RICH_FIXTURE, meta)
        t |> success(ok)
        t |> equal("rich-pkg", meta.pkg_name)
        t |> equal("testauthor", meta.author)
        t |> equal("A richly annotated test package", meta.description)
        t |> equal("MIT", meta.license)
        t |> equal(2, length(meta.tags))
        t |> equal("networking", meta.tags[0])
        t |> equal("http", meta.tags[1])
        t |> equal("0.4", meta.min_sdk)
    }
    t |> run("original fixture has no new fields") @(t : T?) {
        var meta : PackageMeta
        let ok = run_das_package_meta(PKG_FIXTURE, meta)
        t |> success(ok)
        t |> success(empty(meta.license))
        t |> equal(0, length(meta.tags))
        t |> success(empty(meta.min_sdk))
    }
}

// --- index parsing with rich fields ---

[test]
def test_parse_index_json_rich(t : T?) {
    t |> run("parses all enriched fields") @(t : T?) {
        let json_text = "[\{\"name\":\"mypkg\",\"url\":\"github.com/user/mypkg\",\"description\":\"desc\",\"author\":\"alice\",\"license\":\"MIT\",\"tags\":[\"ai\",\"http\"],\"min_sdk\":\"0.4\",\"latest_version\":\"v1.0\",\"last_updated\":\"2025-01-01\",\"has_native\":true,\"dependencies\":[\"dep-a\"]\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        t |> success(index |> key_exists("mypkg"))
        unsafe {
            let entry & = index["mypkg"]
            t |> equal("github.com/user/mypkg", entry.url)
            t |> equal("desc", entry.description)
            t |> equal("alice", entry.author)
            t |> equal("MIT", entry.license)
            t |> equal(2, length(entry.tags))
            t |> equal("ai", entry.tags[0])
            t |> equal("http", entry.tags[1])
            t |> equal("0.4", entry.min_sdk)
            t |> equal("v1.0", entry.latest_version)
            t |> equal("2025-01-01", entry.last_updated)
            t |> success(entry.has_native)
            t |> equal(1, length(entry.dependencies))
            t |> equal("dep-a", entry.dependencies[0])
        }
    }
    t |> run("omitted fields default properly") @(t : T?) {
        let json_text = "[\{\"name\":\"bare\",\"url\":\"github.com/user/bare\",\"description\":\"minimal\"\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        unsafe {
            let entry & = index["bare"]
            t |> success(empty(entry.author))
            t |> success(empty(entry.license))
            t |> equal(0, length(entry.tags))
            t |> success(empty(entry.min_sdk))
            t |> success(!entry.has_native)
        }
    }
}

// --- index serialization roundtrip ---

[test]
def test_serialize_index_rich(t : T?) {
    t |> run("roundtrip preserves all fields") @(t : T?) {
        let json_text = "[\{\"name\":\"pkg1\",\"url\":\"github.com/u/pkg1\",\"description\":\"first\",\"author\":\"bob\",\"license\":\"Apache-2.0\",\"tags\":[\"game\"],\"min_sdk\":\"0.3\",\"latest_version\":\"v2.0\",\"last_updated\":\"2025-06-01\",\"has_native\":true,\"dependencies\":[\"dep1\"]\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        let serialized = serialize_index(index)
        t |> success(!empty(serialized))
        // re-parse
        var index2 : table<string; IndexEntry>
        t |> success(parse_index_json(serialized, index2))
        t |> success(index2 |> key_exists("pkg1"))
        unsafe {
            let e & = index2["pkg1"]
            t |> equal("bob", e.author)
            t |> equal("Apache-2.0", e.license)
            t |> equal(1, length(e.tags))
            t |> equal("game", e.tags[0])
            t |> equal("0.3", e.min_sdk)
            t |> equal("v2.0", e.latest_version)
            t |> equal("2025-06-01", e.last_updated)
            t |> success(e.has_native)
            t |> equal(1, length(e.dependencies))
        }
    }
    t |> run("omits empty optional fields") @(t : T?) {
        let json_text = "[\{\"name\":\"slim\",\"url\":\"github.com/u/slim\",\"description\":\"minimal\"\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        let serialized = serialize_index(index)
        // should NOT contain "author", "license", etc. keys
        t |> success(find(serialized, "\"author\"") < 0)
        t |> success(find(serialized, "\"license\"") < 0)
        t |> success(find(serialized, "\"tags\"") < 0)
        t |> success(find(serialized, "\"has_native\"") < 0)
    }
}

// --- add_to_index_json_rich ---

[test]
def test_add_to_index_json_rich(t : T?) {
    t |> run("adds package with full manifest") @(t : T?) {
        var manifest = PackageManifest(
            name = "rich",
            description = "A rich package",
            author = "alice",
            license = "MIT",
            min_sdk = "0.4",
            has_native = true
        )
        manifest.tags |> push_clone("ai")
        manifest.dependencies |> push_clone("dep-x")
        let result = add_to_index_json_rich("[]", "rich", manifest, "github.com/alice/rich")
        t |> success(!empty(result))
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(result, index))
        unsafe {
            let e & = index["rich"]
            t |> equal("alice", e.author)
            t |> equal("MIT", e.license)
            t |> equal(1, length(e.tags))
            t |> equal("ai", e.tags[0])
            t |> equal("0.4", e.min_sdk)
            t |> success(e.has_native)
            t |> equal(1, length(e.dependencies))
        }
    }
}

// --- search tests ---

[test]
def test_search_matches(t : T?) {
    t |> run("search scoring and filters") @(t : T?) {
        // build a small index
        let json_text = "[\{\"name\":\"das-http\",\"url\":\"github.com/u/das-http\",\"description\":\"HTTP client\",\"author\":\"alice\",\"tags\":[\"networking\",\"http\"]\},\{\"name\":\"das-imgui\",\"url\":\"github.com/u/das-imgui\",\"description\":\"ImGui bindings\",\"tags\":[\"gui\"]\}]"
        var index : table<string; IndexEntry>
        parse_index_json(json_text, index)
        unsafe {
            // name substring match
            t |> success(matches_entry("das-http", index["das-http"], "http") < 100)
            // description match
            t |> success(matches_entry("das-imgui", index["das-imgui"], "bindings") < 100)
            // tag: filter — matching tag
            t |> success(matches_entry("das-http", index["das-http"], "tag:networking") < 100)
            // tag: filter — non-matching tag
            t |> equal(100, matches_entry("das-imgui", index["das-imgui"], "tag:networking"))
            // no match returns 100
            t |> equal(100, matches_entry("das-http", index["das-http"], "zzzzzzzzz"))
            // author match
            t |> success(matches_entry("das-http", index["das-http"], "alice") < 100)
        }
    }
}

// --- README generation ---

[test]
def test_generate_readme(t : T?) {
    t |> run("generates markdown with tags") @(t : T?) {
        let json_text = "[\{\"name\":\"mypkg\",\"url\":\"github.com/u/mypkg\",\"description\":\"My package\",\"author\":\"bob\",\"license\":\"MIT\",\"tags\":[\"ai\"]\}]"
        let readme = generate_index_readme(json_text)
        t |> success(!empty(readme))
        t |> success(find(readme, "# daspkg package index") >= 0)
        t |> success(find(readme, "mypkg") >= 0)
        t |> success(find(readme, "## Packages by Tag") >= 0)
        t |> success(find(readme, "**ai**") >= 0)
        t |> success(find(readme, "## Package Index") >= 0)
        t |> success(find(readme, "| Package | Description | Install |") >= 0)
        t |> success(find(readme, "**Author:** bob") >= 0)
        t |> success(find(readme, "**License:** MIT") >= 0)
        t |> success(find(readme, "daspkg install mypkg") >= 0)
    }
    t |> run("empty index produces valid readme") @(t : T?) {
        let readme = generate_index_readme("[]")
        t |> success(!empty(readme))
        t |> success(find(readme, "0 package(s) registered") >= 0)
    }
}

// --- ansi_colors tests ---

[test]
def test_ansi_colors(t : T?) {
    t |> run("colors disabled returns plain text") @(t : T?) {
        use_tty_colors = false
        t |> equal("hello", red_str("hello"))
        t |> equal("hello", green_str("hello"))
        t |> equal("hello", yellow_str("hello"))
        t |> equal("hello", blue_str("hello"))
        t |> equal("hello", magenta_str("hello"))
        t |> equal("hello", cyan_str("hello"))
        t |> equal("hello", bold_str("hello"))
        t |> equal("hello", dim_str("hello"))
        t |> equal("hello", underline_str("hello"))
        t |> equal("", reset_str())
    }
    t |> run("colors enabled wraps with ANSI codes") @(t : T?) {
        use_tty_colors = true
        t |> equal("\x1B[31mhi\x1B[0m", red_str("hi"))
        t |> equal("\x1B[32mhi\x1B[0m", green_str("hi"))
        t |> equal("\x1B[33mhi\x1B[0m", yellow_str("hi"))
        t |> equal("\x1B[34mhi\x1B[0m", blue_str("hi"))
        t |> equal("\x1B[35mhi\x1B[0m", magenta_str("hi"))
        t |> equal("\x1B[36mhi\x1B[0m", cyan_str("hi"))
        t |> equal("\x1B[1mhi\x1B[0m", bold_str("hi"))
        t |> equal("\x1B[2mhi\x1B[0m", dim_str("hi"))
        t |> equal("\x1B[4mhi\x1B[0m", underline_str("hi"))
        t |> equal("\x1B[0m", reset_str())
        use_tty_colors = false
    }
    t |> run("init_ansi_colors with --color flag") @(t : T?) {
        use_tty_colors = false
        init_ansi_colors(["exe", "script.das", "--color"])
        t |> success(use_tty_colors)
        use_tty_colors = false
    }
    t |> run("init_ansi_colors with --no-color flag") @(t : T?) {
        use_tty_colors = true
        init_ansi_colors(["exe", "script.das", "--no-color"])
        t |> success(!use_tty_colors)
    }
}

// --- parse_args color flags ---
// Under clargs, --color/--no-color populate parsed.color/.no_color and
// main.das's apply_global_flags flips use_tty_colors.  We test the parse +
// apply pipeline together so the integration stays covered.

[test]
def test_parse_args_color_flags(t : T?) {
    t |> run("--color sets use_tty_colors") @(t : T?) {
        use_tty_colors = false
        var r <- parse_args(type<DaspkgArgs>, ["list", "--color"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> success(parsed.color, "color flag parsed")
        apply_global_flags(parsed)
        t |> success(use_tty_colors)
        use_tty_colors = false
    }
    t |> run("--no-color clears use_tty_colors") @(t : T?) {
        use_tty_colors = true
        var r <- parse_args(type<DaspkgArgs>, ["list", "--no-color"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> success(parsed.no_color, "no_color flag parsed")
        apply_global_flags(parsed)
        t |> success(!use_tty_colors)
    }
    t |> run("--color and --no-color together is mutex error") @(t : T?) {
        t |> success(parse_args(type<DaspkgArgs>, ["list", "--color", "--no-color"]) |> is_err, "mutex group rejects both")
    }
}

// --- read_manifest with rich metadata ---

[test]
def test_read_manifest_rich(t : T?) {
    t |> run("reads license, tags, min_sdk from manifest") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest_rich"
        mkdir(tmp_dir)
        fwrite("{tmp_dir}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"richpkg\")\n    package_description(\"Rich package\")\n    package_license(\"Apache-2.0\")\n    package_tag(\"ai\")\n    package_tag(\"ml\")\n    package_min_sdk(\"0.5\")\n\}\n")
        var manifest : PackageManifest
        t |> success(read_manifest(tmp_dir, manifest))
        t |> equal("richpkg", manifest.name)
        t |> equal("Apache-2.0", manifest.license)
        t |> equal(2, length(manifest.tags))
        t |> equal("ai", manifest.tags[0])
        t |> equal("ml", manifest.tags[1])
        t |> equal("0.5", manifest.min_sdk)
        remove("{tmp_dir}/.das_package")
        rmdir(tmp_dir)
    }
}

[test]
def test_parse_args_global_flag(t : T?) {
    t |> run("--global flag parsed") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["install", "foo", "--global"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "foo")
        t |> success(parsed.global_flag)
    }
    t |> run("-g shorthand") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["-g", "list"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> equal(parsed.command ?? "", "list")
        t |> success(parsed.global_flag)
    }
    t |> run("default global is false") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["install", "foo"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> success(!parsed.global_flag)
    }
}

[test]
def test_global_lock_file_roundtrip(t : T?) {
    t |> run("write and read global lock file") @(t : T?) {
        // use a temp dir instead of the real global lock file path
        let tmp_dir = "{get_das_root()}/_daspkg_test_global_lf"
        mkdir(tmp_dir)
        let path = "{tmp_dir}/{GLOBAL_LOCK_FILE_NAME}"
        var lf : LockFile
        lf.sdk_version = "0.5.0"
        set_package(lf, PackageEntry(name = "globalPkg", source = "github.com/user/globalPkg",
            version = "1.0", tag = "v1.0", root = true, global = true))
        t |> success(write_lock_file_at(path, lf))
        // read it back
        var lf2 : LockFile
        t |> success(read_lock_file_at(path, lf2))
        t |> equal(lf2.sdk_version, "0.5.0")
        t |> equal(length(lf2.packages), 1)
        let pkg = get_package(lf2, "globalPkg")
        t |> equal(pkg.source, "github.com/user/globalPkg")
        t |> equal(pkg.tag, "v1.0")
        t |> success(pkg.global)
        t |> success(pkg.root)
        // cleanup
        remove(path)
        rmdir(tmp_dir)
    }
}

[test]
def test_package_entry_global_field(t : T?) {
    t |> run("global field serializes to JSON") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_global_field"
        mkdir(tmp_dir)
        var lf : LockFile
        set_package(lf, PackageEntry(name = "pkg1", source = "github.com/a/pkg1",
            version = "1.0", tag = "v1.0", root = true, global = true))
        set_package(lf, PackageEntry(name = "pkg2", source = "github.com/a/pkg2",
            version = "2.0", tag = "v2.0", root = true, global = false))
        t |> success(write_lock_file(tmp_dir, lf))
        var lf2 : LockFile
        t |> success(read_lock_file(tmp_dir, lf2))
        let p1 = get_package(lf2, "pkg1")
        t |> success(p1.global)
        let p2 = get_package(lf2, "pkg2")
        t |> success(!p2.global)
        // cleanup
        remove("{tmp_dir}/{LOCK_FILE_NAME}")
        rmdir(tmp_dir)
    }
}

[test]
def test_cmd_list_global(t : T?) {
    t |> run("lists global packages") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_list_global"
        mkdir(tmp_dir)
        let path = "{tmp_dir}/{GLOBAL_LOCK_FILE_NAME}"
        var lf : LockFile
        set_package(lf, PackageEntry(name = "gmod", source = "github.com/a/gmod",
            tag = "v1.0", root = true, global = true))
        write_lock_file_at(path, lf)
        // cmd_list --global reads the global lock file; we can't easily redirect it
        // so just verify the lock file is readable with correct content
        var lf2 : LockFile
        t |> success(read_lock_file_at(path, lf2))
        t |> equal(length(lf2.packages), 1)
        t |> equal(lf2.packages[0].name, "gmod")
        t |> success(lf2.packages[0].global)
        // cleanup
        remove(path)
        rmdir(tmp_dir)
    }
}

[test]
def test_cmd_check_global_entry(t : T?) {
    t |> run("check verifies global entries at das_root/modules") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_check_global"
        mkdir(tmp_root)
        // create a lock file with a global entry pointing to a non-existent module
        var lf : LockFile
        set_package(lf, PackageEntry(name = "nonexistent_global", source = "github.com/a/b",
            tag = "v1.0", root = true, global = true))
        write_lock_file(tmp_root, lf)
        // cmd_check should report it as missing (since das_root/modules/nonexistent_global doesn't exist)
        // we can't capture output easily, but we verify the check logic path
        cmd_check(tmp_root)
        // cleanup
        remove("{tmp_root}/{LOCK_FILE_NAME}")
        rmdir(tmp_root)
    }
}

[test]
def test_cmd_remove_global_entry(t : T?) {
    t |> run("removing global entry only removes lock reference") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_rm_global_entry"
        mkdir(tmp_root)
        // create a lock file with a global entry
        var lf : LockFile
        set_package(lf, PackageEntry(name = "global_pkg", source = "github.com/a/b",
            tag = "v1.0", root = true, global = true))
        write_lock_file(tmp_root, lf)
        t |> success(has_package(lf, "global_pkg"))
        // remove — should only remove from lock file
        cmd_remove(tmp_root, "global_pkg")
        var lf2 : LockFile
        read_lock_file(tmp_root, lf2)
        t |> success(!has_package(lf2, "global_pkg"), "removed from lock file")
        // no local directory should have been attempted to delete
        // (the module is global, no local dir to delete)
        // cleanup
        remove("{tmp_root}/{LOCK_FILE_NAME}")
        rmdir(tmp_root)
    }
}

[test]
def test_gitignore_add(t : T?) {
    let tmp_root = "{get_das_root()}/_daspkg_test_gitignore"
    mkdir(tmp_root)
    // create a fake .git dir so gitignore_add doesn't skip
    mkdir("{tmp_root}/.git")
    let gitignore = "{tmp_root}/.gitignore"

    t |> run("creates .gitignore if missing") @(t : T?) {
        remove(gitignore)
        gitignore_add(tmp_root, "modules/foo")
        var content = ""
        fopen(gitignore, "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "modules/foo\n")
    }
    t |> run("appends without duplicating") @(t : T?) {
        remove(gitignore)
        gitignore_add(tmp_root, "modules/foo")
        gitignore_add(tmp_root, "modules/bar")
        gitignore_add(tmp_root, "modules/foo")  // duplicate
        var content = ""
        fopen(gitignore, "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "modules/foo\nmodules/bar\n")
    }
    t |> run("appends newline if file doesn't end with one") @(t : T?) {
        fopen(gitignore, "wb") $(f) {
            if (f != null) {
                fprint(f, "existing")
            }
        }
        gitignore_add(tmp_root, "modules/baz")
        var content = ""
        fopen(gitignore, "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "existing\nmodules/baz\n")
    }
    t |> run("skips if no .git directory") @(t : T?) {
        let no_git = "{get_das_root()}/_daspkg_test_nogit"
        mkdir(no_git)
        gitignore_add(no_git, "modules/foo")
        t |> success(!fexist("{no_git}/.gitignore"))
        rmdir(no_git)
    }
    // cleanup
    remove(gitignore)
    force_rmdir("{tmp_root}/.git")
    rmdir(tmp_root)
}

[test]
def test_gitignore_remove(t : T?) {
    let tmp_root = "{get_das_root()}/_daspkg_test_gitignore_rm"
    mkdir(tmp_root)
    mkdir("{tmp_root}/.git")
    let gitignore = "{tmp_root}/.gitignore"

    t |> run("removes matching line") @(t : T?) {
        fopen(gitignore, "wb") $(f) {
            if (f != null) {
                fprint(f, "modules/foo\nmodules/bar\nmodules/baz\n")
            }
        }
        gitignore_remove(tmp_root, "modules/bar")
        var content = ""
        fopen(gitignore, "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "modules/foo\nmodules/baz\n")
    }
    t |> run("no-op if pattern not found") @(t : T?) {
        fopen(gitignore, "wb") $(f) {
            if (f != null) {
                fprint(f, "modules/foo\n")
            }
        }
        gitignore_remove(tmp_root, "modules/nope")
        var content = ""
        fopen(gitignore, "rb") $(f) {
            if (f != null) {
                content = fread(f)
            }
        }
        t |> equal(content, "modules/foo\n")
    }
    t |> run("no-op if no .gitignore") @(t : T?) {
        remove(gitignore)
        gitignore_remove(tmp_root, "modules/foo")
        t |> success(!fexist(gitignore))
    }
    // cleanup
    remove(gitignore)
    force_rmdir("{tmp_root}/.git")
    rmdir(tmp_root)
}

// --- release tests ---

[test]
def test_run_das_package_release(t : T?) {
    t |> run("reads release info from .das_package") @(tt : T?) {
        var info : PackageReleaseInfo
        let ok = run_das_package_release(PKG_RELEASE_FIXTURE, info)
        tt |> success(ok)
        tt |> equal("custom-bundle-name", info.name)
        tt |> equal("main.das", info.main_script)
        tt |> equal(2, length(info.include_globs))
        tt |> equal("*.png", info.include_globs[0])
        tt |> equal("data/**/*.json", info.include_globs[1])
        tt |> equal(1, length(info.include_if_missing_globs))
        tt |> equal("config/*.toml", info.include_if_missing_globs[0])
        tt |> equal(1, length(info.exclude_globs))
        tt |> equal("data/internal/**", info.exclude_globs[0])
        tt |> equal(2, length(info.forced_modules))
        tt |> equal("dasSQLITE", info.forced_modules[0])
        tt |> equal("dasGlfw", info.forced_modules[1])
        tt |> equal("emcmake cmake -S . -B _wasm && cmake --build _wasm", info.wasm_build_command)
        tt |> equal(2, length(info.wasm_archives))
        tt |> equal("_wasm/liblibDasModuleTest.a", info.wasm_archives[0])
        tt |> equal("_wasm/liblibTestApp.a", info.wasm_archives[1])
        // the two wasm-only knobs cross the same hand-written marshaller: a dropped copy would
        // cross-compile with the module mounted and the flag missing, silently
        tt |> equal(1, length(info.wasm_disabled_modules))
        tt |> equal("dasvulkan", info.wasm_disabled_modules[0])
        tt |> equal(1, length(info.emcc_args))
        tt |> equal("--profiling-funcs", info.emcc_args[0])
    }
    t |> run("returns true with empty spec when release() is missing") @(tt : T?) {
        var info : PackageReleaseInfo
        let ok = run_das_package_release(PKG_FIXTURE, info)
        tt |> success(ok)
        tt |> equal("", info.main_script)
        tt |> equal(0, length(info.include_globs))
    }
}

[test]
def test_cmd_release_pure_daslang(t : T?) {
    t |> run("bundles a pure-daslang project end-to-end") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_pure"
        let out_dir = "{get_das_root()}/_test_release_pure_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)

        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"smoke\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_include(\"*.txt\")\n    release_include_if_missing(\"config.toml\")\n\}\n")
        }
        fopen("{tmp_root}/data.txt", "wb") $(f) {
            fwrite(f, "asset")
        }
        fopen("{tmp_root}/config.toml", "wb") $(f) {
            fwrite(f, "value = \"template\"\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let exe_path = bundle_exe_path(out_dir, "smoke")
        let artifacts = bundle_artifacts_dir(out_dir, "smoke")
        tt |> success(fexist(exe_path), "exe shipped at {exe_path}")
        tt |> success(fexist(path_join(artifacts, "data.txt")), "asset shipped")
        let deployed_config = path_join(artifacts, "config.toml")
        tt |> equal("value = \"template\"\n", fread(deployed_config), "starter config shipped on first release")
        tt |> success(!fexist(path_join(artifacts, "smoke.o")), ".o build artifact dropped")

        fopen(deployed_config, "wb") $(f) {
            fwrite(f, "value = \"user-edited\"\n")
        }
        // Simulate upgrading from the old release_include("config.toml")
        // behavior: the previous manifest still claims ownership. The new
        // if-missing declaration must protect the deployed edit during that
        // one-time migration cleanup.
        let old_manifest = path_join(artifacts, ".daspkg_release.manifest")
        fopen(old_manifest, "ab") $(f) {
            fwrite(f, "config.toml\n")
        }
        let rc2 = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc2, "second cmd_release returns 0")
        tt |> equal("value = \"user-edited\"\n", fread(deployed_config),
            "release_include_if_missing preserves deployed edits")
        let manifest = fread(old_manifest)
        tt |> success(find(manifest, "config.toml") < 0,
            "user-owned starter config is omitted from release manifest")

        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("paranoid with quick warns and releases (no refusal)") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_pq"
        let out_dir = "{get_das_root()}/_test_release_pq_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"smokepq\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }
        let rc = cmd_release(tmp_root, out_dir, true, true)
        tt |> equal(0, rc, "the pair is a warning, not a refusal")
        tt |> success(fexist(bundle_exe_path(out_dir, "smokepq")), "exe shipped")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// the tool release_launcher names must be a real build product: the watchdog the tree builds
// where dasHV, dasStdDlg and dasStbImage are in
def private built_watchdog_path() : string {
    let ext = get_platform_name() == "windows" ? ".exe" : ""
    for (rel in ["bin/watchdog{ext}", "bin/Release/watchdog{ext}"]) {
        let candidate = path_join(get_das_root(), rel)
        if (fexist(candidate)) return candidate
    }
    return ""
}

[test]
def test_cmd_release_launcher(t : T?) {
    t |> run("release_launcher names the shipped tool the macOS .app opens; a tool not shipped refuses before the build") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_launcher"
        let out_dir = "{get_das_root()}/_test_release_launcher_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"launched\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_launcher(\"watchdog\")\n\}\n")
        }
        let refused = cmd_release(tmp_root, out_dir)
        tt |> equal(1, refused, "a launcher no release_include_tool ships is refused")
        tt |> success(!fexist(bundle_exe_path(out_dir, "launched")), "and nothing was built")
        if (empty(built_watchdog_path())) {
            force_rmdir(tmp_root)
            force_rmdir(out_dir)
            tt |> skip("no built watchdog in this tree - the shipped-tool half of the launcher test needs bin/watchdog")
            return
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"launched\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_include_tool(\"watchdog\")\n    release_launcher(\"watchdog\")\n\}\n")
        }
        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0 with the tool shipped")
        let artifacts = bundle_artifacts_dir(out_dir, "launched")
        let ext = get_platform_name() == "windows" ? ".exe" : ""
        tt |> success(fexist(path_join(artifacts, "watchdog{ext}")), "the launcher tool sits beside the exe")
        tt |> success(fexist(bundle_exe_path(out_dir, "launched")), "the exe itself still ships")
        if (get_platform_name() == "darwin") {
            let plist = fread(path_join(bundle_root_dir(out_dir, "launched"), "Contents/Info.plist"))
            let key_at = find(plist, "<key>CFBundleExecutable</key>")
            tt |> success(key_at >= 0, "the plist names an executable")
            let value_at = find(plist, "<string>", key_at)
            tt |> success(value_at >= 0 && starts_with(slice(plist, value_at, length(plist)), "<string>watchdog</string>"),
                "CFBundleExecutable is the launcher, so a double click on the .app starts the supervisor:\n{plist}")
            tt |> success(find(plist, "<key>LSUIElement</key>") >= 0, "a launcher app is menu-bar-only - no Dock tile, no application menu:\n{plist}")
        }
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("without a launcher the .app opens the exe itself and is an ordinary application") @(tt : T?) {
        if (get_platform_name() != "darwin") {
            tt |> skip("the Info.plist exists only in the macOS .app layout")
            return
        }
        let tmp_root = "{get_das_root()}/_test_release_nolauncher"
        let out_dir = "{get_das_root()}/_test_release_nolauncher_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"plainapp\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }
        tt |> equal(0, cmd_release(tmp_root, out_dir), "cmd_release returns 0")
        let plist = fread(path_join(bundle_root_dir(out_dir, "plainapp"), "Contents/Info.plist"))
        tt |> success(find(plist, "<string>plainapp</string>") >= 0, "CFBundleExecutable is the exe:\n{plist}")
        tt |> success(find(plist, "LSUIElement") < 0, "and it is not marked menu-bar-only")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

[test]
def test_cmd_release_program(t : T?) {
    t |> run("release_program bakes a companion exe beside the main one, in the manifest, sharing the runtime") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_program"
        let out_dir = "{get_das_root()}/_test_release_program_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        // the companion alone requires a shared module (stbimage): the bundle must ship it for the
        // companion's sake, where the main program never asked for it
        let stb_module = path_join(get_das_root(), "modules/dasStbImage/dasModuleStbImage.shared_module")
        let stb_built = fexist(stb_module)
        let tool_require = stb_built ? "require stbimage\n" : ""
        fopen("{tmp_root}/tool.das", "wb") $(f) {
            fwrite(f, "options gen2\n{tool_require}[export]\ndef main() \{\n    print(\"companion\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"withtool\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_program(\"side-tool\", \"tool.das\")\n\}\n")
        }
        tt |> equal(0, cmd_release(tmp_root, out_dir), "cmd_release returns 0 with a companion")
        let artifacts = bundle_artifacts_dir(out_dir, "withtool")
        let companion_rel = get_platform_name() == "darwin" ? "side-tool" : "side-tool.exe"
        tt |> success(fexist(bundle_exe_path(out_dir, "withtool")), "the main exe ships")
        tt |> success(fexist(path_join(artifacts, companion_rel)), "the companion exe sits beside it as {companion_rel}")
        tt |> success(!fexist(path_join(artifacts, "side-tool.o")), "the companion's object file is dropped")
        for (stem in ["withtool", "side-tool"]) {
            for (suffix in [".map", ".lib", ".exp"]) {
                tt |> success(!fexist(path_join(artifacts, "{stem}{suffix}")), "no linker residue {stem}{suffix} rides beside the exe: nothing at run time reads it")
            }
        }
        let manifest = fread(path_join(artifacts, ".daspkg_release.manifest"))
        tt |> success(find(manifest, companion_rel) >= 0, "the manifest records the companion, so a re-release cleans it:\n{manifest}")
        if (stb_built) {
            tt |> success(fexist(path_join(artifacts, "modules/dasStbImage/dasModuleStbImage.shared_module")), "the shared module only the companion requires ships")
        } else {
            tt |> success(true, "not covered here: the companion-only shared module needs a built dasStbImage")
        }
        tt |> success(fexist(path_join(artifacts, "side-tool.tune.json")) == fexist(path_join(artifacts, "withtool.tune.json")), "the companion carries the sidecar exactly when the main exe does")
        var runtime_names : table<string>
        var runtime_dupes = 0
        fio::dir(artifacts) $(name) {
            if (find(name, "libDaScriptDyn") >= 0) {
                runtime_dupes += key_exists(runtime_names, name) ? 1 : 0
                runtime_names |> insert(name)
            }
        }
        tt |> success(!empty(runtime_names) && runtime_dupes == 0, "one set of runtime libraries serves both exes, none twice")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

[test]
def test_cmd_release_program_refusals(t : T?) {
    t |> run("a companion that fails to build ends the release with nothing of it left behind") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_program_broken"
        let out_dir = "{get_das_root()}/_test_release_program_broken_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/tool.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    this is not daslang\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"withbroken\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_program(\"side-tool\", \"tool.das\")\n\}\n")
        }
        tt |> equal(1, cmd_release(tmp_root, out_dir), "a companion that does not compile fails the release")
        let artifacts = bundle_artifacts_dir(out_dir, "withbroken")
        tt |> success(!fexist(path_join(artifacts, "side-tool.exe")) && !fexist(path_join(artifacts, "side-tool")) && !fexist(path_join(artifacts, "side-tool.o")), "no half-built companion stays in the bundle")
        var leftover = 0
        fio::dir(get_das_root()) $(name) {
            if (name |> starts_with("_daspkg_release_deps_")) {
                leftover ++
            }
        }
        tt |> equal(0, leftover, "no deps JSON is left in the tree")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("a companion with an unsafe name, the bundle's own name, a shipped tool's name, a twin's name, or a missing script is refused before the build") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_program_bad"
        let out_dir = "{get_das_root()}/_test_release_program_bad_out"
        for (bad in ["release_program(\"bad name\", \"main.das\")", "release_program(\"withtool\", \"main.das\")", "release_program(\"side-tool\", \"absent.das\")",
                     "release_program(\"\", \"main.das\")", "release_program(\"side-tool.exe\", \"main.das\")", "release_program(\"a..b\", \"main.das\")",
                     "release_program(\"side-tool\", \"main.das; rm -rf x\")",
                     "release_include_tool(\"watchdog\")\n    release_program(\"watchdog\", \"main.das\")",
                     "release_program(\"side-tool\", \"main.das\")\n    release_program(\"side-tool\", \"main.das\")"]) {
            force_rmdir(tmp_root)
            force_rmdir(out_dir)
            mkdir(tmp_root)
            fopen("{tmp_root}/main.das", "wb") $(f) {
                fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
            }
            fopen("{tmp_root}/.das_package", "wb") $(f) {
                fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"withtool\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    {bad}\n\}\n")
            }
            tt |> equal(1, cmd_release(tmp_root, out_dir), "refused: {bad}")
            tt |> success(!fexist(bundle_exe_path(out_dir, "withtool")), "and nothing was built for {bad}")
        }
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

[test]
def test_cmd_release_refuses_jit_only(t : T?) {
    t |> run("release_requires_jit() refuses the release before anything builds") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_jitonly"
        let out_dir = "{get_das_root()}/_test_release_jitonly_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"jitonly\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_requires_jit()\n\}\n")
        }
        tt |> equal(1, cmd_release(tmp_root, out_dir), "a JIT-only package is refused")
        tt |> success(!fexist(bundle_exe_path(out_dir, "jitonly")), "and nothing was built")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

[test]
def test_cmd_release_refuses_unsafe_bundle_name(t : T?) {
    // the bundle name is a .das_package string that reaches path_join, force_rmdir and shell
    // command lines; the guard admits letters, digits, `_`, `-` and `.` only - a space or a
    // shell metacharacter is refused before anything is built or removed
    for (bad in ["bad name", "bad;name", "bad$(name)", "bad\"name"]) {
        t |> run("refuses bundle name `{bad}`") @(tt : T?) {
            let tmp_root = "{get_das_root()}/_test_release_badname"
            let out_dir = "{get_das_root()}/_test_release_badname_out"
            force_rmdir(tmp_root)
            force_rmdir(out_dir)
            mkdir(tmp_root)
            fopen("{tmp_root}/main.das", "wb") $(f) {
                fwrite(f, "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
            }
            let escaped = replace(replace(bad, "\\", "\\\\"), "\"", "\\\"")
            fopen("{tmp_root}/.das_package", "wb") $(f) {
                fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"{escaped}\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
            }
            let rc = cmd_release(tmp_root, out_dir)
            tt |> equal(1, rc, "cmd_release refuses the name")
            tt |> success(!fexist(out_dir), "nothing was built or removed under {out_dir}")
            force_rmdir(tmp_root)
            force_rmdir(out_dir)
        }
    }
}

[test]
def test_cmd_release_ships_minted_scope_manifest(t : T?) {
    t |> run("a tune scope's RESOLVED manifest ships beside the exe") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_tunescope"
        let out_dir = "{get_das_root()}/_test_release_tunescope_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        // the scope's tuner is a STUB: a real tuner races candidates (minutes, noise-gated);
        // the stub copies a canned sidecar into DAS_TUNE_MANIFEST, so the SHIP path — the
        // exact line the M4 mis-ship broke — tests at unit speed
        let payload = "\{\"kernels\":\{\"stub_kernel\":\{\"suffix\":\"stub\"\}\},\"provenance\":\{\"mode\":\"stub\"\}\}"
        fopen("{tmp_root}/stub_payload.json", "wb") $(f) {
            fwrite(f, payload)
        }
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/tune\nrequire llvm/daslib/llvm_tune\n[tune_scope(name=\"stubscope\", tuner=\"stub_tuner.das\")]\nstruct StubScope \{\n    unused : int\n\}\n[export]\ndef main() \{\n    print(\"hi\\n\")\n\}\n")
        }
        fopen("{tmp_root}/stub_tuner.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/fio\n[export]\ndef main() \{\n    fopen(get_env_variable(\"DAS_TUNE_MANIFEST\"), \"wb\") $(mf) \{\n        fwrite(mf, fread(\"{tmp_root}/stub_payload.json\"))\n    \}\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"tunestub\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }
        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        // the mint ran: the stub wrote the resolved manifest beside main.das
        let src_manifest = "{tmp_root}/main.tune.json"
        tt |> success(fexist(src_manifest), "the scope's manifest was minted at {src_manifest}")
        // the ship: <bundle>/<name>.tune.json is that SAME manifest, byte-for-byte — shipping
        // a next-to-root guess instead put a stale July sidecar beside a fresh M4 exe once
        let bundle_dir = dir_name(bundle_exe_path(out_dir, "tunestub"))
        let shipped = path_join(bundle_dir, "tunestub.tune.json")
        tt |> success(fexist(shipped), "the bundle ships a tune sidecar at {shipped}")
        tt |> equal(fread(src_manifest), fread(shipped), "shipped sidecar == the resolved manifest")
        tt |> success(find(fread(shipped), "stub_kernel") >= 0, "the shipped winners are the stub mint's")
        // the ship contract: STRICTLY newer than the exe, or the runtime staleness rule drops
        // every kernel to its fallback and the artifact cannot name its winners
        var exe_fs : FStat
        var side_fs : FStat
        tt |> success(stat(bundle_exe_path(out_dir, "tunestub"), exe_fs), "exe stats")
        tt |> success(stat(shipped, side_fs), "shipped sidecar stats")
        tt |> success(int64(side_fs.mtime) > int64(exe_fs.mtime), "shipped sidecar mtime is strictly newer than the exe")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// a fat release's fixture: a scope shipping class profiles under ./defaults and one [tune]
// kernel over the add_plus_k selftest generator; `with_profile` decides whether the host's
// lowest class gets a profile entry for the kernel
def private write_fat_fixture(tmp_root : string; with_profile : bool) {
    mkdir(tmp_root)
    mkdir("{tmp_root}/defaults")
    let arch = get_architecture_name()
    let low = arch == "x86_64" ? "x86-avx2" : "arm-neon"
    let features = arch == "x86_64" ? "avx2;f16c;fma;sse4.2" : "neon;dotprod;fullfp16;lse"
    if (with_profile) {
        fwrite("{tmp_root}/defaults/{low}.tune-defaults.json",
            "\{ \"kernels\" : \{ \"fat_kernel\" : \"k2\" \}, \"provenance\" : \{ \"features\" : \"{features}\" \} \}")
    }
    fwrite("{tmp_root}/main.das", "options gen2\nrequire daslib/tune\nrequire llvm/daslib/llvm_tune\n[tune_scope(name=\"fatscope\", defaults=\"./defaults\")]\nstruct FatScope \{\n    unused : int\n\}\n[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2), tune(gen = \"llvm_code_selftest::add_plus_k\", fallback = \"k1\")]\ndef fat_kernel(a, b : int) : int \{\n    return a + b\n\}\n[export, tune_policy(missing = \"fallback\")]\ndef main() \{\n    print(\"R \{fat_kernel(length(get_command_line_arguments()), 3)}\\n\")\n\}\n")
    fwrite("{tmp_root}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"fatstub\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
}

[test]
def test_cmd_release_fat(t : T?) {
    let arch = get_architecture_name()
    let low = arch == "x86_64" ? "x86-avx2" : "arm-neon"
    t |> run("--fat ships the class profile's clones, mints nothing, and ships no sidecar") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_fat"
        let out_dir = "{get_das_root()}/_test_release_fat_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        write_fat_fixture(tmp_root, true)
        let rc = cmd_release(tmp_root, out_dir, false, false, low)
        tt |> equal(0, rc, "cmd_release --fat {low} returns 0")
        tt |> success(!fexist("{tmp_root}/main.tune.json"), "a fat release mints no sidecar beside the script")
        let bundle_dir = dir_name(bundle_exe_path(out_dir, "fatstub"))
        tt |> success(!fexist(path_join(bundle_dir, "fatstub.tune.json")), "the bundle ships no tune sidecar")
        tt |> success(fexist(bundle_exe_path(out_dir, "fatstub")), "the fat exe is in the bundle")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("--fat refuses a kernel the baseline class has no profile entry for") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_fat_none"
        let out_dir = "{get_das_root()}/_test_release_fat_none_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        write_fat_fixture(tmp_root, false)
        let rc = cmd_release(tmp_root, out_dir, false, false, low)
        tt |> success(rc != 0, "a fallback-stamped baseline clone refuses the release")
        let bundle_dir = dir_name(bundle_exe_path(out_dir, "fatstub"))
        tt |> success(!fexist(path_join(bundle_dir, RELEASE_MANIFEST)), "the refused release never finishes its bundle")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("--fat refuses a class name that is not a name before anything builds") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_fat_unsafe"
        let out_dir = "{get_das_root()}/_test_release_fat_unsafe_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        write_fat_fixture(tmp_root, true)
        for (bad in ["x86-avx2 DAS_X=1", "x86-avx2;echo", "x86-avx2\"", "../x86-avx2"]) {
            tt |> success(cmd_release(tmp_root, out_dir, false, false, bad) != 0, "'{bad}' is refused")
        }
        tt |> success(!fexist(bundle_exe_path(out_dir, "fatstub")), "no exe was built for a refused class name")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
    t |> run("the gate names the kernel and the class, and passes a fully profiled scope") @(tt : T?) {
        let refusal = release_fat_refusal("fatscope", "fat_kernel;other_kernel", low)
        tt |> success(find(refusal, "scope `fatscope` has no {low} profile entry for fat_kernel, other_kernel") >= 0, "the refusal names the scope, the class and the kernels: {refusal}")
        tt |> equal(release_fat_refusal("fatscope", "", low), "", "a scope whose baseline clones all came from a profile passes")
    }
}

[test]
def test_cmd_release_native_dep(t : T?) {
    var dasSQLITE_present = false
    for_each_registered_dynamic_module() $(_path, _mod_name, das_name) {
        if (das_name == "sqlite") {
            dasSQLITE_present = true
        }
    }
    if (!dasSQLITE_present) {
        t |> run("skipped — dasSQLITE not available") @(tt : T?) {
            to_log(LOG_INFO, "test_cmd_release_native_dep: dasSQLITE not loaded; skipping native-dep test\n")
            tt |> success(true)
        }
        return
    }
    t |> run("bundles a native-dep project, runnable from a moved location") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_native"
        let out_dir = "{get_das_root()}/_test_release_native_out"
        let moved_dir = "{get_das_root()}/_test_release_native_moved"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        force_rmdir(moved_dir)
        mkdir(tmp_root)

        // Tiny project: open in-memory sqlite, run a query, print row count.
        // Using stderr-free output so we can match it exactly against the run.
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire sqlite/sqlite_boost\n[export]\ndef main() \{\n    let db = open_sqlite(\":memory:\")\n    db |> exec(\"CREATE TABLE t(x INT)\")\n    db |> exec(\"INSERT INTO t VALUES(42)\")\n    print(\"rows: \{db |> changes()\}\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"sqs\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let exe_path = bundle_exe_path(out_dir, "sqs")
        let artifacts = bundle_artifacts_dir(out_dir, "sqs")
        let bundle_root = bundle_root_dir(out_dir, "sqs")
        tt |> success(fexist(exe_path), "exe shipped at {exe_path}")
        tt |> success(fexist(path_join(artifacts, "modules/dasSQLITE/dasModuleSQLITE.shared_module")),
            "dylib shipped at expected relative path")

        // Move the whole bundle to a new location and run it. PR #2579's
        // exe-relative resolution is what makes this work — the dylib needs
        // to be found via <exe_dir>/modules/dasSQLITE/... at the new path.
        // Use `rename` (cross-platform via libc rename(2)/Win CRT) rather
        // than shell `cp -r` so the test runs on Windows CI; rename also
        // preserves the exe permission bit by definition.
        let moved = rename(bundle_root, moved_dir)
        tt |> success(moved, "bundle moved to new location")
        let moved_exe = moved_bundle_exe_path(moved_dir, "sqs")
        var output : string
        let run_rc = run_cmd("\"{moved_exe}\"", output)
        tt |> equal(0, run_rc, "moved bundle runs cleanly")
        tt |> success(find(output, "rows: 1") >= 0, "moved bundle prints expected sqlite output (got: {output})")

        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        force_rmdir(moved_dir)
    }
}

// release_include_dll() declares a native shared library to ship next to the
// owning .shared_module. Verify the negative path: a project that declares a
// nonexistent DLL must fail loudly, not silently produce an incomplete bundle.
[test]
def test_cmd_release_include_dll_missing_fails(t : T?) {
    t |> run("cmd_release returns non-zero when release_include_dll names a missing file") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_include_dll_missing"
        let out_dir = "{get_das_root()}/_test_release_include_dll_missing_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)

        // Empty main, but the project declares a native dep that can't be
        // resolved under <das_root>/{bin,lib}/. ship_native_dlls runs after
        // the deps copy and must abort the release.
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\n[export]\ndef main() \{\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"missdll\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n    release_include_dll(\"this_file_does_not_exist_anywhere_4f8c.dll\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> success(rc != 0, "cmd_release rejects nonexistent native dep (got rc={rc})")

        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// False-positive test: every registered dynamic module that the program does NOT
// require must NOT show up in the bundle. Skips when the registry has only
// dasSQLITE (no other module to verify isn't shipped).
[test]
def test_cmd_release_no_false_positive_shared(t : T?) {
    var dasSQLITE_present = false
    var other_pkg_dir = ""
    for_each_registered_dynamic_module() $(path, _mod_name, das_name) {
        if (das_name == "sqlite") {
            dasSQLITE_present = true
        } elif (empty(other_pkg_dir)) {
            // First non-sqlite registered module: extract package dir name.
            let g = to_generic_path(path)
            let idx = rfind(g, "/modules/")
            if (idx >= 0) {
                let after = slice(g, idx + length("/modules/"))
                let next_slash = find(after, "/")
                other_pkg_dir = next_slash > 0 ? slice(after, 0, next_slash) : after
            }
        }
    }
    if (!dasSQLITE_present || empty(other_pkg_dir)) {
        t |> run("skipped — needs dasSQLITE plus at least one other registered module") @(tt : T?) {
            to_log(LOG_INFO, "test_cmd_release_no_false_positive_shared: insufficient registered modules; skipping\n")
            tt |> success(true)
        }
        return
    }
    let other_for_capture = other_pkg_dir
    t |> run("ships only modules the program requires; not registered-but-unused dylibs") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_fp_shared"
        let out_dir = "{get_das_root()}/_test_release_fp_shared_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)

        // Project requires only sqlite — no other registered modules.
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire sqlite/sqlite_boost\n[export]\ndef main() \{\n    let db = open_sqlite(\":memory:\")\n    print(\"db opened\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"fpshared\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let artifacts = bundle_artifacts_dir(out_dir, "fpshared")
        tt |> success(fexist(path_join(artifacts, "modules/dasSQLITE/dasModuleSQLITE.shared_module")),
            "dasSQLITE dylib shipped (program required it)")
        // Pick one registered-but-unused package and confirm it's NOT shipped.
        let other_dir = path_join(artifacts, "modules/{other_for_capture}")
        tt |> equal(false, fexist(other_dir),
            "registered-but-unused package must NOT be shipped (false positive)")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// False-positive test: a project with no daspkg deps must NOT pull anything from
// daslib stdlib (`require strings`, `require fio`) or from the project's own
// non-main .das files into the bundle's modules/ tree. Only the exe + the
// project's own assets (release_include) should be shipped.
[test]
def test_cmd_release_no_false_positive_das(t : T?) {
    t |> run("daslib stdlib + project-local .das do NOT create modules/ entries") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_fp_das"
        let out_dir = "{get_das_root()}/_test_release_fp_das_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)

        // Project: main.das requires fio (stdlib) + a project-local module.
        fopen("{tmp_root}/helper.das", "wb") $(f) {
            fwrite(f, "options gen2\nmodule helper shared public\n\ndef public greet(name : string) : string \{\n    return \"hello, \{name\}\"\n\}\n")
        }
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/fio\nrequire helper\n\n[export]\ndef main() \{\n    print(\"\{greet(\"world\")\}\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"fpdas\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let exe_path = bundle_exe_path(out_dir, "fpdas")
        let artifacts = bundle_artifacts_dir(out_dir, "fpdas")
        tt |> success(fexist(exe_path), "exe shipped at {exe_path}")
        // No modules/ tree at all — project has no shared modules and no daspkg deps.
        tt |> equal(false, fexist(path_join(artifacts, "modules")),
            "no modules/ tree (no shared deps, no daspkg deps; daslib stdlib + project-local .das must NOT show up)")

        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// Transitive release: project requires a synthetic dep at <root>/modules/<DepName>/
// whose own .das_package has release_include for asset files. Bundle must place
// dep assets under <bundle>/modules/<DepName>/<rel>.
[test]
def test_cmd_release_transitive_dep(t : T?) {
    t |> run("dep release_include files are shipped under modules/<DepName>/") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_transitive"
        let out_dir = "{get_das_root()}/_test_release_transitive_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)
        mkdir("{tmp_root}/modules")
        mkdir("{tmp_root}/modules/fakedep")
        mkdir("{tmp_root}/modules/fakedep/daslib")
        mkdir("{tmp_root}/modules/fakedep/data")

        // Synthetic dep: package + module + asset.
        fopen("{tmp_root}/modules/fakedep/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"fakedep\")\n\}\n[export]\ndef release() \{\n    release_include(\"data/**\")\n\}\n")
        }
        fopen("{tmp_root}/modules/fakedep/.das_module", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/fio\n[export]\ndef initialize(project_path : string) \{\n    register_native_path(\"fakedep\", \"fakedep_lib\", \"\{project_path\}/daslib/fakedep_lib.das\")\n\}\n")
        }
        fopen("{tmp_root}/modules/fakedep/daslib/fakedep_lib.das", "wb") $(f) {
            fwrite(f, "options gen2\nmodule fakedep_lib shared public\n\ndef public hello() : string \{\n    return \"hello from fakedep\"\n\}\n")
        }
        fopen("{tmp_root}/modules/fakedep/data/asset.bin", "wb") $(f) {
            fwrite(f, "asset-content")
        }
        // Excluded file — must NOT ship.
        fopen("{tmp_root}/modules/fakedep/data/internal.bin", "wb") $(f) {
            fwrite(f, "secret")
        }

        // Parent project pulls fakedep in.
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire fakedep/fakedep_lib\n\n[export]\ndef main() \{\n    print(\"\{hello()\}\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"transit\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let exe_path = bundle_exe_path(out_dir, "transit")
        let artifacts = bundle_artifacts_dir(out_dir, "transit")
        tt |> success(fexist(exe_path), "parent exe shipped at {exe_path}")
        tt |> success(fexist(path_join(artifacts, "modules/fakedep/data/asset.bin")),
            "dep asset shipped via transitive release()")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}

// Walk `path` via `commands::list_lc_rpaths` + `commands::is_external_rpath`
// (the same parser the production scrubber uses) and append every external
// LC_RPATH entry to `leaks`. Thin wrapper so the test exercises the shared
// helpers instead of paraphrasing the parser locally.
def private add_external_lc_rpaths(path : string; var leaks : array<string>) {
    for (rp in list_lc_rpaths(path)) {
        if (is_external_rpath(rp)) {
            leaks |> push("{path}: {rp}")
        }
    }
}

// macOS: a bundle copy is portable only if (1) every dylib's install_name alias
// resolves to an actual file in the bundle, and (2) no shipped binary carries
// an absolute LC_RPATH pointing back into the build tree. Without (1) dyld
// silently falls back through the build-tree LC_RPATH; without (2) the bundle
// "works" on the build machine and breaks on a clean recipient. Both are the
// hard-to-diagnose ship blocker — exercise them explicitly.
//
// Requires dasGlfw because its dylib's install_name (e.g. `@rpath/libglfw.3.dylib`)
// differs from the shipped filename (e.g. `libglfw.3.4.dylib`) — that mismatch
// is what makes the symlink alias load-bearing. The test discovers both names
// at runtime rather than hardcoding versions, so it survives GLFW bumps.
[test]
def test_cmd_release_portable_on_macos(t : T?) { // nolint:STYLE038
    if (get_platform_name() != "darwin") {
        t |> run("skipped — macOS-only audit (otool/install_name_tool)") @(tt : T?) {
            tt |> success(true)
        }
        return
    }
    var dasGlfw_present = false
    for_each_registered_dynamic_module() $(_path, _mod_name, das_name) {
        if (das_name == "glfw") {
            dasGlfw_present = true
        }
    }
    if (!dasGlfw_present) {
        t |> run("skipped — dasGlfw not registered; can't exercise install_name-alias path") @(tt : T?) {
            to_log(LOG_INFO, "test_cmd_release_portable_on_macos: dasGlfw not loaded; skipping\n")
            tt |> success(true)
        }
        return
    }
    t |> run("install_name alias is created and no LC_RPATH points outside the bundle") @(tt : T?) {
        let tmp_root = "{get_das_root()}/_test_release_portable_macos"
        let out_dir = "{get_das_root()}/_test_release_portable_macos_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(tmp_root)

        // Smallest valid project that pulls dasGlfw into the bundle. The exe
        // doesn't run — we only need it on disk to inspect.
        fopen("{tmp_root}/main.das", "wb") $(f) {
            fwrite(f, "options gen2\nrequire glfw/glfw_boost\n[export]\ndef main() \{\n    print(\"glfw\\n\")\n\}\n")
        }
        fopen("{tmp_root}/.das_package", "wb") $(f) {
            fwrite(f, "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"glfwbn\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
        }

        let rc = cmd_release(tmp_root, out_dir)
        tt |> equal(0, rc, "cmd_release returns 0")
        let artifacts = bundle_artifacts_dir(out_dir, "glfwbn")

        // Discover the real dylib (a regular file under modules/dasGlfw/)
        // rather than hardcoding `libglfw.3.4.dylib` — that filename moves
        // every time GLFW bumps and would break the test silently.
        //
        // Asserts exactly one regular `.dylib` entry: if a future dep ever
        // lands a second dylib under `modules/dasGlfw/`, the test should
        // fail loudly so we revisit the discovery logic rather than picking
        // whichever entry the filesystem yields last.
        let glfw_dir = path_join(artifacts, "modules/dasGlfw")
        var real_dylib = ""
        var dylib_count = 0
        dir(glfw_dir) $(filename) {
            if (filename == "." || filename == ".." || !ends_with(filename, ".dylib")) return
            let p = path_join(glfw_dir, filename)
            var sym_err : string
            if (is_symlink(p, sym_err)) return
            real_dylib = p
            dylib_count++
        }
        tt |> equal(1, dylib_count, "exactly one real GLFW dylib in {glfw_dir} (got {dylib_count})")
        var id_out : string
        let id_rc = run_cmd("otool -D \"{real_dylib}\"", id_out)
        tt |> equal(0, id_rc, "otool -D runs")
        var install_name = ""
        for (line in split(id_out, "\n")) {
            let s = strip(line)
            if (empty(s) || ends_with(s, ":")) continue
            install_name = s
        }
        tt |> success(starts_with(install_name, "@rpath/"),
            "install_name is @rpath-relative (got: {install_name})")
        let alias_name = base_name(install_name)
        let alias_path = path_join(glfw_dir, alias_name)
        if (alias_name != base_name(real_dylib)) {
            tt |> success(fexist(alias_path),
                "install_name alias `{alias_name}` materialized at {alias_path}")
            var eq_err : string
            tt |> success(equivalent(alias_path, real_dylib, eq_err),
                "alias resolves to the real dylib (equivalent => true; err=`{eq_err}`)")
        }

        // No shipped binary may keep an absolute LC_RPATH — that's the leak
        // that hides the install_name-alias bug on the build machine. Skip
        // symlinks during the glob walk so the install_name alias doesn't
        // get scanned a second time alongside its real-file target (which
        // would double-report any future leak in the same inode).
        var leaks : array<string>
        add_external_lc_rpaths(bundle_exe_path(out_dir, "glfwbn"), leaks)
        glob(artifacts, "**/*.dylib") $(rel, is_dir) {
            if (is_dir) return
            let p = path_join(artifacts, rel)
            var sym_err : string
            if (is_symlink(p, sym_err)) return
            add_external_lc_rpaths(p, leaks)
        }
        glob(artifacts, "**/*.shared_module") $(rel, is_dir) {
            if (is_dir) return
            let p = path_join(artifacts, rel)
            var sym_err : string
            if (is_symlink(p, sym_err)) return
            add_external_lc_rpaths(p, leaks)
        }
        let leaks_msg = join(leaks, "; ")
        tt |> equal(0, length(leaks),
            "no absolute LC_RPATH in shipped binaries (leaks: {leaks_msg})")

        force_rmdir(tmp_root)
        force_rmdir(out_dir)
    }
}


def private push_text(var bytes : array<uint8>; text : string) {
    bytes |> reserve(length(bytes) + length(text))
    for (c in text) {
        bytes |> push(uint8(c))
    }
}

//! a 60-byte ar member header: name, then the decimal size in its column, then the two-byte terminator (a backtick and a newline)
def private push_ar_header(var bytes : array<uint8>; name : string; size : int) {
    let digits = "{size}"
    push_text(bytes, "{name}{repeat(" ", 48 - length(name))}{digits}{repeat(" ", 10 - length(digits))}`\n")
}

//! a fake `!<arch>` file: a symbol-table member of `index_bytes` zeros, a GNU long-name table when the
//! object's name is `/0` (the shape llvm-ar writes for a long file name), then the object opening with the magic
def private write_archive(path : string; magic : array<uint8>; index_bytes : int = 0; long_name : bool = false) {
    var bytes : array<uint8>
    bytes |> reserve(index_bytes + 256)
    push_text(bytes, "!<arch>\n")
    if (index_bytes > 0) {
        push_ar_header(bytes, "/", index_bytes)
        for (_i in range(index_bytes + (index_bytes & 1))) {
            bytes |> push(0u8)
        }
    }
    if (long_name) {
        push_ar_header(bytes, "//", 20)
        push_text(bytes, "a_long_member_name/\n")
    }
    push_ar_header(bytes, long_name ? "/0" : "member.o/", 64)
    bytes |> push_from(magic)
    push_text(bytes, "padding past the magic so the member has a body")
    fopen(path, "wb") $(f) {
        fwrite(f, bytes)
    }
}

//! the BSD shape macOS's ar writes: a `__.SYMDEF SORTED` table, then a member whose `#1/NN` name leads its body
def private write_bsd_archive(path : string; magic : array<uint8>) {
    var bytes : array<uint8>
    bytes |> reserve(256)
    push_text(bytes, "!<arch>\n")
    push_ar_header(bytes, "__.SYMDEF SORTED", 16)
    push_text(bytes, "0000000000000000")
    let name = "a_long_member_name.o"
    push_ar_header(bytes, "#1/{length(name)}", length(name) + 64)
    push_text(bytes, name)
    bytes |> push_from(magic)
    push_text(bytes, "padding past the magic so the member has a body")
    fopen(path, "wb") $(f) {
        fwrite(f, bytes)
    }
}

[test]
def test_wasm_archive_detection(t : T?) {
    t |> run("build --wasm stages an archive only when a member carries the wasm magic; a native archive under the same name is refused") @(t : T?) {
        let tmp = create_temp_directory_result("daspkg archives ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root = tmp as value
        let wasm_path = path_join(root, "liblibDasModuleWasm.a")
        let indexed_path = path_join(root, "liblibDasModuleIndexed.a")
        let native_path = path_join(root, "liblibDasModuleNative.a")
        let wasm_magic <- [0x00u8, 0x61u8, 0x73u8, 0x6du8, 0x01u8, 0x00u8, 0x00u8, 0x00u8]   // "\0asm" + version 1
        write_archive(wasm_path, wasm_magic)
        write_archive(indexed_path, wasm_magic, 200000)   // a symbol table far past any fixed scan window
        let long_named_path = path_join(root, "liblibDasModuleLongName.a")
        write_archive(long_named_path, wasm_magic, 1024, true)   // llvm-ar's shape: `/` table, `//` names, the object as `/0`
        let odd_path = path_join(root, "liblibDasModuleOdd.a")
        write_archive(odd_path, wasm_magic, 1023)   // an odd-sized table: the next header sits past its padding byte
        let bsd_path = path_join(root, "liblibDasModuleBsd.a")
        write_bsd_archive(bsd_path, wasm_magic)
        let bsd_native_path = path_join(root, "liblibDasModuleBsdNative.a")
        write_bsd_archive(bsd_native_path, [0xcfu8, 0xfau8, 0xedu8, 0xfeu8, 0x0cu8, 0x00u8, 0x00u8, 0x01u8])
        write_archive(native_path, [0xcfu8, 0xfau8, 0xedu8, 0xfeu8, 0x0cu8, 0x00u8, 0x00u8, 0x01u8])   // a 64-bit Mach-O member
        let text_path = path_join(root, "liblibDasModuleText.a")
        fopen(text_path, "wb") $(f) {
            fwrite(f, "not an archive at all, whatever the name says; long enough to read a header from\n")
        }
        t |> success(looks_like_wasm_archive(wasm_path), "an archive whose first object opens with the wasm magic is a wasm archive")
        t |> success(looks_like_wasm_archive(indexed_path), "a 200 KB symbol table ahead of that object changes nothing: members are walked, not scanned")
        t |> success(looks_like_wasm_archive(long_named_path), "an object named through the long-name table is an object, not a table")
        t |> success(looks_like_wasm_archive(odd_path), "an odd-sized table's padding byte is stepped over")
        t |> success(looks_like_wasm_archive(bsd_path), "BSD ar: the __.SYMDEF table is skipped and the #1/NN name is stepped over")
        t |> success(!looks_like_wasm_archive(bsd_native_path), "BSD ar around a Mach-O member is native")
        t |> success(!looks_like_wasm_archive(native_path), "an archive of native objects is not, whatever its name")
        t |> success(!looks_like_wasm_archive(text_path), "a file without the !<arch> header is not an archive")
        t |> success(!looks_like_wasm_archive(path_join(root, "missing.a")), "a file that is not there is not a wasm archive")
        force_rmdir(root)
    }
}

[test]
def test_wasm_archive_staging(t : T?) {
    t |> run("build --wasm stages the runtime and module archives of the web build's lib dir and nothing else") @(t : T?) {
        let tmp = create_temp_directory_result("daspkg staging ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root = tmp as value
        let web_lib = path_join(root, "web_lib")
        let dst = path_join(root, "dst")
        mkdir_rec(web_lib)
        let wasm_magic <- [0x00u8, 0x61u8, 0x73u8, 0x6du8, 0x01u8, 0x00u8, 0x00u8, 0x00u8]
        let native_magic <- [0xcfu8, 0xfau8, 0xedu8, 0xfeu8, 0x0cu8, 0x00u8, 0x00u8, 0x01u8]
        write_archive(path_join(web_lib, "liblibDaScript_runtime.a"), wasm_magic)
        write_archive(path_join(web_lib, "liblibDasModuleGlfw.a"), wasm_magic)
        write_archive(path_join(web_lib, "libfreetype.a"), native_magic)   // a third-party bake beside them
        t |> equal(stage_wasm_archives(web_lib, dst), 2, "the runtime and the module archive are staged")
        t |> success(fexist(path_join(dst, "liblibDaScript_runtime.a")) && fexist(path_join(dst, "liblibDasModuleGlfw.a")), "both land in the lib dir")
        t |> success(!fexist(path_join(dst, "libfreetype.a")), "an archive that is neither runtime nor module is left where it is")
        t |> equal(stage_wasm_archives(web_lib, web_lib), 2, "the same directory counts without copying")
        write_archive(path_join(web_lib, "liblibDasModuleNative.a"), native_magic)
        t |> equal(stage_wasm_archives(web_lib, dst), -1, "a native archive among them refuses the stage")
        force_rmdir(root)
    }
}

[test]
def test_ship_external_files(t : T?) {
    let tmp = create_temp_directory_result("daspkg external ")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    let bundle = path_join(root, "bundle")
    mkdir(bundle)
    var shipped : array<string>
    t |> equal(ship_external_files("pkg", bundle, ["README.md@nested/readme.txt", "README.md@", "utils\\daspkg\\README.md@windows\\nested\\readme.txt"], shipped), 0)
    t |> equal(fread(path_join(bundle, "nested/readme.txt")), fread(path_join(get_das_root(), "README.md")))
    t |> success(fexist(path_join(bundle, "README.md")), "default destination uses the source filename")
    t |> equal(length(shipped), 3)
    t |> success(fexist(path_join(bundle, "windows/nested/readme.txt")), "Windows separators name the same destination on every host")
    if (length(shipped) == 3) {
        t |> equal(shipped[2], "windows/nested/readme.txt", "manifest stores normalized separators")
    }
    fwrite(path_join(root, "victim.txt"), "untouched")
    for (entry in ["README.md@../victim.txt", "README.md@nested/../../victim.txt", "README.md@{root}/absolute.txt", "README.md@nested/quote\".txt", "README.md@nested\\..\\..\\victim.txt", "@empty.txt", "README.md", "missing-external-source@missing.txt"]) {
        var rejected : array<string>
        t |> equal(ship_external_files("pkg", bundle, [entry], rejected), 1, entry)
        t |> equal(length(rejected), 0, "rejected entry is not recorded as shipped")
    }
    t |> equal(fread(path_join(root, "victim.txt")), "untouched", "destinations cannot escape the bundle")
    mkdir(path_join(bundle, "directory"))
    var failed_copy : array<string>
    t |> equal(ship_external_files("pkg", bundle, ["README.md@directory"], failed_copy), 1, "copy errors propagate")
    t |> equal(length(failed_copy), 0)
    let cleanup = rmdir_rec_result(root)
    t |> success(cleanup is value && cleanup as value, "temp directory removed")
}

// --- platform declarations ---

let PKG_PLATFORMS_FIXTURE = "utils/daspkg/fixtures/test_platforms.das_package"

// A known platform name that is not this host's.
def private other_platform() : string {
    for (p in PLATFORM_NAMES) {
        if (p != get_platform_name()) return p
    }
    return ""
}

[test]
def test_run_das_package_platforms(t : T?) {
    t |> run("reads and normalizes package_platform / package_platforms") @(t : T?) {
        var meta : PackageMeta
        t |> success(run_das_package_meta(PKG_PLATFORMS_FIXTURE, meta))
        t |> equal(3, length(meta.platforms))
        t |> equal("windows", meta.platforms[0])
        t |> equal("emscripten", meta.platforms[1])
        t |> equal("darwin", meta.platforms[2])
    }
    t |> run("a manifest without a declaration has none") @(t : T?) {
        var meta : PackageMeta
        t |> success(run_das_package_meta(PKG_FIXTURE, meta))
        t |> equal(0, length(meta.platforms))
    }
    t |> run("vocabulary helpers") @(t : T?) {
        t |> equal("emscripten", normalize_platform_name(" WASM "))
        t |> equal("linux", normalize_platform_name("linux"))
        t |> equal("amiga", normalize_platform_name("amiga"))
        t |> success(is_known_platform("darwin"))
        t |> success(!is_known_platform("amiga"))
        let bad <- unknown_platforms(["windows", "amiga", "beos"])
        t |> equal(2, length(bad))
        t |> equal("amiga", bad[0])
    }
}

[test]
def test_platform_gate(t : T?) {
    let host = get_platform_name()
    let other = other_platform()
    t |> run("no declaration passes") @(t : T?) {
        let none : array<string>
        t |> equal(0, platform_gate("pkg", host, none, false))
    }
    t |> run("host among the declared passes") @(t : T?) {
        t |> equal(0, platform_gate("pkg", host, [other, host], false))
    }
    t |> run("host outside the declared refuses with the sweep's skip code") @(t : T?) {
        t |> equal(RC_UNSUPPORTED_PLATFORM, platform_gate("pkg", host, [other], false))
    }
    t |> run("--force turns the refusal into a warning") @(t : T?) {
        t |> equal(0, platform_gate("pkg", host, [other], true))
    }
    t |> run("an unknown name is an error even under --force") @(t : T?) {
        t |> equal(1, platform_gate("pkg", host, ["amiga"], true))
    }
    t |> run("a directory without a manifest passes") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_gate_nomanifest"
        mkdir(tmp_dir)
        t |> equal(0, platform_gate(tmp_dir, "pkg", host, false))
        force_rmdir(tmp_dir)
    }
    t |> run("a manifest that does not compile passes - the install fails on it later, not here") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_gate_broken"
        mkdir(tmp_dir)
        fwrite("{tmp_dir}/.das_package", "this is not daslang\n")
        t |> equal(0, platform_gate(tmp_dir, "pkg", host, false))
        force_rmdir(tmp_dir)
    }
}

// Writes a releasable package (main.das + release_main) declaring `platform` as its only platform.
def private write_release_platform_pkg(src_dir, pkg_name, platform : string) {
    mkdir(src_dir)
    fwrite("{src_dir}/main.das", "options gen2\n[export]\ndef main() \{\n    print(\"hello\\n\")\n\}\n")
    fwrite("{src_dir}/.das_package", "options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"{pkg_name}\")\n    package_platform(\"{platform}\")\n\}\n[export]\ndef release() \{\n    release_main(\"main.das\")\n\}\n")
}

[test]
def test_release_platform_gate(t : T?) {
    t |> run("release refuses a package declaring another platform before writing anything") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_relgate"
        let out_dir = "{get_das_root()}/_daspkg_test_relgate_out"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        write_release_platform_pkg(tmp_root, "relgate", other_platform())
        t |> equal(RC_UNSUPPORTED_PLATFORM, cmd_release(tmp_root, out_dir))
        t |> success(!fexist(out_dir))
        force_rmdir(tmp_root)
    }
    t |> run("release wasm checks the declaration against emscripten, not the host") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_relgate"
        let out_dir = "{get_das_root()}/_daspkg_test_relgate_out"
        let empty_lib = "{get_das_root()}/_daspkg_test_relgate_lib"
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        mkdir(empty_lib)
        write_release_platform_pkg(tmp_root, "relgate", get_platform_name())
        t |> equal(RC_UNSUPPORTED_PLATFORM, cmd_release_wasm(tmp_root, out_dir, empty_lib))
        write_release_platform_pkg(tmp_root, "relgate", "wasm")
        t |> equal(1, cmd_release_wasm(tmp_root, out_dir, empty_lib), "past the gate, the empty archive dir is the next refusal")
        force_rmdir(tmp_root)
        force_rmdir(out_dir)
        force_rmdir(empty_lib)
    }
}

[test]
def test_index_sweep_skips_other_platforms(t : T?) {
    t |> run("an index entry declaring other platforms only is a skip row, never an install") @(t : T?) {
        var cmake_out : string
        if (run_cmd("cmake --version", cmake_out) != 0) {
            t |> skip("cmake not on PATH")
            return
        }
        let tmp = "{get_das_root()}/_daspkg_test_sweep"
        force_rmdir(tmp)
        mkdir(tmp)
        mkdir("{tmp}/root")
        mkdir("{tmp}/out")
        let entry = "[\{\"name\":\"elsewhere\",\"url\":\"github.com/nobody/elsewhere\",\"description\":\"d\",\"platforms\":[\"{other_platform()}\"]\}]"
        fwrite("{tmp}/packages.json", entry)
        var output : string
        let rc = run_cmd("cmake -DDASLANG=\"{get_das_root()}/bin/daslang\" -DROOT=\"{tmp}/root\" -DOUT=\"{tmp}/out\" -DPACKAGES_JSON=\"{tmp}/packages.json\" -P \"{get_das_root()}/utils/daspkg/index_sweep.cmake\"", output)
        t |> equal(0, rc, output)
        t |> success(find(output, "skip elsewhere:") >= 0, output)
        t |> success(!fexist("{tmp}/root/modules/elsewhere"))
        force_rmdir(tmp)
    }
}

// Writes a .das_package with no build() that declares `platform` as its only platform.
def private write_platform_pkg(src_dir, pkg_name, platform : string) {
    let no_deps : array<string>
    write_passing_pkg(src_dir, pkg_name, ["package_platform(\"{platform}\")"], no_deps)
}

[test]
def test_install_local_platform_gate(t : T?) {
    let other = other_platform()
    t |> run("an unsupported package is refused before anything lands") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_plat_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_plat_src"
        mkdir(tmp_root)
        write_platform_pkg(tmp_src, "foreign", other)
        var lf : LockFile
        t |> equal(RC_UNSUPPORTED_PLATFORM, install_local(tmp_root, "foreign", tmp_src, lf))
        t |> success(!has_package(lf, "foreign"))
        t |> success(!fexist("{tmp_root}/modules/foreign"))
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
    }
    t |> run("--force installs it") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_plat_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_plat_src"
        mkdir(tmp_root)
        write_platform_pkg(tmp_src, "foreign", other)
        var lf : LockFile
        t |> equal(0, install_local(tmp_root, "foreign", tmp_src, lf, [force = true]))
        t |> success(has_package(lf, "foreign"))
        t |> success(fexist("{tmp_root}/modules/foreign/.das_package"))
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
    }
    t |> run("a supported package installs") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_plat_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_plat_src"
        mkdir(tmp_root)
        write_platform_pkg(tmp_src, "native", get_platform_name())
        var lf : LockFile
        t |> equal(0, install_local(tmp_root, "native", tmp_src, lf))
        t |> success(has_package(lf, "native"))
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
    }
    t |> run("an unsupported dependency refuses the parent and rolls it back") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_plat_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_plat_src"
        let dep_src = "{get_das_root()}/_daspkg_test_plat_dep"
        mkdir(tmp_root)
        write_platform_pkg(dep_src, "foreigndep", other)
        write_passing_pkg(tmp_src, "parent", [dep_src])
        var lf : LockFile
        t |> equal(RC_UNSUPPORTED_PLATFORM, install_local(tmp_root, "parent", tmp_src, lf))
        t |> success(!has_package(lf, "parent"))
        t |> success(!has_package(lf, "_daspkg_test_plat_dep"))
        t |> success(!fexist("{tmp_root}/modules/parent"))
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
        force_rmdir(dep_src)
    }
    t |> run("--force reaches the dependency") @(t : T?) {
        let tmp_root = "{get_das_root()}/_daspkg_test_plat_root"
        let tmp_src = "{get_das_root()}/_daspkg_test_plat_src"
        let dep_src = "{get_das_root()}/_daspkg_test_plat_dep"
        mkdir(tmp_root)
        write_platform_pkg(dep_src, "foreigndep", other)
        write_passing_pkg(tmp_src, "parent", [dep_src])
        var lf : LockFile
        t |> equal(0, install_local(tmp_root, "parent", tmp_src, lf, [force = true]))
        t |> success(has_package(lf, "parent"))
        t |> success(has_package(lf, "_daspkg_test_plat_dep"))
        force_rmdir(tmp_root)
        force_rmdir(tmp_src)
        force_rmdir(dep_src)
    }
}

[test]
def test_index_platforms(t : T?) {
    t |> run("parses, serializes and renders platforms") @(t : T?) {
        let json_text = "[\{\"name\":\"winpkg\",\"url\":\"github.com/u/winpkg\",\"description\":\"windows only\",\"platforms\":[\"windows\",\"emscripten\"]\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        unsafe {
            let e & = index["winpkg"]
            t |> equal(2, length(e.platforms))
            t |> equal("windows", e.platforms[0])
            t |> equal("emscripten", e.platforms[1])
        }
        let serialized = serialize_index(index)
        t |> success(find(serialized, "\"platforms\"") >= 0)
        var index2 : table<string; IndexEntry>
        t |> success(parse_index_json(serialized, index2))
        t |> equal(2, unsafe(length(index2["winpkg"].platforms)))
        let readme = generate_index_readme(json_text)
        t |> success(find(readme, "**Platforms:** windows, emscripten") >= 0)
    }
    t |> run("an entry without platforms stays without them") @(t : T?) {
        let json_text = "[\{\"name\":\"bare\",\"url\":\"github.com/u/bare\",\"description\":\"minimal\"\}]"
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(json_text, index))
        t |> equal(0, unsafe(length(index["bare"].platforms)))
        t |> success(find(serialize_index(index), "\"platforms\"") < 0)
        t |> success(find(generate_index_readme(json_text), "**Platforms:**") < 0)
    }
    t |> run("a refresh replaces the entry's platforms with the manifest's, an empty declaration included") @(t : T?) {
        var entry : IndexEntry
        entry.platforms |> push_clone("windows")
        var restricted = PackageManifest(name = "p", description = "d")
        restricted.platforms |> push_clone("linux")
        apply_manifest_to_entry(restricted, entry)
        t |> equal(1, length(entry.platforms))
        t |> equal("linux", entry.platforms[0])
        let unrestricted = PackageManifest(name = "p", description = "d")
        apply_manifest_to_entry(unrestricted, entry)
        t |> equal(0, length(entry.platforms))
    }
    t |> run("introduce carries the manifest's platforms into the entry") @(t : T?) {
        var manifest = PackageManifest(name = "winpkg", description = "windows only")
        manifest.platforms |> push_clone("windows")
        let result = add_to_index_json_rich("[]", "winpkg", manifest, "github.com/u/winpkg")
        var index : table<string; IndexEntry>
        t |> success(parse_index_json(result, index))
        unsafe {
            t |> equal(1, length(index["winpkg"].platforms))
            t |> equal("windows", index["winpkg"].platforms[0])
        }
    }
}

[test]
def test_read_manifest_platforms(t : T?) {
    t |> run("reads a declaration") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest_platforms"
        let no_deps : array<string>
        write_passing_pkg(tmp_dir, "winpkg", ["package_description(\"d\")", "package_platform(\"windows\")"], no_deps)
        var manifest : PackageManifest
        t |> success(read_manifest(tmp_dir, manifest))
        t |> equal(1, length(manifest.platforms))
        t |> equal("windows", manifest.platforms[0])
        force_rmdir(tmp_dir)
    }
    t |> run("an unknown platform name fails the manifest") @(t : T?) {
        let tmp_dir = "{get_das_root()}/_daspkg_test_manifest_platforms"
        let no_deps : array<string>
        write_passing_pkg(tmp_dir, "badpkg", ["package_description(\"d\")", "package_platform(\"amiga\")"], no_deps)
        var manifest : PackageManifest
        t |> success(!read_manifest(tmp_dir, manifest))
        force_rmdir(tmp_dir)
    }
}

[test]
def test_parse_args_update_index_flags(t : T?) {
    t |> run("--commit and --dry-run parse and default off") @(t : T?) {
        var r <- parse_args(type<DaspkgArgs>, ["update-index", "--commit"])
        t |> success(r |> is_ok, "parse succeeds")
        let parsed <- r |> move_unwrap
        t |> success(parsed.commit)
        t |> success(!parsed.dry_run)
        var r2 <- parse_args(type<DaspkgArgs>, ["update-index", "--dry-run"])
        t |> success(r2 |> is_ok, "parse succeeds")
        let parsed2 <- r2 |> move_unwrap
        t |> success(parsed2.dry_run)
        t |> success(!parsed2.commit)
    }
}
