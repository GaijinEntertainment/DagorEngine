options gen2
options rtti

module test_rtti_init_mnh shared public

require dastest/testing_boost public
require daslib/rtti

struct public RttiInitProbe {
    value : int = 7
}

[test]
def test_named_module_structure_init_mnh(t : T?) {
    let initializer = @@RttiInitProbe
    t |> success(initializer != null)

    var initializerHash : uint64
    context_for_each_function() $(info : FuncInfo) {
        if (info.name == "RttiInitProbe" && info.count == 0u) {
            initializerHash = info.hash
        }
    }
    t |> success(initializerHash != 0ul)

    let typeInfo = typeinfo rtti_typeinfo(type<RttiInitProbe>)
    unsafe {
        t |> equal(typeInfo.structType.init_mnh, initializerHash)
    }
}
