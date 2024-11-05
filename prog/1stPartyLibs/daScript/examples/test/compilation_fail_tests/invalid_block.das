expect 30801, 30304:2, 32102, 32101, 30102  // invalid_block

def take_any(b:block)
    invoke(b)

def take_int(b:block<int>):int
    return invoke(b)

def test:bool
    if true
        var a:int
        var b:float
        take_any(${ a++; })
        while true
            take_any() <|
                break               // 30801, captured block can't break outside the block
        while true
            take_any() <|
                while true
                    break           // this one ok
        take_int(${ a++; })          // 30304: no matching function (take_int block<void>)
        take_int(${ return a++; })   // this is ok
        take_int(${ b++; })          // 30304: no matching function ( block<float> )
    take_any() <| $ : void          // 32102: not expecting return value
        return 13
    take_any() <| $ : int           // 32101: expecting return value
        return
    take_any() <| $ : int           // 30102: incompatible return type, expecting int vs float
        return 13.0
    return true
