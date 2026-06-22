#pragma once

// daslang_hash_map / daslang_hash_set — in-tree open-addressing hashmap.
//
// Modeled on include/daScript/simulate/runtime_table.h. Three parallel vectors
// (hashes, keys, values) with linear probing + tombstones. Default ctor allocates
// zero heap — unlike ska::flat_hash_map whose empty_default_table() allocates
// on construction and cascades under DAS_THREAD_LOCAL wrappers.
//
// Probe hash sentinels match anyhash.h: 0 = EMPTY, 1 = KILLED. User-supplied
// hashes that collide with sentinels are remapped to the FNV-64 prime.
//
// K and V must be default-constructible: rehash allocates `das::vector<K>(new_cap)` /
// `das::vector<V>(new_cap)` and erase assigns `K{}` / `V{}` over killed slots. This
// matches runtime_table.h's design and daslang's actual call sites (all AST /
// pointer / primitive types). Supporting non-default-constructible K/V would need
// uninitialized slot storage with placement-new-on-demand — out of scope here.

// This header is pulled in by das_config.h, which already includes <vector>, <utility>,
// <functional>, <string>, <type_traits>, etc. before reaching us. Platform.h's
// __forceinline / NO_ASAN_INLINE / DAS_SUPPRESS_UB are NOT yet defined at this point,
// so we provide a local __forceinline fallback below and inline the FNV-64 hash
// directly (cannot pull in anyhash.h for the same reason). The same constraint
// is why at() routes its missing-key path through das::das_throw (declared by
// das_config.h itself, just above us) rather than raw `throw`: games embedding
// daslang frequently disable C++ exceptions entirely.
#if !defined(_MSC_VER) && !defined(__forceinline)
    #define __forceinline inline __attribute__((always_inline))
#endif

namespace das {

namespace daslang_hash_map_detail {

    static constexpr uint64_t HASH_EMPTY  = 0;
    static constexpr uint64_t HASH_KILLED = 1;
    static constexpr uint64_t HASH_REMAP  = 1099511628211ull;   // FNV-64 prime; matches anyhash.h

    static __forceinline uint64_t to_hash_key ( uint64_t h ) noexcept {
        return h <= HASH_KILLED ? HASH_REMAP : h;
    }

    // default first-grow capacity. Mirrors runtime_table.h::minCapacity.
    static constexpr size_t minCapacity = 8;

    // next_probe_slot — shared increment to keep probe step identical across
    // find/insert/erase/rehash. `mask` must be (capacity - 1) with capacity a power of 2.
    static __forceinline uint64_t next_probe_slot ( uint64_t index, uint64_t mask ) noexcept {
        return (index + 1) & mask;
    }

    // Inline copy of anyhash.h's hash_blockz64 — FNV-64 over a null-terminated byte
    // sequence. Duplicated here because we cannot include anyhash.h (it depends on
    // platform.h macros not yet set up at our inclusion point). Gated on DAS_SAFE_HASH
    // the same way anyhash.h is: safe path breaks on block[0]==0 before touching
    // block[1]; fast path does a 2-byte read and tolerates the 1-byte overread.
    //
    // Under ASAN, default DAS_SAFE_HASH to 1 — the fast path's overread crosses the
    // das::string allocation boundary (heap allocs are sized to chars+null exactly),
    // which ASAN flags. Cannot use NO_ASAN_INLINE here because this header is pulled
    // in before platform.h sets it up.
    //
    // __has_feature is Clang-only; on GCC the preprocessor still has to PARSE the
    // RHS of `&&` even though `defined(__has_feature)` is false (short-circuit is
    // semantic, not lexical), so guard it with the standard portability shim.
    #ifndef __has_feature
        #define __has_feature(x) 0
    #endif
    #ifndef DAS_SAFE_HASH
        #if defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
            #define DAS_SAFE_HASH 1
        #else
            #define DAS_SAFE_HASH 0
        #endif
    #endif
    static inline uint64_t hash_blockz64 ( const uint8_t * block ) noexcept {
        const uint64_t FNV_offset_basis = 14695981039346656037ull;
        const uint64_t FNV_prime        = 1099511628211ull;
        if ( !block ) return FNV_offset_basis;
        uint64_t h = FNV_offset_basis;
#if DAS_SAFE_HASH
        while ( true ) {
            uint64_t v = uint64_t(block[0]);
            if ( v == 0 ) break;
            v |= uint64_t(block[1]) << 8;
            h ^= v;
            h *= FNV_prime;
            if ( v < 0x100 ) break;
            block += 2;
        }
#else
        while ( true ) {
            uint64_t v = uint64_t(block[0]) | (uint64_t(block[1]) << 8);
            if ( (v & 0xff) == 0 ) break;
            h ^= v;
            h *= FNV_prime;
            if ( v < 0x100 ) break;
            block += 2;
        }
#endif
        return h <= HASH_KILLED ? HASH_REMAP : h;
    }

} // namespace daslang_hash_map_detail

//
//  daslang_hash<T> — default hasher for daslang_hash_map / daslang_hash_set.
//
//  * integral types: x + 2 — trivial shift past the EMPTY/KILLED sentinels.
//    AST hash keys are already hash-quality (wyhash outputs used as keys),
//    so no further mixing is needed. to_hash_key still guards collisions
//    with 0/1 just in case.
//  * null-terminated C strings and das::string: hash_blockz64 (FNV-64 from anyhash.h).
//  * anything else: das::hash<T> fallback.
//

template <class T, class Enable = void>
struct daslang_hash {
    size_t operator () ( const T & x ) const { return das::hash<T>{}(x); }
};

template <class T>
struct daslang_hash<T, das::enable_if_t<das::is_integral<T>::value>> {
    __forceinline size_t operator () ( T x ) const noexcept {
        return size_t(x) + 2;
    }
};

// Pointer keys: raw `ptr >> 4` clusters catastrophically on tight-stride
// allocations (1M short heap strings: 36× regression). das::hash<T*> (raw ptr)
// is only mediocre. Fibonacci hashing — shift-out-alignment + golden-ratio
// multiply — is the winner in our benchmarks:
//   N=100: 0.48× ska   N=10k: 0.77× ska   N=1M: 0.87× ska
// Cost: 1 shift + 1 imul (~4 cycles). The multiply scatters sparse low-bit
// entropy (common for freelist-allocated pointers) across all 64 bits so the
// downstream `& (cap-1)` mask selects from well-mixed middle bits.
// Full specializations below (char*, const char*) override this for C strings.
template <class T>
struct daslang_hash<T *, void> {
    __forceinline size_t operator () ( T * p ) const noexcept {
        return (reinterpret_cast<size_t>(p) >> 4) * size_t(0x9E3779B97F4A7C15ull);
    }
};

template <>
struct daslang_hash<das::string> {
    __forceinline size_t operator () ( const das::string & s ) const noexcept {
        return size_t(daslang_hash_map_detail::hash_blockz64(reinterpret_cast<const uint8_t *>(s.c_str())));
    }
};

template <>
struct daslang_hash<const char *> {
    __forceinline size_t operator () ( const char * s ) const noexcept {
        return size_t(daslang_hash_map_detail::hash_blockz64(reinterpret_cast<const uint8_t *>(s)));
    }
};

template <>
struct daslang_hash<char *> {
    __forceinline size_t operator () ( char * s ) const noexcept {
        return size_t(daslang_hash_map_detail::hash_blockz64(reinterpret_cast<const uint8_t *>(s)));
    }
};

//
//  daslang_hash_map
//

template <class K, class V, class Hash = daslang_hash<K>, class KeyEqual = das::equal_to<K>>
class daslang_hash_map {
    // das::vector<bool> is specialized and returns proxy references — incompatible
    // with our iterator's V& contract. Use uint8_t storage for V=bool and cast at
    // the access points. bool and uint8_t share size and representation on every
    // platform daslang ships on.
    //
    // Strict-aliasing note: reinterpret_cast<bool&>(uint8_t&) is technically UB by
    // the letter of the standard (bool is not an aliasing-compatible type for
    // uint8_t, and bool values outside {0,1} are trap representations). In practice
    // MSVC/gcc/clang all compile it correctly — we only ever store 0 or 1 through
    // the bool side, and we never take addresses across the boundary. This is a
    // conscious trade vs (a) storing real bool + dealing with das::vector<bool>'s proxy
    // iterators, or (b) memcpy'ing every read/write. Revisit if a sanitizer fires.
    static constexpr bool is_bool_value = das::is_same<V, bool>::value;
    using value_storage_t = typename das::conditional<is_bool_value, uint8_t, V>::type;

    das::vector<uint64_t>       hashes_;   // HASH_EMPTY / HASH_KILLED / remapped-hash
    das::vector<K>              keys_;
    das::vector<value_storage_t> values_;
    size_t                 size_       = 0;
    size_t                 tombstones_ = 0;
    Hash                   hash_;
    KeyEqual               key_equal_;

    __forceinline V &       val_at ( size_t i )       noexcept { return reinterpret_cast<V       &>(values_[i]); }
    __forceinline const V & val_at ( size_t i ) const noexcept { return reinterpret_cast<const V &>(values_[i]); }

public:
    using key_type    = K;
    using mapped_type = V;
    using size_type   = size_t;
    using hasher      = Hash;
    using key_equal   = KeyEqual;

    // returned by iterator::operator* — aggregate with two reference members
    // so C++17 structured bindings (`auto [k, v] = *it`) bind directly to the
    // underlying container slots without going through das::tuple_size.
    // `first` is a NON-const K& — matches ska::flat_hash_map's `value_type = das::pair<K,V>`
    // rather than das::unordered_map's `das::pair<const K, V>`. Mutating the key through
    // this reference would corrupt the hash; callers must not do that (matches the
    // ska contract used throughout daslang). Converts implicitly to das::pair<K,V> so
    // code like `auto kv = *it; m2.insert(kv);` still compiles.
    struct reference {
        K & first;
        V & second;
        operator das::pair<K, V> () const { return { first, second }; }
    };
    struct const_reference {
        const K & first;
        const V & second;
        operator das::pair<K, V> () const { return { first, second }; }
    };

    class const_iterator;

    // Iterator caches a `reference` (pair of K&, V&) inside its own storage and
    // returns it by lvalue ref. Necessary because `reference` has reference members
    // and is therefore not assignable — we placement-new into the cache on every
    // operator*() call. Trivially destructible so we never need an explicit dtor.
    // This is what makes `for (auto & kv : map)` and `for (auto & [k,v] : map)` bind
    // under clang; MSVC was lenient about prvalue-to-lvalue-ref.
    class iterator {
    public:
        // Aliases come first so `reference` below refers to the alias, not the
        // enclosing daslang_hash_map::reference via injected-class-name lookup.
        // GCC's [basic.scope.class] diagnostic (-fpermissive) fires otherwise,
        // because the name's meaning would differ between enclosing-scope lookup
        // (outer struct) and complete-class-scope lookup (the alias).
        using iterator_category = das::forward_iterator_tag;
        // das::pair<K, V> (not das::pair<const K, V>): matches ska's value_type contract and
        // matches what operator* actually yields (non-const K& inside reference).
        using value_type        = das::pair<K, V>;
        using difference_type   = ptrdiff_t;
        using pointer           = daslang_hash_map::reference *;
        using reference         = daslang_hash_map::reference;
    private:
        daslang_hash_map * owner_ = nullptr;
        size_t             index_ = 0;
        mutable typename das::aligned_storage<sizeof(reference), alignof(reference)>::type ref_storage_;
        friend class daslang_hash_map;
        friend class const_iterator;
    public:
        iterator () noexcept = default;
        iterator ( daslang_hash_map * o, size_t i ) noexcept : owner_(o), index_(i) {}

        reference & operator *  () const {
            return *::new (static_cast<void*>(&ref_storage_))
                reference{ owner_->keys_[index_], owner_->val_at(index_) };
        }
        reference * operator -> () const { return &**this; }

        iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] <= daslang_hash_map_detail::HASH_KILLED ) ++index_;
            return *this;
        }
        iterator   operator ++ (int) { iterator t = *this; ++(*this); return t; }

        bool operator == ( const iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const iterator & o ) const noexcept { return !(*this == o); }
    };

    class const_iterator {
    public:
        using iterator_category = das::forward_iterator_tag;
        using value_type        = das::pair<K, V>;
        using difference_type   = ptrdiff_t;
        using pointer           = const daslang_hash_map::const_reference *;
        using reference         = daslang_hash_map::const_reference;
    private:
        const daslang_hash_map * owner_ = nullptr;
        size_t                   index_ = 0;
        mutable typename das::aligned_storage<sizeof(const_reference), alignof(const_reference)>::type ref_storage_;
        friend class daslang_hash_map;
    public:
        const_iterator () noexcept = default;
        const_iterator ( const daslang_hash_map * o, size_t i ) noexcept : owner_(o), index_(i) {}
        const_iterator ( const iterator & it ) noexcept : owner_(it.owner_), index_(it.index_) {}

        reference & operator *  () const {
            return *::new (static_cast<void*>(&ref_storage_))
                const_reference{ owner_->keys_[index_], owner_->val_at(index_) };
        }
        reference * operator -> () const { return &**this; }

        const_iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] <= daslang_hash_map_detail::HASH_KILLED ) ++index_;
            return *this;
        }
        const_iterator   operator ++ (int) { const_iterator t = *this; ++(*this); return t; }

        bool operator == ( const const_iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const const_iterator & o ) const noexcept { return !(*this == o); }
    };

    daslang_hash_map () = default;
    daslang_hash_map ( const daslang_hash_map & ) = default;
    daslang_hash_map ( daslang_hash_map && ) noexcept = default;
    daslang_hash_map & operator = ( const daslang_hash_map & ) = default;
    daslang_hash_map & operator = ( daslang_hash_map && ) noexcept = default;
    ~ daslang_hash_map () = default;

    daslang_hash_map ( das::initializer_list<das::pair<K, V>> il ) {
        reserve(il.size());
        for ( const auto & kv : il ) insert(kv);
    }

    //
    //  capacity
    //

    bool         empty        () const noexcept { return size_ == 0; }
    size_type    size         () const noexcept { return size_; }
    size_type    bucket_count () const noexcept { return hashes_.size(); }

    void reserve ( size_type n ) {
        if ( n == 0 ) return;
        size_t needed = daslang_hash_map_detail::minCapacity;
        while ( needed < n * 2 ) needed *= 2;    // keep load-factor <= 0.5
        if ( needed > hashes_.size() ) rehash_into(needed);
    }

    void clear () noexcept {
        hashes_.clear();
        keys_.clear();
        values_.clear();
        size_ = 0;
        tombstones_ = 0;
    }

    void shrink_to_fit () {
        hashes_.shrink_to_fit();
        keys_.shrink_to_fit();
        values_.shrink_to_fit();
    }

    //
    //  iteration (skips EMPTY / KILLED slots)
    //

    iterator       begin ()        { return iterator      {this, first_live_index()}; }
    const_iterator begin () const  { return const_iterator{this, first_live_index()}; }
    const_iterator cbegin () const { return begin(); }
    iterator       end   ()        { return iterator      {this, hashes_.size()}; }
    const_iterator end   () const  { return const_iterator{this, hashes_.size()}; }
    const_iterator cend   () const { return end(); }

    //
    //  lookup
    //

    iterator find ( const K & key ) {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : iterator{this, idx};
    }
    const_iterator find ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : const_iterator{this, idx};
    }
    size_type count    ( const K & key ) const { return find_index(key, hash_(key)) == SIZE_MAX ? 0 : 1; }
    bool      contains ( const K & key ) const { return find_index(key, hash_(key)) != SIZE_MAX; }

    V & at ( const K & key ) {
        const size_t idx = find_index(key, hash_(key));
        if ( idx == SIZE_MAX ) das::das_throw("daslang_hash_map::at");
        return val_at(idx);
    }
    const V & at ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        if ( idx == SIZE_MAX ) das::das_throw("daslang_hash_map::at");
        return val_at(idx);
    }

    V & operator [] ( const K & key ) {
        return val_at(reserve_slot(key, hash_(key)).first);
    }
    V & operator [] ( K && key ) {
        auto h = hash_(key);
        return val_at(reserve_slot(das::move(key), h).first);
    }

    //
    //  modification
    //

    das::pair<iterator, bool> insert ( const das::pair<K, V> & kv ) {
        auto r = reserve_slot(kv.first, hash_(kv.first));
        if ( r.second ) val_at(r.first) = kv.second;
        return { iterator{this, r.first}, r.second };
    }
    das::pair<iterator, bool> insert ( das::pair<K, V> && kv ) {
        auto h = hash_(kv.first);
        auto r = reserve_slot(das::move(kv.first), h);
        if ( r.second ) val_at(r.first) = das::move(kv.second);
        return { iterator{this, r.first}, r.second };
    }

    template <class... Args>
    das::pair<iterator, bool> emplace ( Args &&... args ) {
        das::pair<K, V> kv(das::forward<Args>(args)...);
        return insert(das::move(kv));
    }

    template <class... Args>
    das::pair<iterator, bool> try_emplace ( const K & key, Args &&... args ) {
        auto r = reserve_slot(key, hash_(key));
        if ( r.second ) val_at(r.first) = V(das::forward<Args>(args)...);
        return { iterator{this, r.first}, r.second };
    }
    template <class... Args>
    das::pair<iterator, bool> try_emplace ( K && key, Args &&... args ) {
        auto h = hash_(key);
        auto r = reserve_slot(das::move(key), h);
        if ( r.second ) val_at(r.first) = V(das::forward<Args>(args)...);
        return { iterator{this, r.first}, r.second };
    }

    size_type erase ( const K & key ) {
        return erase_index(key, hash_(key)) ? 1 : 0;
    }
    iterator erase ( const_iterator it ) {
        const size_t idx = it.index_;
        hashes_[idx] = daslang_hash_map_detail::HASH_KILLED;
        keys_  [idx] = K{};
        val_at(idx)  = V{};
        --size_;
        ++tombstones_;
        iterator nxt{this, idx};
        ++nxt;
        return nxt;
    }

private:

    size_t first_live_index () const noexcept {
        const size_t cap = hashes_.size();
        const uint64_t * pH = hashes_.data();
        for ( size_t i = 0; i < cap; ++i ) {
            if ( pH[i] > daslang_hash_map_detail::HASH_KILLED ) return i;
        }
        return cap;
    }

    // returns index of key if present, SIZE_MAX if not. capacity == 0 path returns SIZE_MAX.
    // NOTE: body mirrors runtime_table.h::find — same loop form, same hoisting, same else-if
    // ladder. Keep them parallel so the compiler produces matching codegen.
    __forceinline size_t find_index ( const K & key, uint64_t raw_hash ) const {
        if ( hashes_.size() == 0 ) return SIZE_MAX;
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                return SIZE_MAX;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return size_t(index);
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    // returns (index, inserted?). Grows/rehashes as needed.
    // If inserted, slot is initialized with hash + key; caller is responsible for
    // writing the value. NOTE: probe loop mirrors runtime_table.h::reserve.
    template <class KFwd>
    __forceinline das::pair<size_t, bool> reserve_slot ( KFwd && key, uint64_t raw_hash ) {
        if      ( size_ >= hashes_.size() / 2 )                        grow();
        else if ( (hashes_.size() - size_) / 2 < tombstones_ )         rehash_same_capacity();
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        uint64_t insertI = uint64_t(-1);
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                if ( insertI != uint64_t(-1) ) {
                    index = insertI;
                    --tombstones_;
                }
                pHashes[index] = hashKey;
                pKeys  [index] = das::forward<KFwd>(key);
                ++size_;
                return { size_t(index), true };
            } else if ( kh == daslang_hash_map_detail::HASH_KILLED ) {
                if ( insertI == uint64_t(-1) ) insertI = index;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return { size_t(index), false };
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    __forceinline bool erase_index ( const K & key, uint64_t raw_hash ) {
        const size_t idx = find_index(key, raw_hash);
        if ( idx == SIZE_MAX ) return false;
        hashes_[idx] = daslang_hash_map_detail::HASH_KILLED;
        keys_  [idx] = K{};
        val_at(idx)  = V{};
        --size_;
        ++tombstones_;
        return true;
    }

    void grow () {
        const size_t new_cap = hashes_.empty()
            ? daslang_hash_map_detail::minCapacity
            : hashes_.size() * 2;
        rehash_into(new_cap);
    }
    void rehash_same_capacity () { rehash_into(hashes_.size()); }

    void rehash_into ( size_t new_cap ) {
        // new_cap must be a power of 2. grow()/rehash_same_capacity() uphold that.
        das::vector<uint64_t>        new_hashes(new_cap, daslang_hash_map_detail::HASH_EMPTY);
        das::vector<K>               new_keys(new_cap);
        das::vector<value_storage_t> new_values(new_cap);
        const uint64_t mask = new_cap - 1;
        for ( size_t i = 0, n = hashes_.size(); i < n; ++i ) {
            const uint64_t kh = hashes_[i];
            if ( kh <= daslang_hash_map_detail::HASH_KILLED ) continue;
            uint64_t j = kh & mask;
            while ( new_hashes[j] != daslang_hash_map_detail::HASH_EMPTY ) {
                j = daslang_hash_map_detail::next_probe_slot(j, mask);
            }
            new_hashes[j] = kh;
            new_keys  [j] = das::move(keys_  [i]);
            new_values[j] = das::move(values_[i]);
        }
        hashes_ = das::move(new_hashes);
        keys_   = das::move(new_keys);
        values_ = das::move(new_values);
        tombstones_ = 0;
    }
};

//
//  daslang_insert_only_hash_map — same layout as daslang_hash_map but with NO
//  erase API and NO tombstone tracking. Insertion-only / read-only callers
//  pay strictly less per-op work:
//   * reserve_slot's inner loop drops the HASH_KILLED branch + insertI tracking
//   * iterator skip condition is `== HASH_EMPTY` rather than `<= HASH_KILLED`
//   * no `tombstones_` counter, no rehash_same_capacity path
//   * find_index is identical to the regular version (already terminates on
//     EMPTY only — tombstones never extended probe chains for finds, only
//     for insert collision resolution)
//
//  Load factor cap is still 0.5 to keep find behavior matched with the
//  regular version for apples-to-apples benchmarking. Callers wanting denser
//  packing should use absl::flat_hash_map (0.875 cap) instead.
//

template <class K, class V, class Hash = daslang_hash<K>, class KeyEqual = das::equal_to<K>>
class daslang_insert_only_hash_map {
    static constexpr bool is_bool_value = das::is_same<V, bool>::value;
    using value_storage_t = typename das::conditional<is_bool_value, uint8_t, V>::type;

    das::vector<uint64_t>       hashes_;
    das::vector<K>              keys_;
    das::vector<value_storage_t> values_;
    size_t                 size_ = 0;
    Hash                   hash_;
    KeyEqual               key_equal_;

    __forceinline V &       val_at ( size_t i )       noexcept { return reinterpret_cast<V       &>(values_[i]); }
    __forceinline const V & val_at ( size_t i ) const noexcept { return reinterpret_cast<const V &>(values_[i]); }

public:
    using key_type    = K;
    using mapped_type = V;
    using size_type   = size_t;
    using hasher      = Hash;
    using key_equal   = KeyEqual;

    struct reference {
        K & first;
        V & second;
        operator das::pair<K, V> () const { return { first, second }; }
    };
    struct const_reference {
        const K & first;
        const V & second;
        operator das::pair<K, V> () const { return { first, second }; }
    };

    class const_iterator;

    class iterator {
    public:
        using iterator_category = das::forward_iterator_tag;
        using value_type        = das::pair<K, V>;
        using difference_type   = ptrdiff_t;
        using pointer           = daslang_insert_only_hash_map::reference *;
        using reference         = daslang_insert_only_hash_map::reference;
    private:
        daslang_insert_only_hash_map * owner_ = nullptr;
        size_t                         index_ = 0;
        mutable typename das::aligned_storage<sizeof(reference), alignof(reference)>::type ref_storage_;
        friend class daslang_insert_only_hash_map;
        friend class const_iterator;
    public:
        iterator () noexcept = default;
        iterator ( daslang_insert_only_hash_map * o, size_t i ) noexcept : owner_(o), index_(i) {}

        reference & operator *  () const {
            return *::new (static_cast<void*>(&ref_storage_))
                reference{ owner_->keys_[index_], owner_->val_at(index_) };
        }
        reference * operator -> () const { return &**this; }

        iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] == daslang_hash_map_detail::HASH_EMPTY ) ++index_;
            return *this;
        }
        iterator   operator ++ (int) { iterator t = *this; ++(*this); return t; }

        bool operator == ( const iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const iterator & o ) const noexcept { return !(*this == o); }
    };

    class const_iterator {
    public:
        using iterator_category = das::forward_iterator_tag;
        using value_type        = das::pair<K, V>;
        using difference_type   = ptrdiff_t;
        using pointer           = const daslang_insert_only_hash_map::const_reference *;
        using reference         = daslang_insert_only_hash_map::const_reference;
    private:
        const daslang_insert_only_hash_map * owner_ = nullptr;
        size_t                               index_ = 0;
        mutable typename das::aligned_storage<sizeof(const_reference), alignof(const_reference)>::type ref_storage_;
        friend class daslang_insert_only_hash_map;
    public:
        const_iterator () noexcept = default;
        const_iterator ( const daslang_insert_only_hash_map * o, size_t i ) noexcept : owner_(o), index_(i) {}
        const_iterator ( const iterator & it ) noexcept : owner_(it.owner_), index_(it.index_) {}

        reference & operator *  () const {
            return *::new (static_cast<void*>(&ref_storage_))
                const_reference{ owner_->keys_[index_], owner_->val_at(index_) };
        }
        reference * operator -> () const { return &**this; }

        const_iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] == daslang_hash_map_detail::HASH_EMPTY ) ++index_;
            return *this;
        }
        const_iterator   operator ++ (int) { const_iterator t = *this; ++(*this); return t; }

        bool operator == ( const const_iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const const_iterator & o ) const noexcept { return !(*this == o); }
    };

    daslang_insert_only_hash_map () = default;
    daslang_insert_only_hash_map ( const daslang_insert_only_hash_map & ) = default;
    daslang_insert_only_hash_map ( daslang_insert_only_hash_map && ) noexcept = default;
    daslang_insert_only_hash_map & operator = ( const daslang_insert_only_hash_map & ) = default;
    daslang_insert_only_hash_map & operator = ( daslang_insert_only_hash_map && ) noexcept = default;
    ~ daslang_insert_only_hash_map () = default;

    daslang_insert_only_hash_map ( das::initializer_list<das::pair<K, V>> il ) {
        reserve(il.size());
        for ( const auto & kv : il ) insert(kv);
    }

    bool         empty        () const noexcept { return size_ == 0; }
    size_type    size         () const noexcept { return size_; }
    size_type    bucket_count () const noexcept { return hashes_.size(); }

    void reserve ( size_type n ) {
        if ( n == 0 ) return;
        size_t needed = daslang_hash_map_detail::minCapacity;
        while ( needed < n * 2 ) needed *= 2;
        if ( needed > hashes_.size() ) rehash_into(needed);
    }

    void clear () noexcept {
        hashes_.clear();
        keys_.clear();
        values_.clear();
        size_ = 0;
    }

    void shrink_to_fit () {
        hashes_.shrink_to_fit();
        keys_.shrink_to_fit();
        values_.shrink_to_fit();
    }

    iterator       begin ()        { return iterator      {this, first_live_index()}; }
    const_iterator begin () const  { return const_iterator{this, first_live_index()}; }
    const_iterator cbegin () const { return begin(); }
    iterator       end   ()        { return iterator      {this, hashes_.size()}; }
    const_iterator end   () const  { return const_iterator{this, hashes_.size()}; }
    const_iterator cend   () const { return end(); }

    iterator find ( const K & key ) {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : iterator{this, idx};
    }
    const_iterator find ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : const_iterator{this, idx};
    }
    size_type count    ( const K & key ) const { return find_index(key, hash_(key)) == SIZE_MAX ? 0 : 1; }
    bool      contains ( const K & key ) const { return find_index(key, hash_(key)) != SIZE_MAX; }

    V & at ( const K & key ) {
        const size_t idx = find_index(key, hash_(key));
        if ( idx == SIZE_MAX ) das::das_throw("daslang_insert_only_hash_map::at");
        return val_at(idx);
    }
    const V & at ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        if ( idx == SIZE_MAX ) das::das_throw("daslang_insert_only_hash_map::at");
        return val_at(idx);
    }

    V & operator [] ( const K & key ) {
        return val_at(reserve_slot(key, hash_(key)).first);
    }
    V & operator [] ( K && key ) {
        auto h = hash_(key);
        return val_at(reserve_slot(das::move(key), h).first);
    }

    das::pair<iterator, bool> insert ( const das::pair<K, V> & kv ) {
        auto r = reserve_slot(kv.first, hash_(kv.first));
        if ( r.second ) val_at(r.first) = kv.second;
        return { iterator{this, r.first}, r.second };
    }
    das::pair<iterator, bool> insert ( das::pair<K, V> && kv ) {
        auto h = hash_(kv.first);
        auto r = reserve_slot(das::move(kv.first), h);
        if ( r.second ) val_at(r.first) = das::move(kv.second);
        return { iterator{this, r.first}, r.second };
    }

    template <class... Args>
    das::pair<iterator, bool> emplace ( Args &&... args ) {
        das::pair<K, V> kv(das::forward<Args>(args)...);
        return insert(das::move(kv));
    }

    template <class... Args>
    das::pair<iterator, bool> try_emplace ( const K & key, Args &&... args ) {
        auto r = reserve_slot(key, hash_(key));
        if ( r.second ) val_at(r.first) = V(das::forward<Args>(args)...);
        return { iterator{this, r.first}, r.second };
    }
    template <class... Args>
    das::pair<iterator, bool> try_emplace ( K && key, Args &&... args ) {
        auto h = hash_(key);
        auto r = reserve_slot(das::move(key), h);
        if ( r.second ) val_at(r.first) = V(das::forward<Args>(args)...);
        return { iterator{this, r.first}, r.second };
    }

    // NOTE: no erase(). By design.

private:

    size_t first_live_index () const noexcept {
        const size_t cap = hashes_.size();
        const uint64_t * pH = hashes_.data();
        for ( size_t i = 0; i < cap; ++i ) {
            if ( pH[i] != daslang_hash_map_detail::HASH_EMPTY ) return i;
        }
        return cap;
    }

    __forceinline size_t find_index ( const K & key, uint64_t raw_hash ) const {
        if ( hashes_.size() == 0 ) return SIZE_MAX;
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                return SIZE_MAX;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return size_t(index);
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    // Simpler than the tombstone-aware version: no insertI tracking, no
    // HASH_KILLED branch in the inner loop. One fewer compare per probe step.
    template <class KFwd>
    __forceinline das::pair<size_t, bool> reserve_slot ( KFwd && key, uint64_t raw_hash ) {
        if ( size_ >= hashes_.size() / 2 ) grow();
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                pHashes[index] = hashKey;
                pKeys  [index] = das::forward<KFwd>(key);
                ++size_;
                return { size_t(index), true };
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return { size_t(index), false };
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    void grow () {
        const size_t new_cap = hashes_.empty()
            ? daslang_hash_map_detail::minCapacity
            : hashes_.size() * 2;
        rehash_into(new_cap);
    }

    void rehash_into ( size_t new_cap ) {
        das::vector<uint64_t> new_hashes(new_cap, daslang_hash_map_detail::HASH_EMPTY);
        das::vector<K>        new_keys(new_cap);
        das::vector<value_storage_t> new_values(new_cap);
        const uint64_t mask = new_cap - 1;
        for ( size_t i = 0, n = hashes_.size(); i < n; ++i ) {
            const uint64_t kh = hashes_[i];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) continue;
            uint64_t j = kh & mask;
            while ( new_hashes[j] != daslang_hash_map_detail::HASH_EMPTY ) {
                j = daslang_hash_map_detail::next_probe_slot(j, mask);
            }
            new_hashes[j] = kh;
            new_keys  [j] = das::move(keys_  [i]);
            new_values[j] = das::move(values_[i]);
        }
        hashes_ = das::move(new_hashes);
        keys_   = das::move(new_keys);
        values_ = das::move(new_values);
    }
};

//
//  daslang_hash_set
//

template <class K, class Hash = daslang_hash<K>, class KeyEqual = das::equal_to<K>>
class daslang_hash_set {
    das::vector<uint64_t> hashes_;
    das::vector<K>        keys_;
    size_t           size_       = 0;
    size_t           tombstones_ = 0;
    Hash             hash_;
    KeyEqual         key_equal_;

public:
    using key_type  = K;
    using size_type = size_t;
    using hasher    = Hash;
    using key_equal = KeyEqual;

    class const_iterator {
        const daslang_hash_set * owner_ = nullptr;
        size_t                   index_ = 0;
        friend class daslang_hash_set;
    public:
        using iterator_category = das::forward_iterator_tag;
        using value_type        = K;
        using difference_type   = ptrdiff_t;
        using pointer           = const K *;
        using reference         = const K &;

        const_iterator () noexcept = default;
        const_iterator ( const daslang_hash_set * o, size_t i ) noexcept : owner_(o), index_(i) {}

        reference operator *  () const { return owner_->keys_[index_]; }
        pointer   operator -> () const { return &owner_->keys_[index_]; }

        const_iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] <= daslang_hash_map_detail::HASH_KILLED ) ++index_;
            return *this;
        }
        const_iterator   operator ++ (int) { const_iterator t = *this; ++(*this); return t; }

        bool operator == ( const const_iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const const_iterator & o ) const noexcept { return !(*this == o); }
    };

    // set iteration never exposes a mutable key — iterator is const_iterator
    using iterator = const_iterator;

    daslang_hash_set () = default;
    daslang_hash_set ( const daslang_hash_set & ) = default;
    daslang_hash_set ( daslang_hash_set && ) noexcept = default;
    daslang_hash_set & operator = ( const daslang_hash_set & ) = default;
    daslang_hash_set & operator = ( daslang_hash_set && ) noexcept = default;
    ~ daslang_hash_set () = default;

    daslang_hash_set ( das::initializer_list<K> il ) {
        reserve(il.size());
        for ( const auto & k : il ) insert(k);
    }

    bool      empty        () const noexcept { return size_ == 0; }
    size_type size         () const noexcept { return size_; }
    size_type bucket_count () const noexcept { return hashes_.size(); }

    void reserve ( size_type n ) {
        if ( n == 0 ) return;
        size_t needed = daslang_hash_map_detail::minCapacity;
        while ( needed < n * 2 ) needed *= 2;
        if ( needed > hashes_.size() ) rehash_into(needed);
    }

    void clear () noexcept {
        hashes_.clear();
        keys_.clear();
        size_ = 0;
        tombstones_ = 0;
    }

    void shrink_to_fit () {
        hashes_.shrink_to_fit();
        keys_.shrink_to_fit();
    }

    iterator       begin () const  { return const_iterator{this, first_live_index()}; }
    const_iterator cbegin () const { return begin(); }
    iterator       end   () const  { return const_iterator{this, hashes_.size()}; }
    const_iterator cend   () const { return end(); }

    const_iterator find ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : const_iterator{this, idx};
    }
    size_type count    ( const K & key ) const { return find_index(key, hash_(key)) == SIZE_MAX ? 0 : 1; }
    bool      contains ( const K & key ) const { return find_index(key, hash_(key)) != SIZE_MAX; }

    das::pair<iterator, bool> insert ( const K & key ) {
        auto r = reserve_slot(key, hash_(key));
        return { const_iterator{this, r.first}, r.second };
    }
    das::pair<iterator, bool> insert ( K && key ) {
        auto h = hash_(key);
        auto r = reserve_slot(das::move(key), h);
        return { const_iterator{this, r.first}, r.second };
    }
    template <class... Args>
    das::pair<iterator, bool> emplace ( Args &&... args ) {
        K key(das::forward<Args>(args)...);
        return insert(das::move(key));
    }

    size_type erase ( const K & key ) { return erase_index(key, hash_(key)) ? 1 : 0; }
    iterator  erase ( const_iterator it ) {
        const size_t idx = it.index_;
        hashes_[idx] = daslang_hash_map_detail::HASH_KILLED;
        keys_  [idx] = K{};
        --size_;
        ++tombstones_;
        const_iterator nxt{this, idx};
        ++nxt;
        return nxt;
    }

private:
    size_t first_live_index () const noexcept {
        const size_t cap = hashes_.size();
        const uint64_t * pH = hashes_.data();
        for ( size_t i = 0; i < cap; ++i ) {
            if ( pH[i] > daslang_hash_map_detail::HASH_KILLED ) return i;
        }
        return cap;
    }

    __forceinline size_t find_index ( const K & key, uint64_t raw_hash ) const {
        if ( hashes_.size() == 0 ) return SIZE_MAX;
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                return SIZE_MAX;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return size_t(index);
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    template <class KFwd>
    __forceinline das::pair<size_t, bool> reserve_slot ( KFwd && key, uint64_t raw_hash ) {
        if      ( size_ >= hashes_.size() / 2 )                        grow();
        else if ( (hashes_.size() - size_) / 2 < tombstones_ )         rehash_same_capacity();
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        uint64_t insertI = uint64_t(-1);
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                if ( insertI != uint64_t(-1) ) {
                    index = insertI;
                    --tombstones_;
                }
                pHashes[index] = hashKey;
                pKeys  [index] = das::forward<KFwd>(key);
                ++size_;
                return { size_t(index), true };
            } else if ( kh == daslang_hash_map_detail::HASH_KILLED ) {
                if ( insertI == uint64_t(-1) ) insertI = index;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return { size_t(index), false };
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    __forceinline bool erase_index ( const K & key, uint64_t raw_hash ) {
        const size_t idx = find_index(key, raw_hash);
        if ( idx == SIZE_MAX ) return false;
        hashes_[idx] = daslang_hash_map_detail::HASH_KILLED;
        keys_  [idx] = K{};
        --size_;
        ++tombstones_;
        return true;
    }

    void grow () {
        const size_t new_cap = hashes_.empty()
            ? daslang_hash_map_detail::minCapacity
            : hashes_.size() * 2;
        rehash_into(new_cap);
    }
    void rehash_same_capacity () { rehash_into(hashes_.size()); }

    void rehash_into ( size_t new_cap ) {
        das::vector<uint64_t> new_hashes(new_cap, daslang_hash_map_detail::HASH_EMPTY);
        das::vector<K>        new_keys(new_cap);
        const uint64_t mask = new_cap - 1;
        for ( size_t i = 0, n = hashes_.size(); i < n; ++i ) {
            const uint64_t kh = hashes_[i];
            if ( kh <= daslang_hash_map_detail::HASH_KILLED ) continue;
            uint64_t j = kh & mask;
            while ( new_hashes[j] != daslang_hash_map_detail::HASH_EMPTY ) {
                j = daslang_hash_map_detail::next_probe_slot(j, mask);
            }
            new_hashes[j] = kh;
            new_keys  [j] = das::move(keys_[i]);
        }
        hashes_ = das::move(new_hashes);
        keys_   = das::move(new_keys);
        tombstones_ = 0;
    }
};

//
//  daslang_insert_only_hash_set — set sibling of daslang_insert_only_hash_map.
//  Same rationale: drops erase + tombstone bookkeeping, simpler probe loop.
//

template <class K, class Hash = daslang_hash<K>, class KeyEqual = das::equal_to<K>>
class daslang_insert_only_hash_set {
    das::vector<uint64_t> hashes_;
    das::vector<K>        keys_;
    size_t           size_ = 0;
    Hash             hash_;
    KeyEqual         key_equal_;

public:
    using key_type  = K;
    using size_type = size_t;
    using hasher    = Hash;
    using key_equal = KeyEqual;

    class const_iterator {
        const daslang_insert_only_hash_set * owner_ = nullptr;
        size_t                               index_ = 0;
        friend class daslang_insert_only_hash_set;
    public:
        using iterator_category = das::forward_iterator_tag;
        using value_type        = K;
        using difference_type   = ptrdiff_t;
        using pointer           = const K *;
        using reference         = const K &;

        const_iterator () noexcept = default;
        const_iterator ( const daslang_insert_only_hash_set * o, size_t i ) noexcept : owner_(o), index_(i) {}

        reference operator *  () const { return owner_->keys_[index_]; }
        pointer   operator -> () const { return &owner_->keys_[index_]; }

        const_iterator & operator ++ () {
            const size_t cap = owner_->hashes_.size();
            const uint64_t * pH = owner_->hashes_.data();
            ++index_;
            while ( index_ < cap && pH[index_] == daslang_hash_map_detail::HASH_EMPTY ) ++index_;
            return *this;
        }
        const_iterator   operator ++ (int) { const_iterator t = *this; ++(*this); return t; }

        bool operator == ( const const_iterator & o ) const noexcept { return index_ == o.index_ && owner_ == o.owner_; }
        bool operator != ( const const_iterator & o ) const noexcept { return !(*this == o); }
    };

    using iterator = const_iterator;

    daslang_insert_only_hash_set () = default;
    daslang_insert_only_hash_set ( const daslang_insert_only_hash_set & ) = default;
    daslang_insert_only_hash_set ( daslang_insert_only_hash_set && ) noexcept = default;
    daslang_insert_only_hash_set & operator = ( const daslang_insert_only_hash_set & ) = default;
    daslang_insert_only_hash_set & operator = ( daslang_insert_only_hash_set && ) noexcept = default;
    ~ daslang_insert_only_hash_set () = default;

    daslang_insert_only_hash_set ( das::initializer_list<K> il ) {
        reserve(il.size());
        for ( const auto & k : il ) insert(k);
    }

    bool      empty        () const noexcept { return size_ == 0; }
    size_type size         () const noexcept { return size_; }
    size_type bucket_count () const noexcept { return hashes_.size(); }

    void reserve ( size_type n ) {
        if ( n == 0 ) return;
        size_t needed = daslang_hash_map_detail::minCapacity;
        while ( needed < n * 2 ) needed *= 2;
        if ( needed > hashes_.size() ) rehash_into(needed);
    }

    void clear () noexcept {
        hashes_.clear();
        keys_.clear();
        size_ = 0;
    }

    void shrink_to_fit () {
        hashes_.shrink_to_fit();
        keys_.shrink_to_fit();
    }

    iterator       begin () const  { return const_iterator{this, first_live_index()}; }
    const_iterator cbegin () const { return begin(); }
    iterator       end   () const  { return const_iterator{this, hashes_.size()}; }
    const_iterator cend   () const { return end(); }

    const_iterator find ( const K & key ) const {
        const size_t idx = find_index(key, hash_(key));
        return idx == SIZE_MAX ? end() : const_iterator{this, idx};
    }
    size_type count    ( const K & key ) const { return find_index(key, hash_(key)) == SIZE_MAX ? 0 : 1; }
    bool      contains ( const K & key ) const { return find_index(key, hash_(key)) != SIZE_MAX; }

    das::pair<iterator, bool> insert ( const K & key ) {
        auto r = reserve_slot(key, hash_(key));
        return { const_iterator{this, r.first}, r.second };
    }
    das::pair<iterator, bool> insert ( K && key ) {
        auto h = hash_(key);
        auto r = reserve_slot(das::move(key), h);
        return { const_iterator{this, r.first}, r.second };
    }
    template <class... Args>
    das::pair<iterator, bool> emplace ( Args &&... args ) {
        K key(das::forward<Args>(args)...);
        return insert(das::move(key));
    }

    // NOTE: no erase(). By design.

private:
    size_t first_live_index () const noexcept {
        const size_t cap = hashes_.size();
        const uint64_t * pH = hashes_.data();
        for ( size_t i = 0; i < cap; ++i ) {
            if ( pH[i] != daslang_hash_map_detail::HASH_EMPTY ) return i;
        }
        return cap;
    }

    __forceinline size_t find_index ( const K & key, uint64_t raw_hash ) const {
        if ( hashes_.size() == 0 ) return SIZE_MAX;
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                return SIZE_MAX;
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return size_t(index);
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    template <class KFwd>
    __forceinline das::pair<size_t, bool> reserve_slot ( KFwd && key, uint64_t raw_hash ) {
        if ( size_ >= hashes_.size() / 2 ) grow();
        uint64_t mask = hashes_.size() - 1;
        uint64_t index = raw_hash & mask;
        auto pKeys   = keys_.data();
        auto pHashes = hashes_.data();
        auto hashKey = daslang_hash_map_detail::to_hash_key(raw_hash);
        while ( true ) {
            auto kh = pHashes[index];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) {
                pHashes[index] = hashKey;
                pKeys  [index] = das::forward<KFwd>(key);
                ++size_;
                return { size_t(index), true };
            } else if ( kh == hashKey && key_equal_(pKeys[index], key) ) {
                return { size_t(index), false };
            }
            index = daslang_hash_map_detail::next_probe_slot(index, mask);
        }
    }

    void grow () {
        const size_t new_cap = hashes_.empty()
            ? daslang_hash_map_detail::minCapacity
            : hashes_.size() * 2;
        rehash_into(new_cap);
    }

    void rehash_into ( size_t new_cap ) {
        das::vector<uint64_t> new_hashes(new_cap, daslang_hash_map_detail::HASH_EMPTY);
        das::vector<K>        new_keys(new_cap);
        const uint64_t mask = new_cap - 1;
        for ( size_t i = 0, n = hashes_.size(); i < n; ++i ) {
            const uint64_t kh = hashes_[i];
            if ( kh == daslang_hash_map_detail::HASH_EMPTY ) continue;
            uint64_t j = kh & mask;
            while ( new_hashes[j] != daslang_hash_map_detail::HASH_EMPTY ) {
                j = daslang_hash_map_detail::next_probe_slot(j, mask);
            }
            new_hashes[j] = kh;
            new_keys  [j] = das::move(keys_[i]);
        }
        hashes_ = das::move(new_hashes);
        keys_   = das::move(new_keys);
    }
};

} // namespace das
