options gen2
require dastest/testing_boost public

require daslib/fio

[test]
def test_get_das_version(t : T?) {
    t |> run("returns non-empty version string") @(t : T?) {
        let ver = get_das_version()
        t |> success(!empty(ver), "version string should not be empty")
    }
    t |> run("version format is major.minor.patch") @(t : T?) {
        let ver = get_das_version()
        var dots = 0
        for (ch in ver) {
            if (ch == '.') {
                dots++
            }
        }
        t |> equal(dots, 2)
    }
}

[test]
def test_fexist(t : T?) {
    t |> run("existing file returns true") @(t : T?) {
        // this test file itself must exist
        t |> success(fexist("{get_das_root()}/tests/fio/fio_utils.das"))
    }
    t |> run("non-existing file returns false") @(t : T?) {
        t |> success(!fexist("{get_das_root()}/tests/fio/_no_such_file_12345.das"))
    }
    t |> run("existing directory returns true") @(t : T?) {
        t |> success(fexist("{get_das_root()}/tests/fio"))
    }
}

[test]
def test_fread_path(t : T?) {
    t |> run("read file by path") @(t : T?) {
        let tmp = "{get_das_root()}/_test_fread_path.tmp"
        fopen(tmp, "wb") <| $(f) {
            fwrite(f, "hello world")
        }
        let content = fread(tmp)
        t |> equal(content, "hello world")
        remove(tmp)
    }
    t |> run("read non-existing file returns empty string") @(t : T?) {
        let content = fread("{get_das_root()}/_no_such_file_12345.tmp")
        t |> success(empty(content), "expected empty string for missing file")
    }
}

[test]
def test_fwrite_path(t : T?) {
    t |> run("write file by path") @(t : T?) {
        let tmp = "{get_das_root()}/_test_fwrite_path.tmp"
        let ok = fwrite(tmp, "test content")
        t |> success(ok, "fwrite should return true")
        t |> success(fexist(tmp), "file should exist after fwrite")
        let content = fread(tmp)
        t |> equal(content, "test content")
        remove(tmp)
    }
}

[test]
def test_rmdir(t : T?) {
    t |> run("remove empty directory") @(t : T?) {
        let tmp = "{get_das_root()}/_test_rmdir_empty"
        mkdir(tmp)
        t |> success(fexist(tmp), "directory should exist after mkdir")
        t |> success(rmdir(tmp), "rmdir should succeed on empty dir")
        t |> success(!fexist(tmp), "directory should not exist after rmdir")
    }
}

[test]
def test_rmdir_rec(t : T?) {
    t |> run("remove directory tree") @(t : T?) {
        let base = "{get_das_root()}/_test_rmdir_rec"
        let sub = "{base}/sub"
        mkdir(base)
        mkdir(sub)
        fwrite("{base}/a.txt", "aaa")
        fwrite("{sub}/b.txt", "bbb")
        t |> success(fexist("{sub}/b.txt"), "nested file should exist")
        t |> success(rmdir_rec(base), "rmdir_rec should succeed")
        t |> success(!fexist(base), "directory tree should be gone")
    }
}
