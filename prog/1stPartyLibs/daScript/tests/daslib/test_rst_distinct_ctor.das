options gen2
options rtti
options no_aot
options no_coverage

require dastest/testing_boost public
require daslib/rtti
require daslib/ast
require daslib/rst
require daslib/fio
require daslib/strings_boost
require strings
require _rst_probe
require _rst_distinct_mod
require _rst_pageless_mod

[test]
def test_rst_distinct_types(t : T?) {
    let mods : array<Module?> = [find_module("_rst_distinct_mod")]
    let docs <- document_into_temp(t, mods, DocsHook())
    t |> run("a public distinct type is declared with its underlying type") @(t : T?) {
        t |> success(find(docs.page, ".. _handle-_rst_distinct_mod-RstMeters:") >= 0, "no label: {docs.page}")
        t |> success(find(docs.page, ".. das:attribute:: RstMeters = distinct float") >= 0, "no declaration: {docs.page}")
    }
    t |> run("a distinct type is not a structure macro") @(t : T?) {
        t |> success(find(docs.page, "Structure macros") < 0, "listed as a structure macro: {docs.page}")
    }
    t |> run("a private distinct type gets no entry") @(t : T?) {
        t |> success(find(docs.page, "_handle-_rst_distinct_mod-RstHidden") < 0, "private distinct type labeled: {docs.page}")
        t |> success(find(docs.page, "RstHidden = distinct") < 0, "private distinct type declared: {docs.page}")
    }
    t |> run("a signature naming a private distinct type prints it without a link") @(t : T?) {
        t |> success(find(docs.page, "**h** : RstHidden") >= 0, "no plain argument type: {docs.page}")
        t |> success(find(docs.page, "RstHidden <handle-") < 0, "links a label no page writes: {docs.page}")
    }
    t |> run("a signature names the distinct type and links it") @(t : T?) {
        t |> success(find(docs.page, "RstMeters <handle-_rst_distinct_mod-RstMeters>") >= 0, "no link: {docs.page}")
        t |> success(find(docs.page, "**m** : distinct") < 0, "argument printed as its base type: {docs.page}")
    }
    t |> run("the distinct type reads its description from the typedef topic") @(t : T?) {
        t |> success(find(docs.stubs, "|detail/typedef-_rst_distinct_mod-RstMeters|") >= 0, "no typedef stub: {docs.stubs}")
    }
    drop_run(docs)
}

[test]
def test_rst_distinct_type_filtered_by_hook(t : T?) {
    let mods : array<Module?> = [find_module("_rst_distinct_mod")]
    var hook = DocsHook()
    hook.annotationFilter <- @(ann : rtti::Annotation const) : bool {
        return ann.name != "RstMeters"
    }
    let docs <- document_into_temp(t, mods, hook)
    t |> run("a filtered distinct type gets no entry and no group") @(t : T?) {
        t |> success(find(docs.page, "Distinct types") < 0, "empty group written: {docs.page}")
        t |> success(find(docs.page, "_handle-_rst_distinct_mod-RstMeters:") < 0, "filtered type labeled: {docs.page}")
    }
    t |> run("a signature naming a filtered type prints it without a link") @(t : T?) {
        t |> success(find(docs.page, "**m** : RstMeters") >= 0, "no plain argument type: {docs.page}")
        t |> success(find(docs.page, "RstMeters <handle-") < 0, "links a label no page writes: {docs.page}")
    }
    drop_run(docs)
}

[test]
def test_rst_distinct_type_in_a_pageless_module(t : T?) {
    mark_module_pageless("_rst_pageless_mod")
    let mods : array<Module?> = [find_module("_rst_pageless_mod")]
    let docs <- document_into_temp(t, mods, DocsHook())
    t |> run("a signature naming a pageless module's distinct type prints it without a link") @(t : T?) {
        t |> success(find(docs.page, "**i** : RstPagelessId") >= 0, "no plain argument type: {docs.page}")
        t |> success(find(docs.page, "RstPagelessId <handle-") < 0, "links into a module with no page: {docs.page}")
    }
    drop_run(docs)
}

[test]
def test_rst_written_constructors(t : T?) {
    let mods : array<Module?> = [find_module("_rst_distinct_mod")]
    let docs <- document_into_temp(t, mods, DocsHook())
    t |> run("each constructor the class declares is documented") @(t : T?) {
        t |> equal(count(docs.page, ".. das:function:: RstCamera("), 2, docs.page)
    }
    t |> run("a struct's written constructor is documented") @(t : T?) {
        t |> equal(count(docs.page, ".. das:function:: RstPoint("), 1, docs.page)
    }
    t |> run("a class with only the default constructor documents none") @(t : T?) {
        t |> success(find(docs.page, ".. das:function:: RstPlain(") < 0, "default constructor documented: {docs.page}")
    }
    drop_run(docs)
}

[test]
def test_distinct_accessors_reject_other_annotations(t : T?) {
    var checked = false
    module_for_each_annotation(get_module("rtti_core")) $(value) {
        return if (checked || value.isDistinctTypeAnnotation)
        checked = true
        var threw = false
        try {
            feint("{get_distinct_underlying_type(value) != null}")
        } recover {
            threw = true
        }
        t |> success(threw, "get_distinct_underlying_type accepted {value.name}, which is not a distinct type")
    }
    t |> success(checked, "rtti_core carries no annotation to try")
}

[test]
def test_rst_comment_reads_distinct_typedef(t : T?) {
    var err : string
    let root = create_temp_directory("das_rst_comment", err)
    t |> success(empty(err), "temp directory: {err}")
    let exe = get_command_line_arguments()[0]
    let driver = "{get_das_root()}/tests/daslib/_rst_comment_driver.das"
    var output : string
    let exit_code = run_and_capture([exe, "-no-module-cache", "-dasroot", get_das_root(), driver, "--", "--detail_output", root], output)
    t |> run("the trailing //! of a distinct typedef reaches its typedef topic") @(t : T?) {
        t |> equal(exit_code, 0, output)
        let detail = fread("{root}/detail/typedef-_rst_distinct_mod-RstMeters.rst")
        t |> success(find(detail, "Distance along a path") >= 0, "detail: '{detail}' output: {output}")
    }
    rmdir_rec(root, err)
}
