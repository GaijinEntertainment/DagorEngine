options persistent_heap

// OLD das restrictions
options unsafe_table_lookup = false

require _framework
require daslib/hash_map

%spoof_instance~TCuckooHashMap(CuckooHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_test,int,int)%%;
%spoof_instance~TFlatHashMap(PathalogicalFlatHashMap_test,int,int,pathalogical)%%;

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

def pathalogical ( t : int )
    return t>=2 ? uint64(t) : 1099511628211ul

[export]
def main
    print("test012 - insert {TOTAL_RANDOM_NUMBERS} specially distributed numbers (all unique, apart from first 32768 sequential), clear, insert again\n")
    var randomNumbers <- generate_unique_positive_random_numbers()
    for n in range(32768)
        randomNumbers[n] = n
    profile_test("table<int;int>", type<table<int;int>>, randomNumbers)
    profile_test("TCuckooHashMap<int,int>", type<CuckooHashMap_test>, randomNumbers)
    profile_test("TFlatHashMap<int,int>", type<FlatHashMap_test>, randomNumbers)
    profile_test("TFlatHashMap<int,int> with hash(t)->t pathalogical", type<PathalogicalFlatHashMap_test>, randomNumbers)


