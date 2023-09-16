[export]
def test
    unsafe
        var arr : uint[10]
        var pdata : void? = addr(arr[0])
        let data_size = 10*typeinfo(sizeof type<uint>)
        map_to_array(pdata, data_size) <| $ ( var foo : array<float># )
            for x in range(0,10)
                foo[x] = float(x)
        for x in range(0,10)
            let t = reinterpret<float>(arr[x])
            assert(float(x)==t)
    return true
