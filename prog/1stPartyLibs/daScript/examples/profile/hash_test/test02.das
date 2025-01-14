options persistent_heap

require _framework
require daslib/hash_map

[sideeffects]
def test ( hmap : auto(HashMapType); dummy )
    var inscope hashMap : HashMapType
    static_if !typeinfo(is_table type<HashMapType>)
        hashMap <- HashMapType()
    for i in range(25)
        hashMap |> clear()
        for j in range(600000)
            unsafe(hashMap[j]) = -j

%spoof_instance~TCuckooHashMap(CuckooHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test_inline,int,int,hash_inline)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test0,int,int,hash0)%%;

def hash_inline ( i : int ) : uint64
    let DAS_WYHASH_SEED = 0x1234567890abcdeful
    let u = uint64(i)
    let ab = mul128(u, DAS_WYHASH_SEED)
    let h = ab.x ^ ab.y
    return h <= 1ul ? 1099511628211ul : h

[export]
def main
    print("test02 - insert 600000 elements in a hash map 25 times, with clear inbetween\n")
    profile_test("table<int;int>", type<table<int;int>>, 0)
    profile_test("TCuckooHashMap<int,int> hash(x)->x/hash", type<CuckooHashMap_test>, 0)
    profile_test("TFlatHashMap<int,int>", type<FlatHashMap_test>, 0)
    profile_test("TFlatHashMap<int,int> with hash intrinsic", type<FlatHashMap_test_inline>, 0)
    profile_test("TFlatHashMap<int,int> with hash(x)->x", type<FlatHashMap_test0>, 0)

