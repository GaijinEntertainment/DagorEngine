options persistent_heap

require _framework
require daslib/hash_map

struct Value
    val : int

%spoof_instance~TCuckooHashMap(CuckooHashMap_test,int,Value)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test,int,Value)%%;

[sideeffects]
def test ( hmap : auto(HashMapType); randomNumbers )
    for i in range(8)
        var hashMap : HashMapType
        static_if !typeinfo(is_table type<HashMapType>)
            hashMap <- HashMapType()
        for num in randomNumbers
            unsafe(hashMap[num]).val ++
        delete hashMap

[export]
def main
    var randomNumbers <- get_random_numbers_with_intersections_10()
    print("test06 - insert {TOTAL_RANDOM_NUMBERS} random numbers with 10% intersections, increment each element's field\n")
    profile_test("table<int;int>", type<table<int;Value>>, randomNumbers)
    profile_test("TCuckooHashMap<int,int>", type<CuckooHashMap_test>, randomNumbers)
    profile_test("TFlatHashMap<int,int>", type<FlatHashMap_test>, randomNumbers)
