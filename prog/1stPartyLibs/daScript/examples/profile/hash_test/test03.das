options persistent_heap

// OLD das restrictions
options unsafe_table_lookup = false

require _framework
require daslib/hash_map

%spoof_instance~TCuckooHashMap(CuckooHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test,int,int)%%;

[sideeffects]
def test ( hmap : auto(HashMapType); randomNumbers )
    for i in range(10)
        var hashMap : HashMapType
        static_if !typeinfo(is_table type<HashMapType>)
            hashMap <- HashMapType()
        for num in randomNumbers
            hashMap[num] = -num
        hashMap |> clear()
        for num in randomNumbers
            hashMap[num] = -num
        delete hashMap


[export]
def main
    print("test03 - insert {TOTAL_RANDOM_NUMBERS} unique random numbers, clear, insert again\n")
    var randomNumbers <- generate_unique_positive_random_numbers()
    profile_test("table<int;int>", type<table<int;int>>, randomNumbers)
    profile_test("TCuckooHashMap<int,int>", type<CuckooHashMap_test>, randomNumbers)
    profile_test("TFlatHashMap<int,int>", type<FlatHashMap_test>, randomNumbers)




