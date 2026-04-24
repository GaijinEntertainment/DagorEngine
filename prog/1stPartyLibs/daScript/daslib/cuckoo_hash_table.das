options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module cuckoo_hash_table shared private

//! Template cuckoo hash map.
//!
//! ``TCuckooHashTable`` is a ``[template_structure]`` implementing a
//! cuckoo hash table with two hash functions and linear probing as
//! fallback. Good worst-case lookup performance at the cost of more
//! complex insertions.

require daslib/ast_boost
require daslib/templates_boost
require daslib/typemacro_boost
require daslib/macro_boost
require daslib/class_boost
require daslib/random public
require strings public

def public hash0(data) {
    //! this hash function converts and workhorse key to a 64 bit hash
    let k64 = uint64(data)
    return k64 <= 2ul ? 0x9e3779b97f4a7c15ul : k64
}

def public hash_extra(data) {
    //! Returns a secondary hash derived from the upper 32 bits of the primary hash, used for cuckoo hashing.
    var k64 = hash(data)
    k64 = k64 <= 2ul ? 0x9e3779b97f4a7c15ul : k64
    return k64 >>> 32ul
}

/*
def public hash_extra(s : string) : uint64 {
    var h : uint64 = 14695981039346656037ul
    for (c in s) {
        h = (h ^ uint64(c)) * 1099511628211ul
    }
    return h
}
*/

[template_structure(KeyType, ValueType, hashFunction0Name=@@hash0, hashFunction1Name=@@hash_extra)]
struct template TCuckooHashTable {
    //! Cuckoo hash table using two hash functions for collision resolution.
    typedef KHV_TCuckooHashTable = tuple<key : KeyType; hash : uint64; value : ValueType>
    khv : array<KHV_TCuckooHashTable>
    mask : int
    data_length : int
    seed : int4

    def static TCuckooHashTable(data : tuple<k : KeyType; v : ValueType>[]) : TCuckooHashTable {
        var self : TCuckooHashTable
        with (self) {
            seed = random_seed(13)
            let bit = int(clz(uint(length(data))))
            let initialSize = 1 << (32 - bit)
            mask = initialSize - 1
            data_length = 0
            khv |> resize(initialSize)
            for (kv in data) {
                self[kv.k] := kv.v
            }
        }
        return <- self
    }

    def TCuckooHashTable(data : array<tuple<k : KeyType; v : ValueType>>) {
        seed = random_seed(13)
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        data_length = 0
        khv |> resize(initialSize)
        for (kv in data) {
            self[kv.k] := kv.v
        }
    }

    def TCuckooHashTable(initialSize : int = 64) {
        assert((initialSize & (initialSize - 1)) == 0, "size must be a power of 2")
        seed = random_seed(13)
        khv |> resize(initialSize)
        mask = initialSize - 1
    }

    [class_method]
    def static const empty() : bool {
        return data_length == 0
    }

    [class_method]
    def static const length() : int {
        return data_length
    }

    [class_method]
    def static clear() {
        for (t in khv) {
            t.hash = 0ul
        }
        data_length = 0
    }

    [class_method]
    def static grow() {
        let newSize = length(khv) * 2
        self |> reserve(newSize)
    }

    [class_method]
    def static reserve(newSize : int) {
        assert((newSize & (newSize - 1)) == 0, "size must be a power of 2")
        var newKHV : array<KHV_TCuckooHashTable>
        newKHV |> resize(newSize)
        mask = newSize - 1
        swap(newKHV, khv)
        data_length = 0
        for (t in newKHV) {
            if (t.hash != 0ul) {
                self[t.key] <- t.value
            }
        }
        newKHV |> resize(0)
        delete newKHV
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static const key_index(key : KeyType) {
        let firstHash = $c(hashFunction0Name)(key)
        var index = int(firstHash) & mask
        if (khv[index].hash == firstHash && khv[index].key == key) {
            return index
        }
        var secondHash = $c(hashFunction1Name)(key)
        index = int(secondHash) & mask
        if (khv[index].hash == secondHash && khv[index].key == key) {
            return index
        }
        return -1
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static const key_exists(key : KeyType) {
        let index = self |> key_index(key)
        return index != -1
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static get(key : KeyType; blk : block<(var v : ValueType) : void>) : bool {
        let index = self |> key_index(key)
        if (index != -1) {
            blk |> invoke(khv[index].value)
            return true
        } else {
            return false
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const get(key : KeyType; blk : block<(v : ValueType) : void>) : bool {
        let index = self |> key_index(key)
        if (index != -1) {
            blk |> invoke(khv[index].value)
            return true
        } else {
            return false
        }
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static erase(key : KeyType) : bool {
        let index = self |> key_index(key)
        if (index != -1) {
            khv[index].hash = 0ul
            data_length --
            return true
        } else {
            return false
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const foreach(blk : block<(k : KeyType; v : ValueType) : void>) {
        for (t in khv) {
            if (t.hash != 0ul) {
                blk |> invoke(t.key, t.value)
            }
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static foreach(blk : block<(k : KeyType; var v : ValueType) : void>) {
        for (t in khv) {
            if (t.hash != 0ul) {
                blk |> invoke(t.key, t.value)
            }
        }
    }

    [class_method, unsafe_outside_of_for]
    def static keys() {
        var pself = unsafe(addr(self))
        return <- generator<KeyType&>() <| $() {
            for (t in pself.khv) {
                if (t.hash != 0ul) {
                    yield t.key
                }
            }
            return false
        } finally {
            pself = null
        }
    }

    [explicit_const_class_method, unsafe_outside_of_for]
    def static values() {
        var pself = unsafe(addr(self))
        return <- generator<ValueType&>() <| $() {
            for (t in pself.khv) {
                if (t.hash != 0ul) {
                    yield t.value
                }
            }
            return false
        } finally {
            pself = null
        }
    }

    [explicit_const_class_method, unsafe_outside_of_for]
    def static const values() {
        var pself = unsafe(addr(self))
        return <- generator<ValueType const&>() <| $() {
            for (t in pself.khv) {
                if (t.hash != 0ul) {
                    yield t.value
                }
            }
            return false
        } finally {
            pself = null
        }
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static insert(_key : KeyType; _value : ValueType) {
        //! insert a key value pair into the hash map
        var key := _key
        var value := _value
        while (true) {
            for (i in range(length(khv) / 2)) {
                var firstHash = $c(hashFunction0Name)(key)
                var firstIndex = int(firstHash) & mask
                if (khv[firstIndex].hash == 0ul) {
                    khv[firstIndex].hash = firstHash
                    khv[firstIndex].key <- key
                    khv[firstIndex].value <- value
                    data_length ++
                    return
                }
                var secondHash = $c(hashFunction1Name)(key)
                var secondIndex = int(secondHash) & mask
                if (khv[secondIndex].hash == 0ul) {
                    khv[secondIndex].hash = secondHash
                    khv[secondIndex].key <- key
                    khv[secondIndex].value <- value
                    data_length ++
                    return
                }
                if ((random_int(seed) & 1) == 0) {
                    swap(key, khv[firstIndex].key)
                    swap(value, khv[firstIndex].value)
                    khv[firstIndex].hash = firstHash
                } else {
                    swap(key, khv[secondIndex].key)
                    swap(value, khv[secondIndex].value)
                    khv[secondIndex].hash = secondHash
                }
            }
            self |> grow()
        }
    }

    [explicit_const_class_method]
    def static const describe() {
        return build_string() $(writer) {
            writer |> write("\{\{\n")
            for (t, index in khv, count()) {
                if (t.hash != 0ul) {
                    writer |> write(t.key)
                    writer |> write("=>")
                    writer |> write(t.value)
                    writer |> write(index)
                    writer |> write(";")
                }
            }
            writer |> write("\}\}\n")
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static operator [] (key : KeyType) : ValueType & {
        if (data_length > ((mask + 1) >> 1)) {// ((mask+1)*2/3)
            self |> grow()
        }
        // if its already there
        var firstHash = $c(hashFunction0Name)(key)
        var firstIndex = int(firstHash) & mask
        if (khv[firstIndex].hash == firstHash && khv[firstIndex].key == key) {
            unsafe {
                return khv[firstIndex].value
            }
        }
        var secondHash = $c(hashFunction1Name)(key)
        var secondIndex = int(secondHash) & mask
        if (khv[secondIndex].hash == secondHash && khv[secondIndex].key == key) {
            unsafe {
                return khv[secondIndex].value
            }
        }
        // now, if there is a room to insert
        if (khv[firstIndex].hash == 0ul) {
            khv[firstIndex].hash = firstHash
            khv[firstIndex].key := key
            data_length ++
            unsafe {
                return khv[firstIndex].value
            }
        }
        if (khv[secondIndex].hash == 0ul) {
            khv[secondIndex].hash = secondHash
            khv[secondIndex].key := key
            data_length ++
            unsafe {
                return khv[secondIndex].value
            }
        }
        // now we have to kick out one of the existing keys
        self |> insert(key, default<ValueType>)
        let index = self |> key_index(key)
        unsafe {
            return khv[index].value
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const operator [] (key : KeyType) : ValueType const & {
        let index = self |> key_index(key)
        if (index != -1) {
            unsafe {
                return khv[index].value
            }
        }
        panic("key not found")
        unsafe {
            return khv[0].value // we never get here due to panic
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static operator ?[] (key : KeyType) : ValueType ? {
        with (self) {
            let index = self |> key_index(key)
            if (index != -1) {
                return unsafe(addr(khv[index].value))
            } else {
                return null
            }
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const operator ?[] (key : KeyType) : ValueType const ? {
        let index = self |> key_index(key)
        if (index != -1) {
            return unsafe(addr(khv[index].value))
        } else {
            return null
        }
    }
}
