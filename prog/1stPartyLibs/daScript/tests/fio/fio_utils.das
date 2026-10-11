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
        fopen(tmp, "wb") $(f) {
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
    t |> run("UTF-8 Windows path round-trips through file IO") @(t : T?) {
        let tmp = "{get_das_root()}/_test_unicode-Привет.tmp"
        let content = "Unicode path contents"
        t |> success(fwrite(tmp, content), "fwrite should accept a UTF-8 path")
        t |> success(fexist(tmp), "fexist should accept a UTF-8 path")
        t |> equal(fread(tmp), content)
        t |> success(remove(tmp), "remove should accept a UTF-8 path")
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

[test]
def test_tilde_home(t : T?) {
    t |> run("a macOS home anywhere in a receipt becomes ~") @(t : T?) {
        t |> equal(tilde_home("-m /Users/boris/Work/models/x.gguf"), "-m ~/Work/models/x.gguf")
        t |> equal(tilde_home("A=/Users/ci/m B=/home/ci/x/y"), "A=~/m B=~/x/y", "every occurrence, both unix forms")
        t |> equal(tilde_home("\"/Users/ci/bin/tool\" -m \"/Users/ci/m/q.gguf\""), "\"~/bin/tool\" -m \"~/m/q.gguf\"", "a quote opens a path")
        t |> equal(tilde_home("(/Users/ci/a),/Users/ci/b;/Users/ci/c:/Users/ci/d ['/Users/ci/e']"), "(~/a),~/b;~/c:~/d ['~/e']", "a bracket, comma, semicolon, colon and single quote open a path")
        t |> equal(tilde_home("x/Users/ci/a"), "x/Users/ci/a", "mid-word is not a path start")
    }
    t |> run("a Windows home keeps its separator") @(t : T?) {
        t |> equal(tilde_home("C:\\Users\\boris\\Work\\x.exe"), "~\\Work\\x.exe")
        t |> equal(tilde_home("C:/Users/boris/Work/x.exe"), "~/Work/x.exe", "a Windows argv normalized to forward slashes")
    }
    t |> run("what is not a home directory is left alone") @(t : T?) {
        t |> equal(tilde_home("/Users/bob"), "/Users/bob", "a bare home with nothing after the name")
        t |> equal(tilde_home("https://x/Users/bob/y"), "https://x/Users/bob/y", "mid-URL is not a path start")
        t |> equal(tilde_home("plain text"), "plain text")
        t |> equal(tilde_home(""), "")
    }
}
