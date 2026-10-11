options gen2
options rtti
options no_aot
options no_coverage

require dastest/testing_boost public
require daslib/rtti
require daslib/ast
require daslib/rst
require daslib/enum_trait
require strings
require _rst_probe

[test]
def test_rst_enumeration_annotation_hook(t : T?) {
    let mods : array<Module?> = [find_module("enum_trait")]
    let unfiltered <- document_into_temp(t, mods, DocsHook())
    t |> success(find(unfiltered.page, "Handled enumerations") >= 0, "fixture module has no enumeration annotation")
    drop_run(unfiltered)
    var hook = DocsHook()
    hook.annotationFilter <- @(ann : rtti::Annotation const) : bool {
        return false
    }
    let filtered <- document_into_temp(t, mods, hook)
    t |> run("annotationFilter reaches the enumeration annotations too") @(t : T?) {
        t |> success(find(filtered.page, "Handled enumerations") < 0, "filtered-out annotations still documented")
    }
    drop_run(filtered)
}
