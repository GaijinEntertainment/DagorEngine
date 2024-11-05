require daslib/hash_map

struct ComplexKey
    key : int
    key2 : int
    nocopy : array<int>

def ComplexKey ( k1,k2 : int )
    return <- [[ComplexKey key=k1,key2=k2]]

def operator == ( a,b : ComplexKey )
    return a.key == b.key && a.key2 == b.key2

struct SomeValue
    value : int
    value2 : int
    nocopy : array<int>

def SomeValue ( v1,v2 : int )
    return <- [[SomeValue value=v1,value2=v2]]

%spoof_instance~TFlatHashMap(ExampleHashMap,ComplexKey,SomeValue)%%;

def hash0 ( k : ComplexKey )
    return uint64(k.key) | (uint64(k.key2) << 32ul)

[export]
def main
    var hm <- ExampleHashMap()
    hm[ComplexKey(1,2)] <- SomeValue(3,4)
    hm[ComplexKey(2,3)] <- SomeValue(4,5)
    hm[ComplexKey(3,4)] <- SomeValue(5,6)
    hm[ComplexKey(4,5)] <- SomeValue(6,7)

    print("foreach:\n")
    hm |> foreach() <| $ ( k,v )
        print("{k} -> {v}\n")
    print("keys,values:\n")
    for k,v in keys(hm), values(hm)
        print("{k} -> {v}\n")

    hm |> erase(ComplexKey(2,3))
    print("after erase:\n")
    for k,v in keys(hm), values(hm)
        print("{k} -> {v}\n")

    let found = hm |> get(ComplexKey(3,4)) <| $ ( v )
        print("found: {v}\n")
    print("found: {found}\n")

    let notfound = hm |> get(ComplexKey(2,3)) <| $ ( v )
        print("notfound: {v}\n")
    print("notfound: {notfound}\n")

    let exists = hm |> key_exists(ComplexKey(3,4))
    print("exists: {exists}\n")

    let notexists = hm |> key_exists(ComplexKey(2,3))
    print("notexists: {notexists}\n")

    hm[ComplexKey(3,4)] <- SomeValue(7,8)
    print("after update:\n")
    for k,v in keys(hm), values(hm)
        print("{k} -> {v}\n")

    let safe_at_found <- hm?[ComplexKey(3,4)] ?? SomeValue(0,0)
    print("safe_at_found: {safe_at_found}\n")

    let safe_at_notfound <- hm?[ComplexKey(2,3)] ?? SomeValue(0,0)
    print("safe_at_notfound: {safe_at_notfound}\n")


