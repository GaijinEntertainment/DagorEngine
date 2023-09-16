// options log=true, log_nodes=true, log_stack=true

def addValue(a,blk)
    return invoke(blk,a)

[export]
def test
    var CNT = 0
    let counter <- @ <| (extra:int) : int
        let e1 = addValue(1) <| $ (extraBlock:int) : int
            return extraBlock + CNT
        return CNT++ + e1
    verify(invoke(counter,0)==1)
    return true

