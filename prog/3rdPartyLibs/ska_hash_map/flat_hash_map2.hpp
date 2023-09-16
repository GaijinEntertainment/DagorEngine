//          Copyright Malte Skarupke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once
//this is direct copy of flat_hash_map.hpp, but updated to use eastl and faster log2 

#include <EASTL/functional.h>
#include <EASTL/algorithm.h>
#include <EASTL/memory.h>
#include <EASTL/iterator.h>
#include <EASTL/utility.h>
#include <EASTL/type_traits.h>
#include <EASTL/initializer_list.h>
#include <EASTL/memory.h>
#if defined(_MSC_VER) && (EA_PROCESSOR_X86_64 || EA_PROCESSOR_X86)
#include <intrin.h>
#endif

#ifdef _MSC_VER
#define SKA_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#pragma warning(push) // Save warning settings.
#pragma warning(disable : 4582)
#pragma warning(disable : 4293)
#pragma warning(disable : 4309)
#pragma warning(disable : 4583) // destructor is not implicitly called
#else
#define SKA_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

namespace ska
{
struct power_of_two_hash_policy;
struct fibonacci_hash_policy_64;
struct fibonacci_hash_policy_32;
typedef uint32_t hash_size_t;
typedef typename eastl::conditional<sizeof(size_t)==8, fibonacci_hash_policy_64, fibonacci_hash_policy_32>::type fibonacci_hash_policy;

namespace detailv3
{
template<typename Result, typename Functor>
struct functor_storage : Functor
{
    functor_storage() = default;
    functor_storage(const Functor & functor)
        : Functor(functor)
    {
    }
    template<typename... Args>
    Result operator()(Args &&... args)
    {
        return static_cast<Functor &>(*this)(eastl::forward<Args>(args)...);
    }
    template<typename... Args>
    Result operator()(Args &&... args) const
    {
        return static_cast<const Functor &>(*this)(eastl::forward<Args>(args)...);
    }
};
template<typename Result, typename... Args>
struct functor_storage<Result, Result (*)(Args...)>
{
    typedef Result (*function_ptr)(Args...);
    function_ptr function;
    functor_storage(function_ptr function)
        : function(function)
    {
    }
    Result operator()(Args... args) const
    {
        return function(eastl::forward<Args>(args)...);
    }
    operator function_ptr &()
    {
        return function;
    }
    operator const function_ptr &()
    {
        return function;
    }
};
template<typename key_type, typename value_type, typename hasher>
struct KeyOrValueHasher : functor_storage<size_t, hasher>
{
    typedef functor_storage<size_t, hasher> hasher_storage;
    KeyOrValueHasher() = default;
    KeyOrValueHasher(const hasher & hash)
        : hasher_storage(hash)
    {
    }
    size_t operator()(const key_type & key)
    {
        return static_cast<hasher_storage &>(*this)(key);
    }
    size_t operator()(const key_type & key) const
    {
        return static_cast<const hasher_storage &>(*this)(key);
    }
    size_t operator()(const value_type & value)
    {
        return static_cast<hasher_storage &>(*this)(value.first);
    }
    size_t operator()(const value_type & value) const
    {
        return static_cast<const hasher_storage &>(*this)(value.first);
    }
    template<typename F, typename S>
    size_t operator()(const eastl::pair<F, S> & value)
    {
        return static_cast<hasher_storage &>(*this)(value.first);
    }
    template<typename F, typename S>
    size_t operator()(const eastl::pair<F, S> & value) const
    {
        return static_cast<const hasher_storage &>(*this)(value.first);
    }
};
template<typename key_type, typename value_type, typename key_equal>
struct KeyOrValueEquality : functor_storage<bool, key_equal>
{
    typedef functor_storage<bool, key_equal> equality_storage;
    KeyOrValueEquality() = default;
    KeyOrValueEquality(const key_equal & equality)
        : equality_storage(equality)
    {
    }
    bool operator()(const key_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs);
    }
    bool operator()(const key_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    bool operator()(const value_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    bool operator()(const value_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const key_type & lhs, const eastl::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const eastl::pair<F, S> & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    template<typename F, typename S>
    bool operator()(const value_type & lhs, const eastl::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const eastl::pair<F, S> & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename FL, typename SL, typename FR, typename SR>
    bool operator()(const eastl::pair<FL, SL> & lhs, const eastl::pair<FR, SR> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
};

static constexpr int8_t min_lookups = 4;
static constexpr int8_t special_end_value = 0;

template<size_t N, size_t A>
struct sherwood_v3_entry_result_holder
{
  constexpr sherwood_v3_entry_result_holder()=default;
  constexpr sherwood_v3_entry_result_holder(int8_t dfd) : distance_from_desired(dfd) {}
  int8_t distance_from_desired = -1;
  alignas(A) char value[N] = {};
  static const sherwood_v3_entry_result_holder * get_result()
  {
    static constexpr const sherwood_v3_entry_result_holder result[] = { {}, {}, {}, {special_end_value} };
    return result;
  }
};

template<typename T>
struct sherwood_v3_entry
{
    sherwood_v3_entry()
    {
    }
    sherwood_v3_entry(int8_t distance_from_desired)
        : distance_from_desired(distance_from_desired)
    {
    }
    ~sherwood_v3_entry()
    {
    }

    static sherwood_v3_entry * empty_default_table()
    {
      static_assert(sizeof(sherwood_v3_entry) == sizeof(sherwood_v3_entry_result_holder<sizeof(T), alignof(T)>), "Bad entry size");
      return (sherwood_v3_entry*)sherwood_v3_entry_result_holder<sizeof(T), alignof(T)>::get_result();
    }

    bool has_value() const
    {
        return distance_from_desired >= 0;
    }
    bool is_empty() const
    {
        return distance_from_desired < 0;
    }
    bool is_at_desired_position() const
    {
        return distance_from_desired <= 0;
    }
    template<typename... Args>
    void emplace(int8_t distance, Args &&... args)
    {
        new (eastl::addressof(value)) T(eastl::forward<Args>(args)...);
        distance_from_desired = distance;
    }

    void destroy_value()
    {
        value.~T();
        distance_from_desired = -1;
    }

    int8_t distance_from_desired = -1;
    union { T value; };
};

template <typename T>
inline typename eastl::enable_if<sizeof(T)==sizeof(uint32_t), int8_t>::type
log2(T value)
{
#ifdef _MSC_VER
  unsigned long res;
  _BitScanReverse(&res, value);
  return (int8_t)res;
#elif defined(__GNUC__) || defined(__clang__)
    return 31UL - __builtin_clz(value);
#else
#error "log2(uint32_t) not implemented"
#endif
}

#if EA_PLATFORM_WORD_SIZE >= 8
template <typename T>
inline typename eastl::enable_if<sizeof(T)==sizeof(uint64_t), int8_t>::type
log2(T value)
{
#ifdef _MSC_VER
  unsigned long res;
  _BitScanReverse64(&res, value);
  return (int8_t)res;
#elif defined(__GNUC__) || defined(__clang__)
  return 63ULL - __builtin_clzll(value);
#else
#error "log2(uint64_t) not implemented"
#endif
}
#endif

template<typename T, bool>
struct AssignIfTrue
{
    void operator()(T & lhs, const T & rhs)
    {
        lhs = rhs;
    }
    void operator()(T & lhs, T && rhs)
    {
        lhs = eastl::move(rhs);
    }
};
template<typename T>
struct AssignIfTrue<T, false>
{
    void operator()(T &, const T &)
    {
    }
    void operator()(T &, T &&)
    {
    }
};

inline hash_size_t next_power_of_two(hash_size_t i)
{
    return hash_size_t(1) << (detailv3::log2(i - 1) + 1);
}

template <typename T, typename H, typename = void>
struct HashPolicySelector
{
    template <typename U>
    static power_of_two_hash_policy test(decltype(eastl::declval<U&>().c_str())*); // strings are using power_of_two_hash_policy by default

    template <typename U>
    static fibonacci_hash_policy test(...);

    typedef decltype(test<T>(0)) type;
};

template <typename T, typename H>
struct HashPolicySelector<T, H, eastl::void_t<typename H::hash_policy>>
{
    typedef typename H::hash_policy type;
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc>
class
#ifdef _MSC_VER
__declspec(empty_bases)
#endif
sherwood_v3_table : public ArgumentAlloc, private Hasher, private Equal
{
protected:
    using Entry = detailv3::sherwood_v3_entry<T>;
    using EntryPointer = Entry*;
    struct convertible_to_iterator;
public:

    using value_type = T;
    using hash_size_type = hash_size_t;
    using difference_type = ptrdiff_t;
    using hasher = ArgumentHash;
    using key_equal = ArgumentEqual;
    using allocator_type = ArgumentAlloc;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    sherwood_v3_table()
    {
    }
    explicit sherwood_v3_table(hash_size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : ArgumentAlloc(alloc), Hasher(hash), Equal(equal)
    {
        rehash(bucket_count);
    }
    sherwood_v3_table(hash_size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v3_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v3_table(hash_size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v3_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v3_table(const ArgumentAlloc & alloc)
        : ArgumentAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v3_table(It first, It last, hash_size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v3_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v3_table(It first, It last, hash_size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v3_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v3_table(It first, It last, hash_size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v3_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v3_table(std::initializer_list<T> il, hash_size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v3_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            rehash(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v3_table(std::initializer_list<T> il, hash_size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v3_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v3_table(std::initializer_list<T> il, hash_size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v3_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v3_table(const sherwood_v3_table & other)
        : sherwood_v3_table(other, other.get_allocator())
    {
    }
    sherwood_v3_table(const sherwood_v3_table & other, const ArgumentAlloc & alloc)
        : ArgumentAlloc(alloc), Hasher(other), Equal(other), _max_load_factor(other._max_load_factor)
    {
        rehash_for_other_container(other);
        insert(other.begin(), other.end());
    }
    sherwood_v3_table(sherwood_v3_table && other) EA_NOEXCEPT
        : ArgumentAlloc(eastl::move(other)), Hasher(eastl::move(other)), Equal(eastl::move(other))
    {
        swap_pointers(other);
    }
    sherwood_v3_table(sherwood_v3_table && other, const ArgumentAlloc & alloc) EA_NOEXCEPT
        : ArgumentAlloc(alloc), Hasher(eastl::move(other)), Equal(eastl::move(other))
    {
        swap_pointers(other);
    }
    sherwood_v3_table & operator=(const sherwood_v3_table & other)
    {
        if (this == eastl::addressof(other))
            return *this;

        clear();
        _max_load_factor = other._max_load_factor;
        static_cast<Hasher &>(*this) = other;
        static_cast<Equal &>(*this) = other;
        rehash_for_other_container(other);
        insert(other.begin(), other.end());
        return *this;
    }
    sherwood_v3_table & operator=(sherwood_v3_table && other) EA_NOEXCEPT
    {
        if (this == eastl::addressof(other))
            return *this;
        else if (static_cast<ArgumentAlloc&>(*this) == static_cast<ArgumentAlloc&>(other))
        {
            swap_pointers(other);
        }
        else
        {
            clear();
            _max_load_factor = other._max_load_factor;
            rehash_for_other_container(other);
            for (T & elem : other)
                emplace(eastl::move(elem));
            other.clear();
        }
        static_cast<Hasher &>(*this) = eastl::move(other);
        static_cast<Equal &>(*this) = eastl::move(other);
        return *this;
    }
    ~sherwood_v3_table()
    {
        clear();
        deallocate_data(entries, num_slots_minus_one, max_lookups);
    }

    const allocator_type & get_allocator() const
    {
        return static_cast<const allocator_type &>(*this);
    }
    const ArgumentEqual & key_eq() const
    {
        return static_cast<const ArgumentEqual &>(*this);
    }
    const ArgumentHash & hash_function() const
    {
        return static_cast<const ArgumentHash &>(*this);
    }

    template<typename ValueType>
    struct templated_iterator
    {
        templated_iterator() = default;
        templated_iterator(EntryPointer current)
            : current(current)
        {
        }
        EntryPointer current = EntryPointer();

        using iterator_category = eastl::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType *;
        using reference = ValueType &;

        friend bool operator==(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return lhs.current == rhs.current;
        }
        friend bool operator!=(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return !(lhs == rhs);
        }

        templated_iterator & operator++()
        {
            do
            {
                ++current;
            }
            while(current->is_empty());
            return *this;
        }
        templated_iterator operator++(int)
        {
            templated_iterator copy(*this);
            ++*this;
            return copy;
        }

        ValueType & operator*() const
        {
            return current->value;
        }
        ValueType * operator->() const
        {
            return eastl::addressof(current->value);
        }

        operator templated_iterator<const value_type>() const
        {
            return { current };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        for (EntryPointer it = entries;; ++it)
        {
            if (it->has_value())
                return { it };
        }
    }
    const_iterator begin() const
    {
        for (EntryPointer it = entries;; ++it)
        {
            if (it->has_value())
                return { it };
        }
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { entries + static_cast<ptrdiff_t>(num_slots_minus_one + max_lookups) };
    }
    const_iterator end() const
    {
        return { entries + static_cast<ptrdiff_t>(num_slots_minus_one + max_lookups) };
    }
    const_iterator cend() const
    {
        return end();
    }

    iterator find(const FindKey & key)
    {
        hash_size_t index = hash_policy.index_for_hash(hash_object(key), num_slots_minus_one);
        EntryPointer it = entries + ptrdiff_t(index);
        for (int8_t distance = 0; it->distance_from_desired >= distance; ++distance, ++it)
        {
            if (compares_equal(key, it->value))
                return { it };
        }
        return end();
    }
    const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v3_table *>(this)->find(key);
    }
    hash_size_t count(const FindKey & key) const
    {
        return find(key) == end() ? 0 : 1;
    }
    eastl::pair<iterator, iterator> equal_range(const FindKey & key)
    {
        iterator found = find(key);
        if (found == end())
            return { found, found };
        else
            return { found, eastl::next(found) };
    }
    eastl::pair<const_iterator, const_iterator> equal_range(const FindKey & key) const
    {
        const_iterator found = find(key);
        if (found == end())
            return { found, found };
        else
            return { found, eastl::next(found) };
    }

    template<typename Key, typename... Args>
    eastl::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        hash_size_t index = hash_policy.index_for_hash(hash_object(key), num_slots_minus_one);
        EntryPointer current_entry = entries + ptrdiff_t(index);
        int8_t distance_from_desired = 0;
        for (; current_entry->distance_from_desired >= distance_from_desired; ++current_entry, ++distance_from_desired)
        {
            if (compares_equal(key, current_entry->value))
                return { { current_entry }, false };
        }
        return emplace_new_key(distance_from_desired, current_entry, eastl::forward<Key>(key), eastl::forward<Args>(args)...);
    }

    eastl::pair<iterator, bool> insert(const value_type & value)
    {
        return emplace(value);
    }
    eastl::pair<iterator, bool> insert(value_type && value)
    {
        return emplace(eastl::move(value));
    }
    template<typename... Args>
    iterator emplace_hint(const_iterator, Args &&... args)
    {
        return emplace(eastl::forward<Args>(args)...).first;
    }
    iterator insert(const_iterator, const value_type & value)
    {
        return emplace(value).first;
    }
    iterator insert(const_iterator, value_type && value)
    {
        return emplace(eastl::move(value)).first;
    }

    template<typename It>
    void insert(It begin, It end)
    {
        for (; begin != end; ++begin)
        {
            emplace(*begin);
        }
    }
    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }

    void rehash(hash_size_t num_buckets)
    {
        using eastl::swap;
        num_buckets = eastl::max(num_buckets, static_cast<hash_size_t>(ceilf(float(num_elements) / _max_load_factor)));
        if (num_buckets == 0)
        {
            reset_to_empty_state();
            return;
        }
        auto new_prime_index = hash_policy.next_size_over(num_buckets);
        if (num_buckets == bucket_count())
            return;
        int8_t new_max_lookups = compute_max_lookups(num_buckets);
        auto new_buckets = (EntryPointer)eastl::allocate_memory(static_cast<ArgumentAlloc&>(*this), sizeof(Entry) * (num_buckets + new_max_lookups), alignof(Entry), 0);
        EntryPointer special_end_item = new_buckets + static_cast<ptrdiff_t>(num_buckets + new_max_lookups - 1);
        for (EntryPointer it = new_buckets; it != special_end_item; ++it)
            it->distance_from_desired = -1;
        special_end_item->distance_from_desired = special_end_value;
        swap(entries, new_buckets);
        swap(num_slots_minus_one, num_buckets);
        --num_slots_minus_one;
        hash_policy.commit(new_prime_index);
        int8_t old_max_lookups = max_lookups;
        max_lookups = new_max_lookups;
        num_elements = 0;
        for (EntryPointer it = new_buckets, end = it + static_cast<ptrdiff_t>(num_buckets + old_max_lookups); it != end; ++it)
        {
            if (it->has_value())
            {
                emplace(eastl::move(it->value));
                it->destroy_value();
            }
        }
        deallocate_data(new_buckets, num_buckets, old_max_lookups);
    }

    void reserve(hash_size_t num_reserved_elements)
    {
        hash_size_t required_buckets = num_buckets_for_reserve(num_reserved_elements);
        if (required_buckets > bucket_count())
            rehash(required_buckets);
    }

    // the return value is a type that can be converted to an iterator
    // the reason for doing this is that it's not free to find the
    // iterator pointing at the next element. if you care about the
    // next iterator, turn the return value into an iterator
    convertible_to_iterator erase(const_iterator to_erase)
    {
        EntryPointer current = to_erase.current;
        current->destroy_value();
        --num_elements;
        for (EntryPointer next = current + ptrdiff_t(1); !next->is_at_desired_position(); ++current, ++next)
        {
            current->emplace(next->distance_from_desired - 1, eastl::move(next->value));
            next->destroy_value();
        }
        return { to_erase.current };
    }

    iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        if (begin_it == end_it)
            return { begin_it.current };
        for (EntryPointer it = begin_it.current, end = end_it.current; it != end; ++it)
        {
            if (it->has_value())
            {
                it->destroy_value();
                --num_elements;
            }
        }
        if (end_it == this->end())
            return this->end();
        ptrdiff_t num_to_move = eastl::min(static_cast<ptrdiff_t>(end_it.current->distance_from_desired), end_it.current - begin_it.current);
        EntryPointer to_return = end_it.current - num_to_move;
        for (EntryPointer it = end_it.current; !it->is_at_desired_position();)
        {
            EntryPointer target = it - num_to_move;
            target->emplace(it->distance_from_desired - num_to_move, eastl::move(it->value));
            it->destroy_value();
            ++it;
            num_to_move = eastl::min(static_cast<ptrdiff_t>(it->distance_from_desired), num_to_move);
        }
        return { to_return };
    }

    hash_size_t erase(const FindKey & key)
    {
        auto found = find(key);
        if (found == end())
            return 0;
        else
        {
            erase(found);
            return 1;
        }
    }

    void clear()
    {
        for (EntryPointer it = entries, end = it + static_cast<ptrdiff_t>(num_slots_minus_one + max_lookups); it != end; ++it)
        {
            if (it->has_value())
                it->destroy_value();
        }
        num_elements = 0;
    }

    void shrink_to_fit()
    {
        rehash_for_other_container(*this);
    }

    void swap(sherwood_v3_table & other)
    {
        using eastl::swap;
        swap_pointers(other);
        swap(static_cast<ArgumentHash &>(*this), static_cast<ArgumentHash &>(other));
        swap(static_cast<ArgumentEqual &>(*this), static_cast<ArgumentEqual &>(other));
    }

    hash_size_t size() const
    {
        return num_elements;
    }
    hash_size_t max_size() const
    {
        return eastl::numeric_limits<typename eastl::pointer_traits<EntryPointer>::difference_type>::max() / sizeof(Entry);
    }
    hash_size_t bucket_count() const
    {
        return num_slots_minus_one ? num_slots_minus_one + 1 : 0;
    }
    hash_size_type max_bucket_count() const
    {
        return (eastl::numeric_limits<typename eastl::pointer_traits<EntryPointer>::difference_type>::max() - min_lookups) / sizeof(Entry);
    }
    hash_size_t bucket(const FindKey & key) const
    {
        return hash_policy.index_for_hash(hash_object(key), num_slots_minus_one);
    }
    float load_factor() const
    {
        hash_size_t buckets = bucket_count();
        if (buckets)
            return static_cast<float>(num_elements) / bucket_count();
        else
            return 0;
    }
    void max_load_factor(float value)
    {
        _max_load_factor = value;
    }
    float max_load_factor() const
    {
        return _max_load_factor;
    }

    bool empty() const
    {
        return num_elements == 0;
    }

protected:
    EntryPointer entries = Entry::empty_default_table();
    hash_size_t num_slots_minus_one = 0;
    typename HashPolicySelector<FindKey, ArgumentHash>::type hash_policy;
    int8_t max_lookups = detailv3::min_lookups - 1;
    float _max_load_factor = 0.5f;
    hash_size_t num_elements = 0;

    static int8_t compute_max_lookups(hash_size_t num_buckets)
    {
        int8_t desired = detailv3::log2(num_buckets);
        return eastl::max(detailv3::min_lookups, desired);
    }

    hash_size_t num_buckets_for_reserve(hash_size_t num_elements_) const
    {
        return static_cast<hash_size_t>(ceilf(float(num_elements_) / eastl::min(0.5f, _max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v3_table & other)
    {
        rehash(eastl::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }

    void swap_pointers(sherwood_v3_table & other)
    {
        using eastl::swap;
        swap(hash_policy, other.hash_policy);
        swap(entries, other.entries);
        swap(num_slots_minus_one, other.num_slots_minus_one);
        swap(num_elements, other.num_elements);
        swap(max_lookups, other.max_lookups);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename Key, typename... Args>
    SKA_NOINLINE(eastl::pair<iterator, bool>) emplace_new_key(int8_t distance_from_desired, EntryPointer current_entry, Key && key, Args &&... args)
    {
        using eastl::swap;
        if (num_slots_minus_one == 0 || distance_from_desired == max_lookups || num_elements + 1 > hash_size_t(float(num_slots_minus_one + 1) * _max_load_factor))
        {
            grow();
            return emplace(eastl::forward<Key>(key), eastl::forward<Args>(args)...);
        }
        else if (current_entry->is_empty())
        {
            current_entry->emplace(distance_from_desired, eastl::forward<Key>(key), eastl::forward<Args>(args)...);
            ++num_elements;
            return { { current_entry }, true };
        }
        value_type to_insert(eastl::forward<Key>(key), eastl::forward<Args>(args)...);
        swap(distance_from_desired, current_entry->distance_from_desired);
        swap(to_insert, current_entry->value);
        iterator result = { current_entry };
        for (++distance_from_desired, ++current_entry;; ++current_entry)
        {
            if (current_entry->is_empty())
            {
                current_entry->emplace(distance_from_desired, eastl::move(to_insert));
                ++num_elements;
                return { result, true };
            }
            else if (current_entry->distance_from_desired < distance_from_desired)
            {
                swap(distance_from_desired, current_entry->distance_from_desired);
                swap(to_insert, current_entry->value);
                ++distance_from_desired;
            }
            else
            {
                ++distance_from_desired;
                if (distance_from_desired == max_lookups)
                {
                    swap(to_insert, result.current->value);
                    grow();
                    return emplace(eastl::move(to_insert));
                }
            }
        }
    }

    void grow()
    {
        rehash(eastl::max(hash_size_t(4), 2 * bucket_count()));
    }

    void deallocate_data(EntryPointer begin, hash_size_t num_slots_minus_one_, int8_t max_lookups_)
    {
        if (begin != Entry::empty_default_table())
        {
            EASTLFree(static_cast<ArgumentAlloc&>(*this), begin, sizeof(Entry) * (num_slots_minus_one_ + max_lookups_ + 1));
        }
    }

    void reset_to_empty_state()
    {
        deallocate_data(entries, num_slots_minus_one, max_lookups);
        entries = Entry::empty_default_table();
        num_slots_minus_one = 0;
        hash_policy.reset();
        max_lookups = detailv3::min_lookups - 1;
    }

    template<typename U>
    size_t hash_object(const U & key)
    {
        return static_cast<Hasher &>(*this)(key);
    }
    template<typename U>
    size_t hash_object(const U & key) const
    {
        return static_cast<const Hasher &>(*this)(key);
    }
    template<typename L, typename R>
    bool compares_equal(const L & lhs, const R & rhs)
    {
        return static_cast<Equal &>(*this)(lhs, rhs);
    }

    struct convertible_to_iterator
    {
        EntryPointer it;

        operator iterator()
        {
            if (it->has_value())
                return { it };
            else
                return ++iterator{it};
        }
        operator const_iterator()
        {
            if (it->has_value())
                return { it };
            else
                return ++const_iterator{it};
        }
    };

};
}

struct power_of_two_hash_policy
{
    hash_size_t index_for_hash(size_t hash, hash_size_t num_slots_minus_one) const
    {
        return hash & num_slots_minus_one;
    }
    hash_size_t keep_in_range(hash_size_t index, hash_size_t num_slots_minus_one) const
    {
        return index_for_hash(index, num_slots_minus_one);
    }
    int8_t next_size_over(hash_size_t & size) const
    {
	    hash_size_t csize = size - 1;
	    hash_size_t next_size = hash_size_t(1) << (detailv3::log2(csize) + 1);
        size = csize ? next_size : 1;
        return 0;
    }
    void commit(int8_t)
    {
    }
    void reset()
    {
    }

};

struct fibonacci_hash_policy_64
{
    hash_size_t index_for_hash(size_t hash, hash_size_t /*num_slots_minus_one*/) const
    {
        return hash_size_t((11400714819323198485ull * hash) >> shift);
    }
    hash_size_t keep_in_range(hash_size_t index, hash_size_t num_slots_minus_one) const
    {
        return index & num_slots_minus_one;
    }

    int8_t next_size_over(hash_size_t & size) const
    {
        size = detailv3::next_power_of_two(eastl::max(hash_size_t(2), size));
        return 64 - detailv3::log2(size);
    }
    void commit(int8_t shift_)
    {
        this->shift = shift_;
    }
    void reset()
    {
        shift = 63;
    }

private:
    int8_t shift = 63;
};

struct fibonacci_hash_policy_32
{
    hash_size_t index_for_hash(size_t hash, hash_size_t /*num_slots_minus_one*/) const
    {
        return (2654435769ul * hash) >> shift;
    }
    hash_size_t keep_in_range(hash_size_t index, hash_size_t num_slots_minus_one) const
    {
        return index & num_slots_minus_one;
    }

    int8_t next_size_over(hash_size_t & size) const
    {
        size = detailv3::next_power_of_two(eastl::max(hash_size_t(2), size));
        return 32 - detailv3::log2(size);
    }
    void commit(int8_t shift_)
    {
        this->shift = shift_;
    }
    void reset()
    {
        shift = 31;
    }

private:
    int8_t shift = 31;
};

template<typename K, typename V, typename H = eastl::hash<K>, typename E = eastl::equal_to<K>, typename A = EASTLAllocatorType>
class flat_hash_map
        : public detailv3::sherwood_v3_table
        <
            eastl::pair<K, V>,
            K,
            H,
            detailv3::KeyOrValueHasher<K, eastl::pair<K, V>, H>,
            E,
            detailv3::KeyOrValueEquality<K, eastl::pair<K, V>, E>,
            A
        >
{
    using Table = detailv3::sherwood_v3_table
    <
        eastl::pair<K, V>,
        K,
        H,
        detailv3::KeyOrValueHasher<K, eastl::pair<K, V>, H>,
        E,
        detailv3::KeyOrValueEquality<K, eastl::pair<K, V>, E>,
        A
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    flat_hash_map()
    {
    }

    inline V & operator[](const K & key)
    {
        return emplace(key, convertible_to_value()).first->second;
    }
    inline V & operator[](K && key)
    {
        return emplace(eastl::move(key), convertible_to_value()).first->second;
    }

    using Table::emplace;
    eastl::pair<typename Table::iterator, bool> emplace()
    {
        return emplace(key_type(), convertible_to_value());
    }
    template<typename M>
    eastl::pair<typename Table::iterator, bool> insert_or_assign(const key_type & key, M && m)
    {
        auto emplace_result = emplace(key, eastl::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = eastl::forward<M>(m);
        return emplace_result;
    }
    template<typename M>
    eastl::pair<typename Table::iterator, bool> insert_or_assign(key_type && key, M && m)
    {
        auto emplace_result = emplace(eastl::move(key), eastl::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = eastl::forward<M>(m);
        return emplace_result;
    }
    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, const key_type & key, M && m)
    {
        return insert_or_assign(key, eastl::forward<M>(m)).first;
    }
    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, key_type && key, M && m)
    {
        return insert_or_assign(eastl::move(key), eastl::forward<M>(m)).first;
    }

    friend bool operator==(const flat_hash_map & lhs, const flat_hash_map & rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (const typename Table::value_type & value : lhs)
        {
            auto found = rhs.find(value.first);
            if (found == rhs.end())
                return false;
            else if (value.second != found->second)
                return false;
        }
        return true;
    }
    friend bool operator!=(const flat_hash_map & lhs, const flat_hash_map & rhs)
    {
        return !(lhs == rhs);
    }

    template <typename U, typename UHash, typename BinaryPredicate>
    typename Table::iterator find_as(const U& key, UHash uhash, BinaryPredicate predicate)
    {
        hash_size_t index = Table::hash_policy.index_for_hash(uhash(key), Table::num_slots_minus_one);
        typename Table::EntryPointer it = Table::entries + ptrdiff_t(index);
        for (int8_t distance = 0; it->distance_from_desired >= distance; ++distance, ++it)
        {
            if (predicate(key, it->value.first))
                return { it };
        }
        return Table::end();
    }
    template <typename U, typename UHash, typename BinaryPredicate>
    typename Table::const_iterator find_as(const U& key, UHash uhash, BinaryPredicate predicate) const
    {
        return const_cast<flat_hash_map *>(this)->find_as(key, uhash, predicate);
    }
    template <typename U>
    typename Table::iterator find_as(const U key)
    {
      return find_as(key, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>());
    }
    template <typename U>
    typename Table::const_iterator find_as(U key) const
    {
      return const_cast<flat_hash_map *>(this)->find_as(key);
    }


private:
    struct convertible_to_value
    {
        operator V() const
        {
            return V();
        }
    };
};

template<typename T, typename H = eastl::hash<T>, typename E = eastl::equal_to<T>, typename A = EASTLAllocatorType>
class flat_hash_set
        : public detailv3::sherwood_v3_table
        <
            T,
            T,
            H,
            detailv3::functor_storage<hash_size_t, H>,
            E,
            detailv3::functor_storage<bool, E>,
            A
        >
{
    using Table = detailv3::sherwood_v3_table
    <
        T,
        T,
        H,
        detailv3::functor_storage<hash_size_t, H>,
        E,
        detailv3::functor_storage<bool, E>,
        A
    >;
public:

    using key_type = T;

    using Table::Table;
    flat_hash_set()
    {
    }

    template<typename... Args>
    eastl::pair<typename Table::iterator, bool> emplace(Args &&... args)
    {
        return Table::emplace(T(eastl::forward<Args>(args)...));
    }
    eastl::pair<typename Table::iterator, bool> emplace(const key_type & arg)
    {
        return Table::emplace(arg);
    }
    eastl::pair<typename Table::iterator, bool> emplace(key_type & arg)
    {
        return Table::emplace(arg);
    }
    eastl::pair<typename Table::iterator, bool> emplace(const key_type && arg)
    {
        return Table::emplace(eastl::move(arg));
    }
    eastl::pair<typename Table::iterator, bool> emplace(key_type && arg)
    {
        return Table::emplace(eastl::move(arg));
    }

    template <typename U, typename UHash, typename BinaryPredicate>
    typename Table::iterator find_as(const U& key, UHash uhash, BinaryPredicate predicate)
    {
        hash_size_t index = Table::hash_policy.index_for_hash(uhash(key), Table::num_slots_minus_one);
        typename Table::EntryPointer it = Table::entries + ptrdiff_t(index);
        for (int8_t distance = 0; it->distance_from_desired >= distance; ++distance, ++it)
        {
            if (predicate(key, it->value))
                return { it };
        }
        return Table::end();
    }
    template <typename U, typename UHash, typename BinaryPredicate>
    typename Table::const_iterator find_as(const U& key, UHash uhash, BinaryPredicate predicate) const
    {
        return const_cast<flat_hash_set *>(this)->find_as(key, uhash, predicate);
    }
    template <typename U>
    typename Table::iterator find_as(U key)
    {
      return find_as(key, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>());
    }
    template <typename U>
    typename Table::const_iterator find_as(U key) const
    {
      return const_cast<flat_hash_set *>(this)->find_as(key);
    }

    friend bool operator==(const flat_hash_set & lhs, const flat_hash_set & rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (const T & value : lhs)
        {
            if (rhs.find(value) == rhs.end())
                return false;
        }
        return true;
    }
    friend bool operator!=(const flat_hash_set & lhs, const flat_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};


template<typename T>
struct power_of_two_std_hash : eastl::hash<T>
{
    typedef ska::power_of_two_hash_policy hash_policy;
};

} // end namespace ska
#ifdef _MSC_VER
#pragma warning(pop) // Restore warning settings.*
#endif
