options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require dastest/testing_boost public
require daslib/ast
require daslib/fio
require strings
require daslib/strings_boost
require ../patch.das
require ../convert.das

//! End-to-end: read a v1 file, parse it with infer off, apply the edit set, and hand back text
//! whose comments are provably intact.

let FIXTURE = "{get_das_root()}/utils/gen1-to-gen2/tests/_fixture_gen1_basic.das"


let STMT_FIXTURE = "{get_das_root()}/utils/gen1-to-gen2/tests/_fixture_gen1_stmt.das"


//! gen1.5 keeps the indentation, so its output has to stay v1 - the gen2 twin of this check
//! (`compiles_as_gen2`) would reject it for the braces it deliberately did not add.
def private compiles_as_gen1(near : string; text : string; var issues_out : string&) : bool {
    let probe = "{dir_name(near)}/.gen15_verify_{base_name(near)}"
    if (!fwrite(probe, text)) {
        issues_out = "could not write {probe}"
        return false
    }
    var ok_out = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = false
            cop.no_optimizations = true
            compile_file(probe, access, unsafe(addr(mg)), cop) $(ok; _program; issues) {
                ok_out = ok
                issues_out = string(issues)
            }
        }
    }
    remove(probe)
    return ok_out
}


[test]
def test_convert_pipeline(t : T?) {
    t |> run("a v1 file gains the gen2 header and keeps every comment") @(t : T?) {
        let res <- convert_file(FIXTURE, ConvertOptions(v2 = true))
        t |> equal(res.failed, "", "no failure")
        t |> equal(res.skipped, "", "not skipped")
        t |> equal(length(res.refused), 0, "no refused edits")
        t |> success(res.changed, "the file changed")
        t |> success(res.output |> starts_with("options gen2\n"), "the opt-out marker became the header")
        let original = fread(FIXTURE)
        t |> success(res.output |> find("var s = Foo( /*keep*/ a=1)") >= 0, "make-syntax converted")
        t |> success(res.output |> find("// a header comment") >= 0, "prose comment untouched")
        t |> success(check_comments_preserved(original, res.output).ok, "comments conserved")
    }

    t |> run("--semicolon terminates the header too") @(t : T?) {
        let res <- convert_file(FIXTURE, ConvertOptions(v2 = true, semicolon = true))
        t |> success(res.output |> starts_with("options gen2;\n"), "header carries the ';'")
    }

    t |> run("gen1.5 converts make-syntax but leaves the marker alone") @(t : T?) {
        let res <- convert_file(FIXTURE, ConvertOptions(v2 = false))
        t |> success(res.output |> starts_with("options gen2 = false"), "still opted out of gen2")
        t |> success(res.output |> find("var s = Foo( /*keep*/ a=1)") >= 0, "make-syntax still converted")
    }

    t |> run("gen1.5 output still compiles as v1") @(t : T?) {
        for (f in [FIXTURE, STMT_FIXTURE]) {
            let res <- convert_file(f, ConvertOptions(v2 = false))
            t |> equal(res.failed, "", "{f} converted")
            var issues = ""
            t |> success(compiles_as_gen1(f, res.output, issues), "compiles as v1: {issues}")
        }
    }

    t |> run("an already-gen2 file is left alone byte for byte") @(t : T?) {
        let self = "{get_das_root()}/utils/gen1-to-gen2/tests/test_convert.das"
        let res <- convert_file(self, ConvertOptions(v2 = true))
        t |> equal(res.skipped, "already gen2", "skipped as already converted")
        t |> success(!res.changed, "unchanged")
        t |> equal(res.output, fread(self), "output is the input")
    }
}


[test]
def test_crlf_input(t : T?) {
    t |> run("a CRLF file converts to the LF result with CRLF endings") @(t : T?) {
        for (f in [FIXTURE, STMT_FIXTURE]) {
            let lf <- convert_file(f, ConvertOptions(v2 = true))
            let crlfPath = "{dir_name(f)}/.crlf_{base_name(f)}"
            let crlfText = fread(f) |> replace("\r\n", "\n") |> replace("\n", "\r\n")
            t |> success(fwrite(crlfPath, crlfText), "wrote the CRLF twin of {f}")
            let crlf <- convert_file(crlfPath, ConvertOptions(v2 = true))
            remove(crlfPath)
            t |> equal(crlf.failed, "", "{f} converted")
            t |> equal(length(crlf.refused), 0, "no refused edits")
            t |> success(check_comments_preserved(crlfText, crlf.output).ok, "comments conserved")
            t |> equal(crlf.output, lf.output |> replace("\n", "\r\n"), "same conversion, CRLF endings")
        }
    }
}
