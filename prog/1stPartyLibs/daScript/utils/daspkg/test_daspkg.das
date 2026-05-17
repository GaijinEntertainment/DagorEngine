options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false

require dastest/testing_boost public

require daslib/fio
require strings

require daslib/json
require daslib/json_boost

require daslib/ansi_colors

require lockfile
require utils
require index
require commands
require package_runner public

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
def test_parse_args(t : T?) {
    t |> run("install command") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["install", "dasImgui"]), "")
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "dasImgui")
        t |> success(!parsed.force)
    }
    t |> run("install with force") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["install", "dasImgui", "--force"]), "")
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "dasImgui")
        t |> success(parsed.force)
    }
    t |> run("install with root") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["--root", "/tmp/proj", "install", "foo"]), "")
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "foo")
        t |> equal(parsed.root, "/tmp/proj")
    }
    t |> run("list command no arg") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["list"]), "")
        t |> equal(parsed.command ?? "", "list")
        t |> success(parsed.command_arg |> is_none)
    }
    t |> run("no command") @(t : T?) {
        var parsed = DaspkgArgs()
        let empty_args : array<string>
        t |> equal(parse_args(parsed, empty_args), "")
        t |> success(parsed.command |> is_none)
    }
    t |> run("cleanup command") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["cleanup"]), "")
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(!parsed.force)
    }
    t |> run("cleanup with force") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["cleanup", "--force"]), "")
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(parsed.force)
    }
    t |> run("cleanup with global and force") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["cleanup", "--global", "--force"]), "")
        t |> equal(parsed.command ?? "", "cleanup")
        t |> success(parsed.force)
        t |> success(parsed.global_flag)
    }
    t |> run("default root is dot") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["list"]), "")
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
    mkdir(src_dir)
    let manifest = build_string() $(var w) {
        w |> write("options gen2\nrequire daslib/daspkg\n[export]\ndef package() \{\n    package_name(\"")
        w |> write(pkg_name)
        w |> write("\")\n\}\n")
        if (length(deps) > 0) {
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
        unsafe {
            t |> equal(length(js.value as _array), 0)
        }
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
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["list", "--color"]), "")
        t |> success(parsed.color, "color flag parsed")
        apply_global_flags(parsed)
        t |> success(use_tty_colors)
        use_tty_colors = false
    }
    t |> run("--no-color clears use_tty_colors") @(t : T?) {
        use_tty_colors = true
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["list", "--no-color"]), "")
        t |> success(parsed.no_color, "no_color flag parsed")
        apply_global_flags(parsed)
        t |> success(!use_tty_colors)
    }
    t |> run("--color and --no-color together is mutex error") @(t : T?) {
        var parsed = DaspkgArgs()
        let err = parse_args(parsed, ["list", "--color", "--no-color"])
        t |> success(!empty(err), "mutex group rejects both")
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
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["install", "foo", "--global"]), "")
        t |> equal(parsed.command ?? "", "install")
        t |> equal(parsed.command_arg ?? "", "foo")
        t |> success(parsed.global_flag)
    }
    t |> run("-g shorthand") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["-g", "list"]), "")
        t |> equal(parsed.command ?? "", "list")
        t |> success(parsed.global_flag)
    }
    t |> run("default global is false") @(t : T?) {
        var parsed = DaspkgArgs()
        t |> equal(parse_args(parsed, ["install", "foo"]), "")
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

