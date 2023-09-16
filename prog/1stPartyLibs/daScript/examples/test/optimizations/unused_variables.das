//options log_optimization_passes=true
//options log=true
//options optimize=false,log=true

[export]
def test:bool
    if true
        let i:int
        assert(true)
    if true
        let i,j:int
        assert(j==0)
    if true
        var a1,a2:int[20]
        for v1,v2 in a1,a2
            v1 = 1              // only this loop and a1 stays
    if true
        let a1,a2:int[20]
        for v1,v2 in a1,a2
            assert(true)            // nothing. note, assumed to have no side-effects and will be optimized out
    return true

