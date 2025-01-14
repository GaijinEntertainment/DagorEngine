options persistent_heap

require _framework
require daslib/hash_map
require daslib/spoof

//! Slot adapter for SlotMap
let public TSlotMapAdapter = %spoof_template~(SlotMap,HashMapType)
[skip_field_lock_check]
struct %SlotMap
    id : uint64
    hashMap : %HashMapType

def %SlotMap
    var self : %SlotMap
    static_if !typeinfo(is_table type<%HashMapType>)
        self.hashMap <- %HashMapType()
    return <- self

def emplace ( var self : %SlotMap explicit; val : int )
    with self
        let r = id
        unsafe(hashMap[id]) = val
        id ++
        return r
%%

%spoof_instance~TCuckooHashMap(CuckooHashMap_uint64_int,uint64,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap_uint64_int,uint64,int)%%;
%spoof_instance~TFlatHashMap(FlatHashMap0_uint64_int,uint64,int,hash0)%%;

typedef SlotTable = table<uint64;int>

%spoof_instance~TSlotMapAdapter(SlotMapAdapter_Table,SlotTable)%%;
%spoof_instance~TSlotMapAdapter(SlotMapAdapter_CuckooHashMap,CuckooHashMap_uint64_int)%%;
%spoof_instance~TSlotMapAdapter(SlotMapAdapter_FlatHashMap,FlatHashMap_uint64_int)%%;
%spoof_instance~TSlotMapAdapter(SlotMapAdapter_FlatHashMap0,FlatHashMap0_uint64_int)%%;

[sideeffects]
def test ( hmap : auto(SlotMapType); randomNumbers )
    for i in range(25)
        var slotMap <- SlotMapType()
        for num in randomNumbers
            slotMap |> emplace(num)
        delete slotMap

[export]
def main
    print("test11 - insert {TOTAL_RANDOM_NUMBERS} unique random numbers into slotmap\n")
    var randomNumbers <- generate_unique_positive_random_numbers()
    profile_test("table<int;int>", type<SlotMapAdapter_Table>, randomNumbers)
    profile_test("TCuckooHashMap<int,int>", type<SlotMapAdapter_CuckooHashMap>, randomNumbers)
    profile_test("TFlatHashMap<int,int>", type<SlotMapAdapter_FlatHashMap>, randomNumbers)
    profile_test("TFlatHashMap<int,int> with hash(x)->x", type<SlotMapAdapter_FlatHashMap0>, randomNumbers)

