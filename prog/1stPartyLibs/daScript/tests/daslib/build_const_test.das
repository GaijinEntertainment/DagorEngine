options gen2
options no_aot   // build_const folds from the COMPILE-time environment — an AOT artifact bakes the builder's env and desyncs at run by design

require dastest/testing_boost public
require daslib/fio
require daslib/command_line

//! End-to-end tests for daslib/build_const: spawn the fixture with controlled argv and
//! environment, read back the values its compile folded. The GATE field proves the
//! static_if selection followed the constant. Environment cases rely on child processes
//! inheriting this process's environment; set-but-empty counts as unset by contract, so
//! cleanup sets "" instead of needing a true unset.

def private run_fixture(t : T?; tmp_dir, tag, extra_args : string; expect_ok : bool = true) : string {
    let fixture = path_join(get_das_root(), "tests/daslib/_fixture_build_const.das")
    let out_path = path_join(tmp_dir, "{tag}.txt")
    // exe/fixture unquoted: a leading quote breaks Windows _popen (cmd /c quote stripping)
    let cmd = "{get_das_exe()} -dasroot {get_das_root()} {fixture} -- {extra_args} --out {out_path} 2>&1"
    var child_output : string
    let rc = unsafe(popen(cmd) $(f) {
        if (f != null) {
            child_output = fread(f)
        }
    })
    feint("{child_output}")
    if (expect_ok) {
        t |> equal(0, rc)
    } else {
        t |> success(rc != 0, "expected the child compile to fail")
    }
    var content = ""
    fopen(out_path, "rb") $(f) {
        if (f != null) {
            content = fread(f)
        }
    }
    return content
}

def private clear_bct_env() {
    // "" counts as unset for every accessor (the module's set-but-empty contract)
    for (name in ["BCT_PROF", "BCT_NAME", "BCT_LEVEL", "BCT_EFLAG", "BCT_EVAL", "BCT_EINT"]) {
        set_env_variable(name, "")
    }
}

[test]
def test_build_const(t : T?) {
    var tmp_err : string
    let tmp_dir = create_temp_directory("bct", tmp_err)
    if (!empty(tmp_err)) {
        t |> failure("cannot create temp directory: {tmp_err}")
        return
    }
    clear_bct_env()

    t |> run("defaults with no carriers") @(t : T?) {
        let got = run_fixture(t, tmp_dir, "defaults", "")
        t |> equal("PROF=false;NAME=fallback;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=off", got)
    }

    t |> run("argv carriers, both spellings") @(t : T?) {
        let got = run_fixture(t, tmp_dir, "argv", "--bct-prof --bct-name cli --bct-level=3 --bct-has --bct-aval=av --bct-aint 5")
        t |> equal("PROF=true;NAME=cli;LEVEL=3;HAS=true;AVAL=av;AINT=5;EFLAG=false;EVAL=edef;EINT=2;GATE=on", got)
    }

    t |> run("flag accepts the =value spelling with the shared bool rule") @(t : T?) {
        let got_on = run_fixture(t, tmp_dir, "flageq_on", "--bct-prof=1")
        t |> equal("PROF=true;NAME=fallback;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=on", got_on)
        let got_off = run_fixture(t, tmp_dir, "flageq_off", "--bct-prof=0")
        t |> equal("PROF=false;NAME=fallback;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=off", got_off)
    }

    t |> run("last occurrence wins") @(t : T?) {
        let got = run_fixture(t, tmp_dir, "lastwins", "--bct-name first --bct-name=second")
        t |> equal("PROF=false;NAME=second;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=off", got)
    }

    t |> run("env carriers") @(t : T?) {
        set_env_variable("BCT_PROF", "yes")
        set_env_variable("BCT_NAME", "envn")
        set_env_variable("BCT_LEVEL", "9")
        set_env_variable("BCT_EFLAG", "1")
        set_env_variable("BCT_EVAL", "ev")
        set_env_variable("BCT_EINT", "4")
        let got = run_fixture(t, tmp_dir, "env", "")
        t |> equal("PROF=true;NAME=envn;LEVEL=9;HAS=false;AVAL=adef;AINT=1;EFLAG=true;EVAL=ev;EINT=4;GATE=on", got)
        clear_bct_env()
    }

    t |> run("argv beats env") @(t : T?) {
        set_env_variable("BCT_PROF", "0")
        set_env_variable("BCT_NAME", "envn")
        set_env_variable("BCT_LEVEL", "9")
        let got = run_fixture(t, tmp_dir, "precedence", "--bct-prof --bct-name cli --bct-level 3")
        t |> equal("PROF=true;NAME=cli;LEVEL=3;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=on", got)
        clear_bct_env()
    }

    t |> run("env false spellings") @(t : T?) {
        set_env_variable("BCT_PROF", "off")
        set_env_variable("BCT_EFLAG", "No")
        let got = run_fixture(t, tmp_dir, "envfalse", "")
        t |> equal("PROF=false;NAME=fallback;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=off", got)
        clear_bct_env()
    }

    t |> run("set-but-empty env counts as unset") @(t : T?) {
        clear_bct_env()
        let got = run_fixture(t, tmp_dir, "emptyenv", "")
        t |> equal("PROF=false;NAME=fallback;LEVEL=7;HAS=false;AVAL=adef;AINT=1;EFLAG=false;EVAL=edef;EINT=2;GATE=off", got)
    }

    t |> run("garbage int is a compile error") @(t : T?) {
        let got = run_fixture(t, tmp_dir, "garbage", "--bct-aint xyz", false)
        t |> equal("", got)
    }

    // cleanup: out files + the temp dir itself
    for (tag in ["defaults", "argv", "lastwins", "env", "precedence", "envfalse", "emptyenv", "garbage"]) {
        remove(path_join(tmp_dir, "{tag}.txt"))
    }
    rmdir(tmp_dir)
}
