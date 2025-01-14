options persistent_heap

require _framework
require daslib/hash_map

%spoof_instance~TCuckooHashMap(CuckooHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test,int,int)%%;

[sideeffects]
def test ( hmap : auto(HashMapType); dummy )
    var inscope hashMap : HashMapType
    static_if !typeinfo(is_table type<HashMapType>)
        hashMap <- HashMapType()
    for i in range(250)
        for j in range(300000)
            unsafe(hashMap[3]) = 3

[export]
def main
    print("test05 - insert 300000 elements of the same value 250 times\n")
    profile_test("table<int;int>", type<table<int;int>>,0)
    profile_test("TCuckooHashMap<int,int>", type<CuckooHashMap_test>, 0)
    profile_test("TFlatHashMap<int,int>", type<FlatHashMap_test>,0)



