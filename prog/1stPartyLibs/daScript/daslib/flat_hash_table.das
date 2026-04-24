options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module flat_hash_table shared private

//! Template flat hash map using open addressing with linear probing.
//!
//! ``TFlatHashTable`` is a ``[template_structure]`` parameterised by key type,
//! value type, and hash function. It stores keys, values, and hashes in
//! parallel arrays for cache-friendly access and supports ``insert``,
//! ``erase``, iteration, and automatic growth.

require daslib/ast_boost
require daslib/templates_boost
require daslib/typemacro_boost
require daslib/macro_boost
require daslib/class_boost

//! TFlatHashTable is a hash map that uses open addressing with linear probing.
[template_structure(KeyType, ValueType, hashFunctionName=@@hash)]
struct template TFlatHashTable {
    //! Open-addressing hash table with linear probing.
    key_data   : array<KeyType>
    hashes : array<uint64>
    value_data : array<ValueType>
    mask : int
    data_length : int
    capacity : int
    tombstones : int

    def static TFlatHashTable(data : tuple<k : KeyType; v : ValueType>[]) : TFlatHashTable {
        var self : TFlatHashTable
        with (self) {
            let bit = int(clz(uint(length(data))))
            let initialSize = 1 << (32 - bit)
            mask = initialSize - 1
            data_length = 0
            capacity = initialSize
            key_data |> resize(initialSize)
            hashes |> resize(initialSize)
            value_data |> resize(initialSize)
            for (kv in data) {
                self[kv.k] := kv.v
            }
        }
        return <- self
    }

    def TFlatHashTable(data : array<tuple<k : KeyType; v : ValueType>>) {
        let bit = int(clz(uint(length(data))))
        let initialSize = 1 << (32 - bit)
        mask = initialSize - 1
        data_length = 0
        capacity = initialSize
        key_data |> resize(initialSize)
        hashes |> resize(initialSize)
        value_data |> resize(initialSize)
        for (kv in data) {
            self[kv.k] := kv.v
        }
    }

    def TFlatHashTable(initialSize : int = 8) {
        assert((initialSize & (initialSize - 1)) == 0, "size must be a power of 2")
        capacity = initialSize
        key_data |> resize(initialSize)
        hashes |> resize(initialSize)
        value_data |> resize(initialSize)
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
        for (h in hashes) {
            h = 0ul
        }
        data_length = 0
        tombstones = 0
    }

    [class_method]
    def static grow() {
        let newSize = capacity * 2
        self |> reserve(newSize)
    }

    [class_method]
    def static rehash() {
        self |> reserve(capacity)
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static reserve(newSize : int) {
        assert((newSize & (newSize - 1)) == 0, "size must be a power of 2")
        var newkey_data : array<KeyType>
        var newHashes : array<uint64>
        var newvalue_data : array<ValueType>
        newkey_data |> resize(newSize)
        newHashes |> resize(newSize)
        newvalue_data |> resize(newSize)
        mask = newSize - 1
        swap(newkey_data, key_data)
        swap(newHashes, hashes)
        swap(newvalue_data, value_data)
        data_length = 0
        tombstones = 0
        capacity = newSize
        for (k, v, h in newkey_data, newvalue_data, newHashes) {
            if (h > 1ul) {
                self[k] <- v
            }
        }
        newkey_data |> resize(0)
        newHashes |> resize(0)
        newvalue_data |> resize(0)
        delete newkey_data
        delete newHashes
        delete newvalue_data
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static const key_index(key : KeyType) {
        let hash = $c(hashFunctionName)(key)
        var index = int(hash) & mask
        while (true) {
            let h = hashes[index]
            if (h == hash) {
                if (key_data[index] == key) {   // otherwise skip matching hash collision
                    return index
                }
            } elif (h == 0ul) {               // empty, key not found
                break
            }
            // skip hash collision or tombstone(1)
            index = (index + 1) & mask
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
        with (self) {
            let index = self |> key_index(key)
            if (index != -1) {
                blk |> invoke(value_data[index])
                return true
            } else {
                return false
            }
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const get(key : KeyType; blk : block<(v : ValueType) : void>) : bool {
        with (self) {
            let index = self |> key_index(key)
            if (index != -1) {
                blk |> invoke(value_data[index])
                return true
            } else {
                return false
            }
        }
    }

    [class_method, hint(unsafe_range_check, noalias=self)]
    def static erase(key : KeyType) : bool {
        let index = self |> key_index(key)
        if (index != -1) {
            hashes[index] = 1ul   // tombstone
            tombstones ++
            data_length --
            return true
        } else {
            return false
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const foreach(blk : block<(k : KeyType; v : ValueType) : void>) {
        with (self) {
            for (k, v, h in key_data, value_data, hashes) {
                if (h > 1ul) {
                    blk |> invoke(k, v)
                }
            }
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static foreach(blk : block<(k : KeyType; var v : ValueType) : void>) {
        with (self) {
            for (k, v, h in key_data, value_data, hashes) {
                if (h > 1ul) {
                    blk |> invoke(k, v)
                }
            }
        }
    }

    [class_method, unsafe_outside_of_for]
    def static keys() {
        var pself = unsafe(addr(self))
        return <- generator<KeyType&>() <| $() {
            for (h, k in pself.hashes, pself.key_data) {
                if (h > 1ul) {
                    yield k
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
            for (h, v in pself.hashes, pself.value_data) {
                if (h > 1ul) {
                    yield v
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
            for (h, v in pself.hashes, pself.value_data) {
                if (h > 1ul) {
                    yield v
                }
            }
            return false
        } finally {
            pself = null
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static operator [] (key : KeyType) : ValueType & {
        if (data_length > (capacity >> 1)) {// ((mask+1)*2/3)
            self |> grow()
        } elif (((capacity - data_length) >> 1) < tombstones) {
            self |> rehash()
        }
        let hash = $c(hashFunctionName)(key)
        var index = int(hash) & mask
        var lastTombstone = -1
        while (true) {
            let h = hashes[index]
            if (h == hash) {
                if (key_data[index] == key) {   // otherwise skip matching hash collision
                    unsafe {
                        return value_data[index]
                    }
                }
            } elif (h == 1ul) {
                if (lastTombstone == -1) {
                    lastTombstone = index
                }
            } elif (h == 0ul) {
                if (lastTombstone != -1) {
                    index = lastTombstone
                    tombstones --
                }
                key_data[index] := key
                hashes[index] = hash
                data_length ++
                unsafe {
                    return value_data[index]
                }
            }
            index = (index + 1) & mask
        }
        panic("unreachable")
        unsafe {
            return value_data[0] // we never get here due to panic
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const operator [] (key : KeyType) : ValueType const & {
        let index = self |> key_index(key)
        if (index != -1) {
            unsafe {
                return value_data[index]
            }
        }
        panic("key not found")
        unsafe {
            return value_data[0] // we never get here due to panic
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static operator ?[] (key : KeyType) : ValueType ? {
        let index = self |> key_index(key)
        if (index != -1) {
            return unsafe(addr(value_data[index]))
        } else {
            return null
        }
    }

    [explicit_const_class_method, hint(unsafe_range_check, noalias=self)]
    def static const operator ?[] (key : KeyType) : ValueType const ? {
        let index = self |> key_index(key)
        if (index != -1) {
            return unsafe(addr(value_data[index]))
        } else {
            return null
        }
    }
}

