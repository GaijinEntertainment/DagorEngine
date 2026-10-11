options gen2
require dastest/testing_boost public
require daslib/fio

// `get_executable_path` names the binary running this test - daslang, dastest or an AOT test
// host - as an absolute path to a regular file, which is what a standalone program locates its
// shipped files from

[test]
def test_executable_path(t : T?) {
    t |> run("the running binary: absolute, a regular file") @(t : T?) {
        let exe = get_executable_path()
        t |> success(!empty(exe), "the platform names it")
        t |> success(is_absolute(exe), "absolute: {exe}")
        let st = stat(exe)
        t |> success(st.is_valid && st.is_reg, "a regular file: {exe}")
    }
}
