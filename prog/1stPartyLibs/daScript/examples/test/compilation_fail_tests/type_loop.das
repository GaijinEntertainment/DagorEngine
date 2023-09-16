options max_infer_passes=5

expect 41000

def infinite(a:auto(ptrt)?)
    let t : ptrt ? ?
    infinite(t)

struct Foo
    bar:int

def test
    infinite(new Foo)
    return true

