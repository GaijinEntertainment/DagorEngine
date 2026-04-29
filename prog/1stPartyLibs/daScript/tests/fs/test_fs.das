options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/fio
require strings

[test]
def test_extension(t : T?) {
    t |> run("basic extension") @(t : T?) {
        t |> equal(extension("foo.txt"), ".txt")
    }
    t |> run("no extension") @(t : T?) {
        t |> success(empty(extension("foo")))
    }
    t |> run("double extension") @(t : T?) {
        t |> equal(extension("archive.tar.gz"), ".gz")
    }
    t |> run("empty path") @(t : T?) {
        t |> success(empty(extension("")))
    }
    t |> run("path with directory") @(t : T?) {
        t |> equal(extension("/path/to/file.txt"), ".txt")
    }
}

[test]
def test_stem(t : T?) {
    t |> run("basic stem") @(t : T?) {
        t |> equal(stem("foo.txt"), "foo")
    }
    t |> run("no extension") @(t : T?) {
        t |> equal(stem("foo"), "foo")
    }
    t |> run("path with directory") @(t : T?) {
        t |> equal(stem("/path/to/file.txt"), "file")
    }
    t |> run("double extension") @(t : T?) {
        t |> equal(stem("archive.tar.gz"), "archive.tar")
    }
}

[test]
def test_replace_extension(t : T?) {
    t |> run("swap extension") @(t : T?) {
        t |> equal(replace_extension("foo.txt", ".md"), "foo.md")
    }
    t |> run("remove extension") @(t : T?) {
        t |> equal(replace_extension("foo.txt", ""), "foo")
    }
    t |> run("add extension to no-ext") @(t : T?) {
        t |> equal(replace_extension("foo", ".md"), "foo.md")
    }
}

[test]
def test_path_join(t : T?) {
    t |> run("two components") @(t : T?) {
        let result = path_join("foo", "bar.txt")
        t |> success(result != "")
        t |> success(find(result, "foo") >= 0)
        t |> success(find(result, "bar.txt") >= 0)
    }
    t |> run("empty first") @(t : T?) {
        t |> equal(path_join("", "bar.txt"), "bar.txt")
    }
}

[test]
def test_normalize(t : T?) {
    t |> run("remove dot-dot") @(t : T?) {
        let result = normalize("foo/bar/../baz")
        t |> success(result != "")
        t |> success(find(result, "..") < 0)
        t |> success(find(result, "baz") >= 0)
    }
    t |> run("remove dot") @(t : T?) {
        let result = normalize("foo/./bar")
        t |> success(result != "")
        t |> success(find(result, "bar") >= 0)
    }
}

[test]
def test_is_absolute(t : T?) {
    t |> run("relative path") @(t : T?) {
        t |> success(!is_absolute("foo/bar"))
    }
    t |> run("empty path") @(t : T?) {
        t |> success(!is_absolute(""))
    }
}

[test]
def test_parent(t : T?) {
    t |> run("file in directory") @(t : T?) {
        let result = parent("/foo/bar/baz.txt")
        t |> success(result != "")
        t |> success(find(result, "bar") >= 0)
    }
    t |> run("root-level file") @(t : T?) {
        let result = parent("foo")
        t |> success(empty(result))
    }
}

[test]
def test_file_size(t : T?) {
    t |> run("known file size") @(t : T?) {
        let root = get_das_root()
        let tmp = root + "/_test_fs_size.tmp"
        fopen(tmp, "wb") <| $(f) {
            fwrite(f, "12345")
        }
        var error : string
        t |> equal(file_size(tmp, error), int64(5))
        t |> success(empty(error), "unexpected error: {error}")
        remove(tmp)
    }
    t |> run("missing file") @(t : T?) {
        var error : string
        t |> equal(file_size("/no/such/file_that_exists", error), int64(-1))
        t |> success(!empty(error), "expected error for missing file")
    }
}

[test]
def test_equivalent(t : T?) {
    t |> run("same file") @(t : T?) {
        let root = get_das_root()
        let tmp = root + "/_test_fs_equiv.tmp"
        fopen(tmp, "wb") <| $(f) {
            fwrite(f, "test")
        }
        var error : string
        t |> success(equivalent(tmp, tmp, error))
        t |> success(empty(error), "unexpected error: {error}")
        remove(tmp)
    }
    t |> run("different files") @(t : T?) {
        let root = get_das_root()
        let a = root + "/_test_fs_equiv_a.tmp"
        let b = root + "/_test_fs_equiv_b.tmp"
        fopen(a, "wb") <| $(f) {
            fwrite(f, "a")
        }
        fopen(b, "wb") <| $(f) {
            fwrite(f, "b")
        }
        var error : string
        t |> success(!equivalent(a, b, error))
        t |> success(empty(error), "unexpected error: {error}")
        remove(a)
        remove(b)
    }
}

[test]
def test_is_symlink(t : T?) {
    t |> run("regular file is not symlink") @(t : T?) {
        let root = get_das_root()
        let tmp = root + "/_test_fs_symlink.tmp"
        fopen(tmp, "wb") <| $(f) {
            fwrite(f, "test")
        }
        var error : string
        t |> success(!is_symlink(tmp, error))
        t |> success(empty(error), "unexpected error: {error}")
        remove(tmp)
    }
    t |> run("missing path") @(t : T?) {
        var error : string
        t |> success(!is_symlink("/no/such/path", error))
    }
}

[test]
def test_copy_file(t : T?) {
    t |> run("copy existing file") @(t : T?) {
        let root = get_das_root()
        let src = root + "/_test_fs_copy_src.tmp"
        let dst = root + "/_test_fs_copy_dst.tmp"
        fopen(src, "wb") <| $(f) {
            fwrite(f, "hello copy")
        }
        var error : string
        t |> success(copy_file(src, dst, false, error))
        t |> success(empty(error), "unexpected error: {error}")
        t |> success(fexist(dst))
        fopen(dst, "rb") <| $(f) {
            let content = fread(f)
            t |> equal(content, "hello copy")
        }
        remove(src)
        remove(dst)
    }
    t |> run("no overwrite") @(t : T?) {
        let root = get_das_root()
        let src = root + "/_test_fs_copy_noow_src.tmp"
        let dst = root + "/_test_fs_copy_noow_dst.tmp"
        fopen(src, "wb") <| $(f) {
            fwrite(f, "src")
        }
        fopen(dst, "wb") <| $(f) {
            fwrite(f, "dst")
        }
        var error : string
        t |> success(!copy_file(src, dst, false, error))
        remove(src)
        remove(dst)
    }
    t |> run("with overwrite") @(t : T?) {
        let root = get_das_root()
        let src = root + "/_test_fs_copy_ow_src.tmp"
        let dst = root + "/_test_fs_copy_ow_dst.tmp"
        fopen(src, "wb") <| $(f) {
            fwrite(f, "new content")
        }
        fopen(dst, "wb") <| $(f) {
            fwrite(f, "old content")
        }
        var error : string
        t |> success(copy_file(src, dst, true, error))
        t |> success(empty(error), "unexpected error: {error}")
        fopen(dst, "rb") <| $(f) {
            let content = fread(f)
            t |> equal(content, "new content")
        }
        remove(src)
        remove(dst)
    }
}

[test]
def test_dir_rec(t : T?) {
    t |> run("recursive listing") @(t : T?) {
        let root = get_das_root()
        let base = root + "/_test_fs_dir_rec"
        mkdir(base)
        mkdir(base + "/sub")
        fopen(base + "/a.txt", "wb") <| $(f) {
            fwrite(f, "a")
        }
        fopen(base + "/sub/b.txt", "wb") <| $(f) {
            fwrite(f, "b")
        }
        var count = 0
        var found_sub = false
        var found_a = false
        var found_b = false
        dir_rec(base) <| $(name : string; is_dir : bool) {
            count++
            if (find(name, "sub") >= 0 && is_dir) {
                found_sub = true
            }
            if (find(name, "a.txt") >= 0 && !is_dir) {
                found_a = true
            }
            if (find(name, "b.txt") >= 0 && !is_dir) {
                found_b = true
            }
        }
        t |> success(count >= 3, "expected at least 3 entries")
        t |> success(found_sub, "expected to find sub directory")
        t |> success(found_a, "expected to find a.txt")
        t |> success(found_b, "expected to find b.txt")
        rmdir_rec(base)
    }
    t |> run("relative paths") @(t : T?) {
        let root = get_das_root()
        let base = root + "/_test_fs_dir_rec2"
        mkdir(base)
        fopen(base + "/file.txt", "wb") <| $(f) {
            fwrite(f, "x")
        }
        var has_relative = false
        dir_rec(base) <| $(name : string; is_dir : bool) {
            if (name == "file.txt") {
                has_relative = true
            }
        }
        t |> success(has_relative, "expected relative path 'file.txt'")
        rmdir_rec(base)
    }
}

[test]
def test_temp_directory(t : T?) {
    t |> run("returns non-empty path") @(t : T?) {
        var error : string
        let tmp = temp_directory(error)
        t |> success(tmp != "")
        t |> success(fexist(tmp))
        t |> success(empty(error), "unexpected error: {error}")
    }
}

[test]
def test_create_temp_file(t : T?) {
    t |> run("creates file") @(t : T?) {
        var error : string
        let path = create_temp_file("dastest", ".tmp", error)
        t |> success(path != "")
        t |> success(fexist(path))
        t |> success(find(path, "dastest") >= 0)
        t |> success(find(path, ".tmp") >= 0)
        t |> success(empty(error), "unexpected error: {error}")
        remove(path)
    }
}

[test]
def test_create_temp_directory(t : T?) {
    t |> run("creates directory") @(t : T?) {
        var error : string
        let path = create_temp_directory("dastest", error)
        t |> success(path != "")
        t |> success(fexist(path))
        t |> success(find(path, "dastest") >= 0)
        t |> success(empty(error), "unexpected error: {error}")
        rmdir(path)
    }
}

[test]
def test_disk_space(t : T?) {
    t |> run("current directory") @(t : T?) {
        var info = DiskSpaceInfo()
        var error : string
        t |> success(builtin_disk_space(".", info, error))
        t |> success(info.capacity > uint64(0))
        t |> success(info.available > uint64(0))
        t |> success(empty(error), "unexpected error: {error}")
    }
    t |> run("convenience overload") @(t : T?) {
        let info = disk_space(".")
        t |> success(info.capacity > uint64(0))
    }
}

[test]
def test_set_mtime(t : T?) {
    t |> run("set modification time") @(t : T?) {
        let root = get_das_root()
        let tmp = root + "/_test_fs_mtime.tmp"
        fopen(tmp, "wb") <| $(f) {
            fwrite(f, "test")
        }
        let target_time = mktime(2020, 1, 1, 0, 0, 0)
        var error : string
        t |> success(set_mtime(tmp, target_time, error))
        t |> success(empty(error), "unexpected error: {error}")
        let st = stat(tmp)
        t |> success(st.is_valid)
        let diff = int64(st.mtime) - int64(target_time)
        let abs_diff = diff >= int64(0) ? diff : -diff
        t |> success(abs_diff < int64(60), "mtime should be close to target")
        remove(tmp)
    }
}

[test]
def test_relative(t : T?) {
    t |> run("compute relative path") @(t : T?) {
        let root = get_das_root()
        let sub = path_join(root, "foo")
        var error : string
        let rel = relative(sub, root, error)
        t |> success(rel != "")
        t |> equal(rel, "foo")
        t |> success(empty(error), "unexpected error: {error}")
    }
}
