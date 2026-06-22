#pragma once

#include "daScript/misc/arraytype.h"
#include "daScript/simulate/hash.h"

namespace das
{

    // TODO:
    //  -   return correct insert index of original value? is this at all possible?
    //  -   throw runtime error in the context, when grow inside locked table (recover well)

    DAS_API extern const char * rts_null;

    template <typename KeyType>
    struct KeyCompare {
        __forceinline bool operator () ( const KeyType & a, const KeyType & b ) {
            return a == b;
        }
    };

    template <>
    struct KeyCompare <vec4f> {
        __forceinline bool operator () ( const vec4f & a, const vec4f & b ) {
            return v_signmask(v_cmp_eq(a, b)) == 0xF;
        }
    };

    template <>
    struct KeyCompare <char *> {
        __forceinline bool operator () ( const char * a, const char * b ) {
            if ( a==b ) return true;
            if ( !a || !b ) return false;
            return strcmp(a,b)==0;
        }
    };

    template <>
    struct KeyCompare <const char *> {
        __forceinline bool operator () ( const char * a, const char * b ) {
            if ( a==b ) return true;
            if ( !a || !b ) return false;
            return strcmp(a,b)==0;
        }
    };

    // Tables with capacity <= this use packed storage: keys/values/hashes are dense in
    // [0,size) (insertion order, load factor 1.0) and find/erase are a flat linear scan
    // instead of a hash probe. Crossing this on insert promotes to open addressing.
    // It must equal TableHash::minCapacity (static_assert below) so every packed table is
    // exactly this many slots — PackedFind scans a fixed count and would read out of bounds
    // otherwise. (Decoupling them — e.g. a smaller minCapacity for memory — means switching
    // PackedFind back to a capacity-bounded scan, losing the fixed unroll.)
    constexpr static uint32_t TABLE_MAX_LINEAR_CAPACITY = 8;

    // Reduce a 64-bit hash to the 32-bit hashKey stored in LARGE (open-addressed) tables. The
    // 0/1 sentinels (EMPTY/KILLED) remap to the FNV-32 prime so a real key never reads as an
    // empty/killed slot. Packed tables do not use this — see PackedPolicy.
    __forceinline TableHashKey hashToHashKey ( TableHashKey hash ) {
        if ( sizeof(TableHashKey)==4 ) return hash <= 1 ? 16777619u : hash; // this should optimize out
        else return hash;
    }

    // Large (open-addressed) NON-STRING tables (tableNoHash) store a 1-byte control state per slot in
    // place of the 32-bit hash: keys compare cheaply, so the hash earns nothing on find. Values mirror
    // HASH_EMPTY64 / HASH_KILLED64 so the liveness test stays "control > TOMBSTONE".
    constexpr static uint8_t CTRL_EMPTY = 0;        // == HASH_EMPTY64
    constexpr static uint8_t CTRL_TOMBSTONE = 1;    // == HASH_KILLED64
    constexpr static uint8_t CTRL_OCCUPIED = 2;     // any live slot (no per-slot hash to distinguish)

    // Liveness of slot i. Packed tables are dense in [0,size) (insertion order, swap-remove on
    // erase), so a slot is live iff i<size — no hash read. Large tables are open-addressed: a string
    // table is live iff the 32-bit hash is past the KILLED sentinel; a non-string table iff the 1-byte
    // control is past TOMBSTONE. EVERY liveness scan (GC, RTTI/data walk, iterators) must route through
    // this: representation varies by capacity AND key type (a packed string table stores 64-bit
    // hashes), so a raw 32-bit `hashes[i] > KILLED` read would misindex it.
    __forceinline bool tableLiveSlot ( const Table & tab, uint64_t i ) {
        // Only STRING tables pack (dense [0,size)); non-string keys are open-addressed at every capacity.
        if ( !tab.tableNoHash && tab.capacity <= TABLE_MAX_LINEAR_CAPACITY ) return i < tab.size;
        if ( tab.tableNoHash ) return ((const uint8_t *) tab.hashes)[i] > CTRL_TOMBSTONE;
        return tab.hashes[i] > HASH_KILLED64;
    }

    // Bytes per hash slot for the type-erased table-size computations (free / GC range) that have
    // no C++ KeyType. The width is NOT a pure function of capacity: a non-string key stores no
    // per-slot hash (tableNoHash flag, set in reserveInternal from PackedPolicy::storesHash), so two
    // same-capacity tables can differ. PACKED string = 64-bit (full-hash compare, no strcmp); PACKED
    // non-string = none. LARGE string = 32-bit hash; LARGE non-string = 1-byte control (CTRL_*).
    // Must agree with PackedPolicy<KeyType>::hashBytes and reserveInternal's large store width.
    __forceinline uint64_t tableHashSlotBytes ( const Table & tab ) {
        // Only STRING tables pack (64-bit hash). Non-string keys are open-addressed at every capacity
        // (1-byte control); large string is 32-bit hash.
        if ( !tab.tableNoHash && tab.capacity <= TABLE_MAX_LINEAR_CAPACITY )
            return uint64_t(sizeof(uint64_t));
        return tab.tableNoHash ? uint64_t(1) : uint64_t(sizeof(TableHashKey));
    }

    // Packed STRING tables store 64-bit hashes in the `hashes` block (declared TableHashKey*=uint32_t*);
    // packed non-string tables store no hash at all, so these helpers are only used on the string path.
    // Access them via memcpy: a direct ((uint64_t*)hashes)[i] lets clang -O3 assume 8-byte alignment
    // and vectorize the fixed packed-find scan into an aligned 16-byte load that faults on linux/wasm
    // (the block is only 8-aligned). memcpy lowers to the same unaligned load with no UB.
    __forceinline uint64_t loadHash64 ( const void * hashes, uint64_t slot ) {
        uint64_t h; memcpy(&h, (const char *)hashes + slot * sizeof(uint64_t), sizeof(uint64_t)); return h;
    }
    __forceinline void storeHash64 ( void * hashes, uint64_t slot, uint64_t h ) {
        memcpy((char *)hashes + slot * sizeof(uint64_t), &h, sizeof(uint64_t));
    }

    // Iterate a table's live (key,value) slots from C++. ALWAYS use this (or tableLiveSlot) instead
    // of a raw `for (i in [0,capacity)) if (hashes[i] > KILLED)` scan: a packed table stores 64-bit
    // hashes (PackedPolicy), so a 32-bit `((TableHashKey*)hashes)[i]` read misindexes it. fn is
    // called as fn(const TK & key, const TV & value); TV's size must match the table's value stride.
    template <typename TK, typename TV, typename Fn>
    __forceinline void table_for_each ( const Table & tab, Fn && fn ) {
        auto pk = (const TK *) tab.keys;
        auto pv = (const TV *) tab.data;
        for ( uint64_t i=0, n=tab.capacity; i!=n; ++i ) {
            if ( tableLiveSlot(tab, i) ) fn(pk[i], pv[i]);
        }
    }

    // Per-key-type policy for the PACKED (small-table) regime: how a packed slot's hash is compared
    // on find, stored on insert, moved on swap-remove, and read back for promotion, plus how wide the
    // packed hash array is (`hashBytes`) and whether one exists at all (`storesHash`). TableHash
    // delegates here so its packed paths stay key-agnostic; reserveInternal records `!storesHash` in
    // the table's `tableNoHash` flag so the type-erased free/GC size math can read the width.
    //  - Workhorse keys (this generic): storesHash=false, hashBytes=0 — NO per-slot hash. find
    //    compares key DATA over the dense [0,size) prefix; promotion recomputes the hash from the key.
    //  - String keys (specialization): storesHash=true, 64-bit hash. find compares the full 64-bit
    //    hash EXACTLY — no strcmp, and a 64-bit collision is treated as impossible.
    template <typename KeyType>
    struct PackedPolicy {
        static constexpr bool storesHash = false;     // non-string packed: no per-slot hash at all
        static constexpr uint32_t hashBytes = 0;
        static __forceinline int64_t find ( const Table & tab, const KeyType & key, uint64_t ) {
            auto pKeys = (const KeyType *) tab.keys;
            uint32_t sz = (uint32_t) tab.size;
            for ( uint32_t i=0; i!=TABLE_MAX_LINEAR_CAPACITY; ++i ) {
                if ( i<sz && KeyCompare<KeyType>()(pKeys[i], key) ) return (int64_t) i;
            }
            return -1;
        }
        static __forceinline void insertHash ( Table &, uint64_t, uint64_t ) {}
        static __forceinline void moveHash ( Table &, uint64_t, uint64_t ) {}
        static __forceinline void clearHash ( Table &, uint64_t ) {}
        // No stored hash: re-derive it from the key to re-bucket into the large target on promotion.
        static __forceinline uint64_t promoteHash ( const Table &, uint64_t, const KeyType & key, Context * ctx ) {
            return hash_function(*ctx, key);
        }
    };

    template <>
    struct PackedPolicy<char *> {
        static constexpr bool storesHash = true;
        static constexpr uint32_t hashBytes = sizeof(uint64_t);
        static __forceinline int64_t find ( const Table & tab, char * const &, uint64_t hash ) {
            // All-8 scan: the tail is always 0 (alloc / erase / table_clear clear the full 64-bit
            // width) and a real hash is >1, so the tail never matches — no size guard needed.
            // loadHash64 reads unaligned — see its definition for why a raw (uint64_t*) cast here
            // faults under clang -O3 on linux/wasm.
            for ( uint32_t i=0; i!=TABLE_MAX_LINEAR_CAPACITY; ++i ) {
                if ( loadHash64(tab.hashes, i)==hash ) return (int64_t) i;
            }
            return -1;
        }
        static __forceinline void insertHash ( Table & tab, uint64_t slot, uint64_t hash ) {
            storeHash64(tab.hashes, slot, hash);  // full 64-bit; hash_blockz64 already clamps 0/1
        }
        static __forceinline void moveHash ( Table & tab, uint64_t to, uint64_t from ) {
            storeHash64(tab.hashes, to, loadHash64(tab.hashes, from));
        }
        static __forceinline void clearHash ( Table & tab, uint64_t slot ) {
            storeHash64(tab.hashes, slot, 0);
        }
        static __forceinline uint64_t promoteHash ( const Table & tab, uint64_t slot, char * const &, Context * ) {
            return loadHash64(tab.hashes, slot);
        }
    };

    template <>
    struct PackedPolicy<const char *> {
        static constexpr bool storesHash = true;
        static constexpr uint32_t hashBytes = sizeof(uint64_t);
        static __forceinline int64_t find ( const Table & tab, const char * const &, uint64_t hash ) {
            for ( uint32_t i=0; i!=TABLE_MAX_LINEAR_CAPACITY; ++i ) {
                if ( loadHash64(tab.hashes, i)==hash ) return (int64_t) i;
            }
            return -1;
        }
        static __forceinline void insertHash ( Table & tab, uint64_t slot, uint64_t hash ) {
            storeHash64(tab.hashes, slot, hash);
        }
        static __forceinline void moveHash ( Table & tab, uint64_t to, uint64_t from ) {
            storeHash64(tab.hashes, to, loadHash64(tab.hashes, from));
        }
        static __forceinline void clearHash ( Table & tab, uint64_t slot ) {
            storeHash64(tab.hashes, slot, 0);
        }
        static __forceinline uint64_t promoteHash ( const Table & tab, uint64_t slot, const char * const &, Context * ) {
            return loadHash64(tab.hashes, slot);
        }
    };

    template <typename KeyType>
    class TableHash {
        Context *   context = nullptr;
        uint32_t    valueTypeSize = 0;
        enum {
            minCapacity = 8
        };
        static_assert(minCapacity == TABLE_MAX_LINEAR_CAPACITY,
            "PackedPolicy scans a fixed TABLE_MAX_LINEAR_CAPACITY slots, so every packed table must be exactly that many slots");
    public:
        TableHash () = delete;
        TableHash ( const TableHash & ) = delete;
        TableHash ( Context * ctx, uint32_t vs ) : context(ctx), valueTypeSize(vs) {}

        __forceinline int64_t find ( const Table & tab, KeyType key, uint64_t hash ) const {
            DAS_ASSERT(hash>1);
            if ( tab.capacity==0 ) return -1;
            auto pKeys = (const KeyType *) tab.keys;
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string: packed (cap<=MAX) or large open-addressed
                if ( tab.capacity <= TABLE_MAX_LINEAR_CAPACITY ) {  // packed: exact 64-bit hash
                    return PackedPolicy<KeyType>::find(tab, key, hash);
                }
                uint64_t mask = tab.capacity - 1;
                uint64_t index = hash & mask;
                auto hashKey = hashToHashKey(TableHashKey(hash));
                auto pHashes = tab.hashes;
                while ( true ) {
                    auto kh = pHashes[index];
                    if ( kh==HASH_EMPTY64 ) {
                        return -1;
                    } else if ( kh==hashKey && KeyCompare<KeyType>()(pKeys[index],key) ) {
                        return (int64_t) index;
                    }
                    index = (index + 1) & mask;
                }
            } else {  // non-string: always open-addressed (1-byte control + direct key compare)
                uint64_t mask = tab.capacity - 1;
                uint64_t index = hash & mask;
                auto pCtrl = (const uint8_t *) tab.hashes;
                while ( true ) {
                    auto c = pCtrl[index];
                    if ( c==CTRL_EMPTY ) {
                        return -1;
                    } else if ( c==CTRL_OCCUPIED && KeyCompare<KeyType>()(pKeys[index],key) ) {
                        return (int64_t) index;
                    }
                    index = (index + 1) & mask;
                }
            }
        }

        __forceinline int64_t reserve ( Table & tab, KeyType key, uint64_t hash, LineInfo * at = nullptr ) {
            DAS_ASSERT(hash>1);
            // A promote/alloc re-dispatches by looping back here, not by a recursive call:
            // gcc -flto rejects a recursive always_inline function (MSVC tolerates it).
          retry:
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string: packed regime for cap<=MAX
                if ( tab.capacity <= TABLE_MAX_LINEAR_CAPACITY ) {  // packed: dense, load factor 1.0
                    if ( tab.capacity != 0 ) {  // dedup against existing (skip on an unallocated table)
                        int64_t idx = PackedPolicy<KeyType>::find(tab, key, hash);  // exact (no confirm needed)
                        if ( idx>=0 ) return idx;
                    }
                    if ( tab.isLocked() ) context->throw_error_at(at, "can't insert into locked table");
                    // A full packed table promotes to open addressing. *4 gives the hashed table
                    // load-factor headroom so it does not immediately re-grow (cap*2 would land exactly
                    // on the 0.5 trigger). cap-0 -> first allocation (stays packed).
                    if ( tab.size >= tab.capacity ) {
                        uint64_t newCapacity = (tab.capacity == 0) ? minCapacity : tab.capacity*4;
                        reserveInternal(tab, newCapacity, at);
                        goto retry;  // re-dispatch: now hashed (or the freshly allocated packed table)
                    }
                    uint64_t i = tab.size;
                    PackedPolicy<KeyType>::insertHash(tab, i, hash);
                    ((KeyType *) tab.keys)[i] = key;
                    tab.size++;
                    return (int64_t) i;
                }
            } else {  // non-string: open-addressed from the first insert (no packed regime)
                if ( tab.capacity == 0 ) {
                    if ( tab.isLocked() ) context->throw_error_at(at, "can't insert into locked table");
                    reserveInternal(tab, minCapacity, at);  // cap-0 -> open-addressed minCapacity slots
                    goto retry;
                }
            }
            if ( tab.size >= (tab.capacity/2) ) grow(tab, at);
            else if ( (tab.capacity-tab.size)/2 < tab.tombstones ) rehash(tab, at);
            uint64_t mask = tab.capacity - 1;
            uint64_t index = hash & mask;
            uint64_t insertI = ~uint64_t(0);
            auto pKeys = (KeyType *) tab.keys;
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string: 32-bit hash
                auto hashKey = hashToHashKey(TableHashKey(hash));
                auto pHashes = tab.hashes;
                while ( true ) {
                    auto kh = pHashes[index];
                    if (kh == HASH_EMPTY64 ) {
                        if ( tab.isLocked() ) context->throw_error_at(at, "can't insert into locked table");
                        if ( insertI != ~uint64_t(0) ) {
                            index = insertI;
                            tab.tombstones--;
                        }
                        pHashes[index] = hashKey;
                        pKeys[index] = key;
                        tab.size++;
                        return (int64_t)index;
                    } else if (kh == HASH_KILLED64) {
                        if ( insertI == ~uint64_t(0) ) insertI = index;
                    } else if (kh == hashKey && KeyCompare<KeyType>()(pKeys[index], key)) {
                        return (int64_t)index;
                    }
                    index = (index + 1) & mask;
                }
            } else {  // non-string: 1-byte control
                auto pCtrl = (uint8_t *) tab.hashes;
                while ( true ) {
                    auto c = pCtrl[index];
                    if (c == CTRL_EMPTY ) {
                        if ( tab.isLocked() ) context->throw_error_at(at, "can't insert into locked table");
                        if ( insertI != ~uint64_t(0) ) {
                            index = insertI;
                            tab.tombstones--;
                        }
                        pCtrl[index] = CTRL_OCCUPIED;
                        pKeys[index] = key;
                        tab.size++;
                        return (int64_t)index;
                    } else if (c == CTRL_TOMBSTONE) {
                        if ( insertI == ~uint64_t(0) ) insertI = index;
                    } else if (KeyCompare<KeyType>()(pKeys[index], key)) {  // c == CTRL_OCCUPIED
                        return (int64_t)index;
                    }
                    index = (index + 1) & mask;
                }
            }
        }

        // Insert a key the caller has ALREADY PROVEN absent from a packed table — the JIT
        // path runs the SIMD packed find inline, and on a miss it knows the key is not there,
        // so re-running PackedFind here (as reserve() does to dedup) is wasted work. This is
        // that shortcut: it skips the dedup scan and goes straight to the dense append, or
        // promotes to open addressing first if the packed table is full. PRECONDITION: the
        // table is packed and the key is genuinely absent — 0 < capacity <= the linear cap,
        // and no live slot carries this key. Calling it on a non-packed table, or
        // with a key that is actually present, double-inserts. Not a general reserve; use
        // reserve() unless you just did the packed find yourself and it missed.
        __forceinline int64_t reserveAfterPackedMiss ( Table & tab, KeyType key, uint64_t hash, LineInfo * at = nullptr ) {
            DAS_ASSERT(hash>1);
            DAS_ASSERT(tab.capacity!=0 && tab.capacity<=TABLE_MAX_LINEAR_CAPACITY);
            if ( tab.isLocked() ) context->throw_error_at(at, "can't insert into locked table");
            if ( tab.size >= tab.capacity ) {
                // full packed table -> promote to open addressing, then insert via the hashed
                // path (no packed dedup re-run, since it is now hashed).
                reserveInternal(tab, tab.capacity*4, at);
                return reserve(tab, key, hash, at);
            }
            uint64_t i = tab.size;
            PackedPolicy<KeyType>::insertHash(tab, i, hash);
            ((KeyType *) tab.keys)[i] = key;
            tab.size++;
            return (int64_t) i;
        }

        __forceinline int64_t erase ( Table & tab, KeyType key, uint64_t hash ) {
            DAS_ASSERT(hash>1);
            if ( tab.capacity==0 ) return -1;
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string only: packed swap-remove
                if ( tab.capacity <= TABLE_MAX_LINEAR_CAPACITY ) {  // packed: locate, then swap-remove the last live slot into the hole
                    int64_t fidx = PackedPolicy<KeyType>::find(tab, key, hash);  // exact
                    if ( fidx<0 ) return -1;
                    auto pKeys = (KeyType *) tab.keys;
                    uint64_t i = (uint64_t) fidx;
                    uint64_t last = tab.size - 1;
                    if ( i != last ) {
                        PackedPolicy<KeyType>::moveHash(tab, i, last);
                        pKeys[i] = pKeys[last];
                        memcpy(tab.data + i*valueTypeSize, tab.data + last*valueTypeSize, valueTypeSize);
                    }
                    PackedPolicy<KeyType>::clearHash(tab, last);
                    memset(tab.data + last*valueTypeSize, 0, valueTypeSize);
                    tab.size--;
                    return (int64_t) i;
                }
            }
            auto pKeys = (KeyType *) tab.keys;
            uint64_t mask = tab.capacity - 1;
            uint64_t index = hash & mask;
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string: 32-bit hash
                auto hashKey = hashToHashKey(TableHashKey(hash));
                auto pHashes = tab.hashes;
                while ( true ) {
                    auto kh = pHashes[index];
                    if ( kh==HASH_EMPTY64 ) {
                        return -1;
                    } else if ( kh==hashKey && KeyCompare<KeyType>()(pKeys[index],key) ) {
                        tab.size--;
                        tab.tombstones++;
                        pHashes[index] = HASH_KILLED64;
                        memset(tab.data + index*valueTypeSize, 0, valueTypeSize);
                        return (int64_t) index;
                    }
                    index = (index + 1) & mask;
                }
            } else {  // non-string: 1-byte control
                auto pCtrl = (uint8_t *) tab.hashes;
                while ( true ) {
                    auto c = pCtrl[index];
                    if ( c==CTRL_EMPTY ) {
                        return -1;
                    } else if ( c==CTRL_OCCUPIED && KeyCompare<KeyType>()(pKeys[index],key) ) {
                        tab.size--;
                        tab.tombstones++;
                        pCtrl[index] = CTRL_TOMBSTONE;
                        memset(tab.data + index*valueTypeSize, 0, valueTypeSize);
                        return (int64_t) index;
                    }
                    index = (index + 1) & mask;
                }
            }
        }

        bool grow ( Table & tab, LineInfo * at ) {
            // Guard against capacity*2 overflow — at uint64_t the wrap-to-zero produces
            // a non-power-of-two `newCapacity` that would trip reserveInternal's verify
            // and, before that, make `mask = capacity - 1` underflow into all-ones.
            if ( tab.capacity > (uint64_t(1) << 62) ) {
                context->throw_error_at(at, "table grow: capacity %llu exceeds 2^62", (unsigned long long)tab.capacity);
                return false;
            }
            uint64_t newCapacity = das::max(uint64_t(minCapacity), tab.capacity*2);
            return reserveInternal(tab, newCapacity, at);
        }

        bool rehash ( Table & tab, LineInfo * at ) {
            return reserveInternal(tab, tab.capacity, at);
        }

        bool reserve(Table & tab, uint64_t size, LineInfo * at ) {
            if (size <= tab.capacity)
              return true;

            // Guard against overflow before doubling — otherwise huge `size` (e.g. a negative
            // signed input cast to uint64) would infinite-loop here as newCapacity wraps to 0.
            if (size > (uint64_t(1) << 62)) {
              context->throw_error_at(at, "table reserve: size %llu exceeds 2^62", (unsigned long long)size);
              return false;
            }
            uint64_t newCapacity = das::max(uint64_t(minCapacity), tab.capacity*2);
            while (newCapacity < size)
            {
              newCapacity *= 2;
            }

            return reserveInternal(tab, newCapacity, at);
        }

    private:
        // Returns the slot index. Always finds a slot (called only during rehash on a fresh
        // pre-allocated table), so signed int64_t can hold any value the uint64 capacity allows.
        __forceinline int64_t insertNew ( Table & tab, uint64_t hash ) const {
            // TODO: take key under account and be less aggressive?
            DAS_ASSERT(hash>1);
            uint64_t mask = tab.capacity - 1;
            uint64_t index = hash & mask;
            // Called only on a freshly allocated (all-empty, no-tombstone) target during rehash.
            if constexpr ( PackedPolicy<KeyType>::storesHash ) {
                auto pHashes = tab.hashes;
                while ( true ) {
                    if ( pHashes[index]==HASH_EMPTY64 ) return (int64_t) index;
                    index = (index + 1) & mask;
                }
            } else {
                auto pCtrl = (const uint8_t *) tab.hashes;
                while ( true ) {
                    if ( pCtrl[index]==CTRL_EMPTY ) return (int64_t) index;
                    index = (index + 1) & mask;
                }
            }
        }

        bool reserveInternal(Table & tab, uint64_t newCapacity, LineInfo * at) {
            DAS_VERIFYF(newCapacity != 0 && (newCapacity & (newCapacity - 1)) == 0, "newCapacity must be a nonzero power of 2, and not %llu", (unsigned long long)newCapacity);
            if ( tab.magic!=0 || tab.lock!=0 ) {
                context->throw_error_at(at, "can't grow a locked table");
                return false;
            }
            Table newTab;
            // Hash-region width by (regime x key type): packed string = 64-bit hash, packed non-string
            // = none (PackedPolicy::hashBytes); large string = 32-bit hash, large non-string = 1-byte
            // control. Must agree with tableHashSlotBytes.
            bool newPacked = PackedPolicy<KeyType>::storesHash && newCapacity <= TABLE_MAX_LINEAR_CAPACITY;  // only string packs
            uint64_t newHashBytes = newPacked ? uint64_t(PackedPolicy<KeyType>::hashBytes)
                : (PackedPolicy<KeyType>::storesHash ? uint64_t(sizeof(TableHashKey)) : uint64_t(1));
            uint64_t perSlot = uint64_t(valueTypeSize) + uint64_t(sizeof(KeyType)) + newHashBytes;
            if ( perSlot && newCapacity > UINT64_MAX / perSlot ) {
                context->throw_error_ex("can't grow table, capacity*perSlot overflows uint64 [capacity=%llu]", (unsigned long long)newCapacity);
                return false;
            }
            uint64_t memSize64 = newCapacity * perSlot;
            const char * prev_comment = tab.data ? context->heap->get_comment(tab.data) : nullptr;
            newTab.data = (char *) context->allocate(memSize64, at);
            context->heap->mark_comment(newTab.data, prev_comment ? prev_comment : "table");
            newTab.keys = newTab.data + newCapacity * valueTypeSize;
            newTab.hashes = (TableHashKey *)(newTab.keys + newCapacity * sizeof(KeyType));
            newTab.size = tab.size;
            newTab.capacity = newCapacity;
            newTab.lock = tab.lock;
            newTab.magic = 0;
            newTab.flags = tab.flags;
            // Record the per-slot-hash representation on the instance so the type-erased
            // tableHashSlotBytes / tableLiveSlot can read it without a KeyType. Constant per
            // KeyType, so this is idempotent across reallocs (and corrects a fresh table whose
            // flags started at 0).
            newTab.tableNoHash = !PackedPolicy<KeyType>::storesHash;
            newTab.tombstones = 0;
            if ( valueTypeSize ) memset(newTab.data, 0, size_t(newCapacity)*size_t(valueTypeSize));
            auto pHashes = newTab.hashes;
            memset(pHashes, 0, size_t(newCapacity) * size_t(newHashBytes));
            if ( tab.size ) {
                // Entries are only ever copied into a LARGE (open-addressed) target: the sole packed
                // target is the cap-0 -> minCapacity first allocation, which has no entries. So the
                // target store is the large form for KeyType (string = 32-bit hash, non-string =
                // 1-byte control). The SOURCE may be packed (dense [0,size), hash via promoteHash) or
                // large (string: 32-bit probe scan; non-string: control scan + recomputed hash).
                auto pKeys = (KeyType *) newTab.keys;
                auto pOldValues = tab.data;
                auto pValues = newTab.data;
                auto pOldKeys = (const KeyType *) tab.keys;
                if constexpr ( PackedPolicy<KeyType>::storesHash ) {  // string source: packed (dense) or large (32-bit hashes)
                    if ( tab.capacity <= TABLE_MAX_LINEAR_CAPACITY ) {  // packed string source: dense [0,size)
                        for ( uint64_t i=0, is=tab.size; i!=is; ++i ) {
                            uint64_t hash = PackedPolicy<KeyType>::promoteHash(tab, i, pOldKeys[i], context);
                            int64_t index = insertNew(newTab, hash);
                            pHashes[index] = hashToHashKey(TableHashKey(hash));
                            pKeys[index] = pOldKeys[i];
                            memcpy ( pValues + index*valueTypeSize, pOldValues + i*valueTypeSize, valueTypeSize );
                        }
                    } else {  // large string source: 32-bit hashes
                        auto pOldHashes = tab.hashes;
                        for ( uint64_t i=0, is=tab.capacity; i!=is; ++i ) {
                            auto hash = pOldHashes[i];
                            if ( hash>HASH_KILLED64 ) {
                                int64_t index = insertNew(newTab, hash);
                                pHashes[index] = hash;
                                pKeys[index] = pOldKeys[i];
                                memcpy ( pValues + index*valueTypeSize, pOldValues + i*valueTypeSize, valueTypeSize );
                            }
                        }
                    }
                } else {  // non-string source: always open-addressed (1-byte control), recompute the hash to re-bucket
                    auto pOldCtrl = (const uint8_t *) tab.hashes;
                    for ( uint64_t i=0, is=tab.capacity; i!=is; ++i ) {
                        if ( pOldCtrl[i] > CTRL_TOMBSTONE ) {
                            uint64_t hash = hash_function(*context, pOldKeys[i]);
                            int64_t index = insertNew(newTab, hash);
                            ((uint8_t *) newTab.hashes)[index] = CTRL_OCCUPIED;
                            pKeys[index] = pOldKeys[i];
                            memcpy ( pValues + index*valueTypeSize, pOldValues + i*valueTypeSize, valueTypeSize );
                        }
                    }
                }
            }
            if (tab.capacity && !context->verySafeContext) {
                uint64_t oldHashBytes = (PackedPolicy<KeyType>::storesHash && tab.capacity <= TABLE_MAX_LINEAR_CAPACITY) ? uint64_t(PackedPolicy<KeyType>::hashBytes)
                    : (PackedPolicy<KeyType>::storesHash ? uint64_t(sizeof(TableHashKey)) : uint64_t(1));
                uint64_t oldSize = tab.capacity * (uint64_t(valueTypeSize) + uint64_t(sizeof(KeyType)) + oldHashBytes);
                context->free(tab.data, oldSize, at);
            }
            swap ( newTab, tab );
            return true;
        }
    };
}



