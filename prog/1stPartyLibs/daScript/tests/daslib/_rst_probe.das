options gen2
options rtti
options no_aot

module _rst_probe shared private

require dastest/testing_boost public
require daslib/fio
require daslib/rtti
require daslib/rst public

struct public DocRun {
    root : string
    page : string
    stubs : string
}

def public document_into_temp(t : T?; mods : array<Module?>; hook : DocsHook) : DocRun {
    //! Runs the RST generator over `mods` into a throwaway tree and returns the page
    //! plus its substitution-stub file. Call `drop_run` on the result.
    var err : string
    let root = create_temp_directory("das_rst_probe", err)
    t |> success(empty(err), "temp directory: {err}")
    let saved_topic = topic_root
    let saved_handmade = handmade_root
    topic_root = "{root}/generated"
    handmade_root = "{root}/handmade"
    var groups : array<DocGroup>
    documents("Probe", mods, "probe.rst", groups, hook)
    let made = DocRun(root = root,
        page = fread("{topic_root}/probe.rst"),
        stubs = fread("{topic_root}/detail/probe.rst"))
    topic_root = saved_topic
    handmade_root = saved_handmade
    return made
}

def public drop_run(made : DocRun) {
    //! Removes the throwaway tree a `document_into_temp` run left behind.
    var err : string
    rmdir_rec(made.root, err)
}
