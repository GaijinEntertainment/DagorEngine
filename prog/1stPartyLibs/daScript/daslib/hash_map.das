options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module hash_map shared private

require daslib/spoof public

require daslib/random public
require strings public

//! FlatHashMap is a hash map that uses open addressing with linear probing.
let public TFlatHashMap = %spoof_template~(FlatHashMap,KeyType,ValueType,HashFunction=hash)
[skip_field_lock_check]
struct %FlatHashMap
    keys   : array<%KeyType>
    hashes : array<uint64>
    values : array<%ValueType>
    mask : int
    length : int
    capacity : int
    tombstones : int

def %FlatHashMap ( data : tuple<k:%KeyType;v:%ValueType>[] ) : %FlatHashMap
    var self : %FlatHashMap
    with self
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        length = 0
        capacity = initialSize
        keys |> resize(initialSize)
        hashes |> resize(initialSize)
        values |> resize(initialSize)
        for kv in data
            self[kv.k] := kv.v
    return <- self

def %FlatHashMap ( data : array<tuple<k:%KeyType;v:%ValueType>> ) : %FlatHashMap
    var self : %FlatHashMap
    with self
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        length = 0
        capacity = initialSize
        keys |> resize(initialSize)
        hashes |> resize(initialSize)
        values |> resize(initialSize)
        for kv in data
            self[kv.k] := kv.v
    return <- self

def %FlatHashMap ( initialSize : int = 8 ) : %FlatHashMap
    assert((initialSize & (initialSize-1))==0, "size must be a power of 2")
    var self : %FlatHashMap
    with self
        capacity = initialSize
        keys |> resize(initialSize)
        hashes |> resize(initialSize)
        values |> resize(initialSize)
        mask = initialSize - 1
    return <- self

def empty ( var self:%FlatHashMap explicit ) : bool
    return self.length == 0

def length ( var self:%FlatHashMap explicit ) : int
    return self.length

def clear ( var self:%FlatHashMap explicit )
    with self
        for h in hashes
            h = 0ul
        length = 0
        tombstones = 0

def grow ( var self:%FlatHashMap explicit )
    with self
        let newSize = capacity * 2
        self |> reserve(newSize)

def rehash ( var self:%FlatHashMap explicit )
    with self
        self |> reserve(capacity)

[hint(unsafe_range_check,noalias=self)]
def reserve ( var self:%FlatHashMap explicit; newSize : int )
    assert((newSize & (newSize-1))==0, "size must be a power of 2")
    with self
        var newKeys : array<%KeyType>
        var newHashes : array<uint64>
        var newValues : array<%ValueType>
        newKeys |> resize(newSize)
        newHashes |> resize(newSize)
        newValues |> resize(newSize)
        mask = newSize - 1
        swap(newKeys, keys)
        swap(newHashes, hashes)
        swap(newValues, values)
        length = 0
        tombstones = 0
        capacity = newSize
        for k,v,h in newKeys, newValues, newHashes
            if h > 1ul
                self[k] <- v
        newKeys |> resize(0)
        newHashes |> resize(0)
        newValues |> resize(0)
        delete newKeys
        delete newHashes
        delete newValues

[hint(unsafe_range_check,noalias=self)]
def key_index ( self:%FlatHashMap explicit; key : %KeyType )
    with self
        let hash = %HashFunction(key)
        var index = int(hash) & mask
        while true
            let h = hashes[index]
            if h == hash
                if keys[index] == key   // otherwise skip matching hash collision
                    return index
            elif h==0ul                 // empty, key not found
                break
            // skip hash collision or tombstone(1)
            index = (index + 1) & mask
        return -1

[hint(unsafe_range_check,noalias=self)]
def key_exists ( self:%FlatHashMap explicit; key : %KeyType )
    with self
        let index = self |> key_index(key)
        return index != -1

[hint(unsafe_range_check,noalias=self)]
def get ( var self:%FlatHashMap ==const explicit; key : %KeyType; blk:block<(var v:%ValueType):void> ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            blk |> invoke(values[index])
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def get ( self:%FlatHashMap ==const explicit; key : %KeyType; blk:block<(v:%ValueType):void> ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            blk |> invoke(values[index])
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def erase ( var self:%FlatHashMap explicit; key : %KeyType ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            hashes[index] = 1ul   // tombstone
            tombstones ++
            length --
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def foreach ( self:%FlatHashMap ==const explicit; blk:block<(k:%KeyType;v:%ValueType):void> )
    with self
        for k,v,h in keys, values, hashes
            if h > 1ul
                blk |> invoke(k,v)

[hint(unsafe_range_check,noalias=self)]
def foreach ( var self:%FlatHashMap ==const explicit; blk:block<(k:%KeyType;var v:%ValueType):void> )
    with self
        for k,v,h in keys, values, hashes
            if h > 1ul
                blk |> invoke(k,v)

[unsafe_outside_of_for]
def keys ( var self:%FlatHashMap explicit )
    var pself = unsafe(addr(self))
    return <- generator<%KeyType&>() <|
        for h,k in pself.hashes, pself.keys
            if h > 1ul
                yield k
        return false
    finally
        pself = null

[unsafe_outside_of_for]
def values ( var self:%FlatHashMap ==const explicit )
    var pself = unsafe(addr(self))
    return <- generator<%ValueType&>() <|
        for h,v in pself.hashes, pself.values
            if h > 1ul
                yield v
        return false
    finally
        pself = null

[unsafe_outside_of_for]
def values ( self:%FlatHashMap ==const explicit )
    var pself = unsafe(addr(self))
    return <- generator<%ValueType const&>() <|
        for h,v in pself.hashes, pself.values
            if h > 1ul
                yield v
        return false
    finally
        pself = null

[hint(unsafe_range_check,noalias=self)]
def operator [] ( var self:%FlatHashMap ==const explicit; key : %KeyType ) : %ValueType&
    with self
        if length > (capacity>>1) // ((mask+1)*2/3)
            self |> grow()
        elif ((capacity - length)>>1) < tombstones
            self |> rehash()
        let hash = %HashFunction(key)
        var index = int(hash) & mask
        var lastTombstone = -1
        while true
            let h = hashes[index]
            if h == hash
                if keys[index] == key   // otherwise skip matching hash collision
                    unsafe
                        return values[index]
            elif h == 1ul
                if lastTombstone == -1
                    lastTombstone = index
            elif h == 0ul
                if lastTombstone != -1
                    index = lastTombstone
                    tombstones --
                keys[index] := key
                hashes[index] = hash
                length ++
                unsafe
                    return values[index]
            index = (index + 1) & mask
        panic("unreachable")
        unsafe
            return values[0] // we never get here due to panic

[hint(unsafe_range_check,noalias=self)]
def operator [] ( self:%FlatHashMap ==const explicit; key : %KeyType ) : %ValueType const&
    with self
        let index = self |> key_index(key)
        if index != -1
            unsafe
                return values[index]
        panic("key not found")
        unsafe
            return values[0] // we never get here due to panic

[hint(unsafe_range_check,noalias=self)]
def operator ?[] ( var self:%FlatHashMap ==const explicit; key : %KeyType ) : %ValueType?
    with self
        let index = self |> key_index(key)
        if index != -1
            return unsafe(addr(values[index]))
        else
            return null

[hint(unsafe_range_check,noalias=self)]
def operator ?[] ( self:%FlatHashMap ==const explicit; key : %KeyType ) : %ValueType const?
    with self
        let index = self |> key_index(key)
        if index != -1
            return unsafe(addr(values[index]))
        else
            return null
%%

[hint(alwaysinline)]
def public hash0 ( k )
    //! this hash function converts and workhorse key to a 64 bit hash
    let k64 = uint64(k)
    return k64 <= 2ul ? 0x9e3779b97f4a7c15ul : k64

[hint(alwaysinline)]
def public hash0 ( s : string ) : uint64
    //! adler32 extended to 64 bits
    let MOD_ADLER = 0xFFFFFFFFFFFFFFC5ul
    var a = 1ul
    var b = 0ul
    for c in s
        a = (a + uint64(c)) % MOD_ADLER
        b = (b + a) % MOD_ADLER
    return (uint64(b) << 32ul) | uint64(a)

//! CompactHashMap is a hash map that uses 2 hash functions and double lookup.
let public TCuckooHashMap = %spoof_template~(CuckooHashMap,KeyType,ValueType,FirstHashFunction=hash0,SecondHashFunction=hash)

struct KHV_%CuckooHashMap
    key : %KeyType
    hash : uint64
    value : %ValueType

[skip_field_lock_check]
struct %CuckooHashMap
    khv : array<KHV_%CuckooHashMap>
    mask : int
    length : int
    seed : int4

def %CuckooHashMap ( data : tuple<k:%KeyType;v:%ValueType>[] ) : %CuckooHashMap
    var self : %CuckooHashMap
    with self
        seed = random_seed(13)
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        length = 0
        khv |> resize(initialSize)
        for kv in data
            self[kv.k] := kv.v
    return <- self

def %CuckooHashMap ( data : array<tuple<k:%KeyType;v:%ValueType>> ) : %CuckooHashMap
    var self : %CuckooHashMap
    with self
        seed = random_seed(13)
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        length = 0
        khv |> resize(initialSize)
        for kv in data
            self[kv.k] := kv.v
    return <- self

def %CuckooHashMap ( initialSize : int = 64 ) : %CuckooHashMap
    assert((initialSize & (initialSize-1))==0, "size must be a power of 2")
    var self : %CuckooHashMap
    with self
        seed = random_seed(13)
        khv |> resize(initialSize)
        mask = initialSize - 1
    return <- self

def empty ( var self:%CuckooHashMap explicit ) : bool
    return self.length == 0

def length ( var self:%CuckooHashMap explicit ) : int
    return self.length

def clear ( var self:%CuckooHashMap explicit )
    with self
        for t in khv
            t.hash = 0ul
        length = 0

def grow ( var self:%CuckooHashMap explicit )
    with self
        let newSize = length(khv) * 2
        self |> reserve(newSize)

[hint(unsafe_range_check,noalias=self)]
def reserve ( var self:%CuckooHashMap explicit; newSize : int )
    assert((newSize & (newSize-1))==0, "size must be a power of 2")
    with self
        var newKHV : array<KHV_%CuckooHashMap>
        newKHV |> resize(newSize)
        mask = newSize - 1
        swap(newKHV, khv)
        length = 0
        for t in newKHV
            if t.hash != 0ul
                self[t.key] <- t.value
        newKHV |> resize(0)
        delete newKHV

[hint(unsafe_range_check,noalias=self)]
def key_index ( self:%CuckooHashMap explicit; key : %KeyType )
    with self
        let firstHash = %FirstHashFunction(key)
        var index = int(firstHash) & mask
        if khv[index].hash == firstHash && khv[index].key == key
            return index
        var secondHash = %SecondHashFunction(key)
        index = int(secondHash) & mask
        if khv[index].hash == secondHash && khv[index].key == key
            return index
        return -1

[hint(unsafe_range_check,noalias=self)]
def key_exists ( self:%CuckooHashMap explicit; key : %KeyType )
    with self
        let index = self |> key_index(key)
        return index != -1

[hint(unsafe_range_check,noalias=self)]
def get ( var self:%CuckooHashMap ==const explicit; key : %KeyType; blk:block<(var v:%ValueType):void> ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            blk |> invoke(khv[index].value)
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def get ( self:%CuckooHashMap ==const explicit; key : %KeyType; blk:block<(v:%ValueType):void> ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            blk |> invoke(khv[index].value)
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def erase ( var self:%CuckooHashMap explicit; key : %KeyType ) : bool
    with self
        let index = self |> key_index(key)
        if index != -1
            khv[index].hash = 0ul
            length --
            return true
        else
            return false

[hint(unsafe_range_check,noalias=self)]
def foreach ( self:%CuckooHashMap ==const explicit; blk:block<(k:%KeyType;v:%ValueType):void> )
    with self
        for t in khv
            if t.hash != 0ul
                blk |> invoke(t.key,t.value)

[hint(unsafe_range_check,noalias=self)]
def foreach ( var self:%CuckooHashMap ==const explicit; blk:block<(k:%KeyType;var v:%ValueType):void> )
    with self
        for t in khv
            if t.hash != 0ul
                blk |> invoke(t.key,t.value)

[unsafe_outside_of_for]
def keys ( var self:%CuckooHashMap explicit )
    var pself = unsafe(addr(self))
    return <- generator<%KeyType&>() <|
        for t in pself.khv
            if t.hash != 0ul
                yield t.key
        return false
    finally
        pself = null

[unsafe_outside_of_for]
def values ( var self:%CuckooHashMap ==const explicit )
    var pself = unsafe(addr(self))
    return <- generator<%ValueType&>() <|
        for t in pself.khv
            if t.hash != 0ul
                yield t.value
        return false
    finally
        pself = null

[unsafe_outside_of_for]
def values ( self:%CuckooHashMap ==const explicit )
    var pself = unsafe(addr(self))
    return <- generator<%ValueType const&>() <|
        for t in pself.khv
            if t.hash != 0ul
                yield t.value
        return false
    finally
        pself = null

[hint(unsafe_range_check,noalias=self)]
def insert ( var self:%CuckooHashMap explicit; _key : %KeyType; _value : %ValueType )
    //! insert a key value pair into the hash map
    with self
        var key := _key
        var value := _value
        while true
            for i in range(length(khv)/2)
                var firstHash = %FirstHashFunction(key)
                var firstIndex = int(firstHash) & mask
                if khv[firstIndex].hash == 0ul
                    khv[firstIndex].hash = firstHash
                    khv[firstIndex].key <- key
                    khv[firstIndex].value <- value
                    length ++
                    return
                var secondHash = %SecondHashFunction(key)
                var secondIndex = int(secondHash) & mask
                if khv[secondIndex].hash == 0ul
                    khv[secondIndex].hash = secondHash
                    khv[secondIndex].key <- key
                    khv[secondIndex].value <- value
                    length ++
                    return
                if (random_int(seed) & 1) == 0
                    swap(key, khv[firstIndex].key)
                    swap(value, khv[firstIndex].value)
                    khv[firstIndex].hash = firstHash
                else
                    swap(key, khv[secondIndex].key)
                    swap(value, khv[secondIndex].value)
                    khv[secondIndex].hash = secondHash
            self |> grow()

def describe ( var self:%CuckooHashMap ==const explicit )
    with self
        return build_string <| $ ( writer )
            writer |> write("\{\{\n")
            for t,index in khv,count()
                if t.hash != 0ul
                    writer |> write(t.key)
                    writer |> write("=>")
                    writer |> write(t.value)
                    writer |> write(index)
                    writer |> write(";")
            writer |> write("\}\}\n")

[hint(unsafe_range_check,noalias=self)]
def operator [] ( var self:%CuckooHashMap ==const explicit; key : %KeyType ) : %ValueType&
    with self
        if length > ((mask+1)>>1) // ((mask+1)*2/3)
            self |> grow()
        // if its already there
        var firstHash = %FirstHashFunction(key)
        var firstIndex = int(firstHash) & mask
        if khv[firstIndex].hash == firstHash && khv[firstIndex].key == key
            unsafe
                return khv[firstIndex].value
        var secondHash = %SecondHashFunction(key)
        var secondIndex = int(secondHash) & mask
        if khv[secondIndex].hash == secondHash && khv[secondIndex].key == key
            unsafe
                return khv[secondIndex].value
        // now, if there is a room to insert
        if khv[firstIndex].hash == 0ul
            khv[firstIndex].hash = firstHash
            khv[firstIndex].key := key
            length ++
            unsafe
                return khv[firstIndex].value
        if khv[secondIndex].hash == 0ul
            khv[secondIndex].hash = secondHash
            khv[secondIndex].key := key
            length ++
            unsafe
                return khv[secondIndex].value
        // now we have to kick out one of the existing keys
        self |> insert(key, [[%ValueType]])
        let index = self |> key_index(key)
        unsafe
            return khv[index].value

[hint(unsafe_range_check,noalias=self)]
def operator [] ( self:%CuckooHashMap ==const explicit; key : %KeyType ) : %ValueType const&
    with self
        let index = self |> key_index(key)
        if index != -1
            unsafe
                return khv[index].value
        panic("key not found")
        unsafe
            return khv[0].value // we never get here due to panic

[hint(unsafe_range_check,noalias=self)]
def operator ?[] ( var self:%CuckooHashMap ==const explicit; key : %KeyType ) : %ValueType?
    with self
        let index = self |> key_index(key)
        if index != -1
            return unsafe(addr(khv[index].value))
        else
            return null

[hint(unsafe_range_check,noalias=self)]
def operator ?[] ( self:%CuckooHashMap ==const explicit; key : %KeyType ) : %ValueType const?
    with self
        let index = self |> key_index(key)
        if index != -1
            return unsafe(addr(khv[index].value))
        else
            return null
%%
