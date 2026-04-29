options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct MorPipeline {
    a : int
}

def operator delete(var p : MorPipeline) {
    assert(p.a == 2)
    p.a = 0
}

def mor_create_pipelines() : array<MorPipeline?> {
    return <- [new MorPipeline(a = 1), new MorPipeline(a = 2)]
}

def mor_fooBar() : MorPipeline? {
    var pipelines <- mor_create_pipelines()
    return <- pipelines[0]
} finally {
    unsafe {
        delete pipelines
    }
}

[test]
def test_move_on_return(t : T?) {
    t |> run("move on return with finally delete") @(t : T?) {
        var x = mor_fooBar()
        t |> equal(x.a, 1)
    }
}
