struct Pipeline
    a : int

def operator delete ( var p : Pipeline )
    assert(p.a==2)
    p.a = 0

def create_graphics_pipelines
    return <- [{auto new [[Pipeline a=1]]; new [[Pipeline a=2]] }]

def fooBar
    var pipelines <- create_graphics_pipelines()
    return <- pipelines[0]
finally
    unsafe
        delete pipelines

[export]
def test
    var x = fooBar()
    assert(x.a==1)
    return true
