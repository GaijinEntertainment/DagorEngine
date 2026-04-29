#pragma once

#include "squtils.h"

#include <cstring>
#include <memory>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#define ALIGN_SIZE(len, align) (((len)+(align - 1)) & ~((align)-1))
#define ALIGN_SIZE_TO_WORD(len) ALIGN_SIZE(len, 0x8)

class Arena {
public:

    Arena(SQAllocContext alloc_ctx, const char *name, size_t chunkSize = 16<<10)
        : _alloc_ctx(alloc_ctx), _name(name), _chunks(NULL)
        , _chunkSize(chunkSize) {
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    ~Arena() {
        release();
    }

    uint8_t *allocate(size_t size) {
        size = ALIGN_SIZE_TO_WORD(size);

        if (_chunks && size <= _chunks->left())
            return _chunks->allocate(size);

        // Request doesn't fit in current chunk - allocate new
        size_t dataSize = (size > _chunkSize) ? size : _chunkSize;
        size_t totalSize = sizeof(Chunk) + dataSize;
        uint8_t *block = (uint8_t *)SQ_MALLOC(_alloc_ctx, totalSize);
        assert(block);
        uint8_t *data = block + sizeof(Chunk);
        _chunks = new(block) Chunk(_chunks, data, dataSize, totalSize);
        return _chunks->allocate(size);
    }

    bool tryExtend(void *ptr, size_t oldSize, size_t newSize) {
        if (!_chunks) return false;
        uint8_t *end = (uint8_t *)ptr + oldSize;
        if (end == _chunks->_ptr && newSize - oldSize <= _chunks->left()) {
            _chunks->_ptr += (newSize - oldSize);
            return true;
        }
        return false;
    }

    void release() {
        struct Chunk *ch = _chunks;
        while (ch) {
            struct Chunk *cur = ch;
            ch = cur->_next;
            SQ_FREE(_alloc_ctx, cur, cur->_totalSize);
        }
        _chunks = NULL;
    }

private:
    struct Chunk {
        struct Chunk *_next;
        uint8_t *_start;
        uint8_t *_ptr;
        size_t _size;
        size_t _totalSize; // sizeof(Chunk) + _size, for SQ_FREE

        size_t allocated() const { return _ptr - _start; }
        size_t left() const { return _size - allocated(); }

        Chunk(struct Chunk *n, uint8_t *p, size_t s, size_t total)
            : _next(n), _start(p), _ptr(p), _size(s), _totalSize(total) {
        }

        uint8_t *allocate(size_t size) {
            assert(size <= left());
            uint8_t *res = _ptr;
            _ptr += size;
            return res;
        }
    };

    struct Chunk *_chunks;
    SQAllocContext _alloc_ctx;
    const char *_name;
    size_t _chunkSize;
};


class ArenaObj {
protected:
    ArenaObj() = default;
    ~ArenaObj() = default;

private:
    void *operator new(size_t /*size*/) = delete;

public:
    void *operator new(size_t size, Arena *arena) {
        return arena->allocate(size);
    }

    void operator delete(void * /*p*/, Arena * /*arena*/) { }
    void operator delete(void * /*p*/) { }
};


template<typename T>
class ArenaVector {
public:
    using size_type = uint32_t;

    ArenaVector(Arena *arena)
        : _arena(arena)
        , _vals(NULL)
        , _size(0)
        , _allocated(0)
    {
    }

    // Implement if needed
    ArenaVector(const ArenaVector&) = delete;
    ArenaVector& operator=(const ArenaVector&) = delete;

    ArenaVector(ArenaVector&& other) noexcept
        : _arena(other._arena)
        , _vals(other._vals)
        , _size(other._size)
        , _allocated(other._allocated)
    {
        other._vals = nullptr;
        other._size = 0;
        other._allocated = 0;
    }

    inline T &push_back(const T& val)
    {
        if (_allocated <= _size)
            _realloc(_size * 2);
        return *(new ((void *)&_vals[_size++]) T(val));
    }
    inline void pop_back()
    {
        _size--; _vals[_size].~T();
    }

    T& top() const { return _vals[_size - 1]; }
    inline size_type size() const { return _size; }
    bool empty() const { return (_size == 0); }

    inline T &back() const { return _vals[_size - 1]; }
    inline T& operator[](size_type pos) const { return _vals[pos]; }

    void insert(size_type pos, T v) {
      assert(pos <= _size);
      if (_allocated <= _size) _realloc(_size * 2);
      memmove(&_vals[pos + 1], &_vals[pos], (_size - pos) * sizeof(T));
      new ((void *)&_vals[pos]) T(v);
      _size += 1;
    }

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin() { return _vals; }
    const_iterator begin() const { return _vals; }
    iterator end() { return _vals + _size; }
    const_iterator end() const { return _vals + _size; }

    void reserve(size_type newSize) {
      if (newSize <= _allocated) return;
      _realloc(newSize);
    }

private:

    void _realloc(size_type newsize)
    {
        newsize = (newsize > 0) ? newsize : 4;
        size_t newBytes = newsize * sizeof(T);
        if (_vals) {
            size_t oldBytes = ALIGN_SIZE_TO_WORD(_allocated * sizeof(T));
            if (_arena->tryExtend(_vals, oldBytes, ALIGN_SIZE_TO_WORD(newBytes))) {
                _allocated = newsize;
                return;
            }
            T *newPtr = (T *)_arena->allocate(newBytes);
            memcpy(newPtr, _vals, _size * sizeof(T));
            _vals = newPtr;
        } else {
            _vals = (T *)_arena->allocate(newBytes);
        }
        _allocated = newsize;
    }

    Arena *_arena;

    T *_vals;

    size_type _size;
    size_type _allocated;
};

template <typename T>
class StdArenaAllocator {
public:
  Arena *_arena;
  typedef size_t size_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T value_type;

  pointer allocate(const size_type n)
  {
    return (pointer)_arena->allocate(n * sizeof(T));
  }

  pointer allocate(const size_type n, const void *hint)
  {
    return allocate(n);
  }

  void deallocate(const_pointer p, const size_type n) {}

  template<typename U>
  bool operator==(const StdArenaAllocator<U> &other) const { return _arena == other._arena; }
  template<typename U>
  bool operator!=(const StdArenaAllocator<U> &other) const { return _arena != other._arena; }

  template<typename O>
  StdArenaAllocator(const StdArenaAllocator<O> &a) : StdArenaAllocator(a._arena) {}
  StdArenaAllocator(Arena *arena) : _arena(arena) { assert(arena); }
  ~StdArenaAllocator() {}
};

template<typename K, typename V, typename Cmp = std::less<K>>
struct ArenaMap : public std::map<K, V, Cmp, StdArenaAllocator<std::pair<const K, V>>> {

  typedef StdArenaAllocator<std::pair<const K, V>> Allocator;

  ArenaMap(const Allocator &allocator) : std::map<K, V, Cmp, Allocator>(allocator) {}
};

template<typename V, typename Cmp = std::less<V>>
struct ArenaSet : public std::set<V, Cmp, StdArenaAllocator<V>> {

  typedef StdArenaAllocator<V> Allocator;

  ArenaSet(const Allocator &allocator) : std::set<V, Cmp, Allocator>(allocator) {}
};

template<typename V, typename Hasher = std::hash<V>, typename KeyEq = std::equal_to<V>>
struct ArenaUnorderedSet : public std::unordered_set<V, Hasher, KeyEq, StdArenaAllocator<V>> {

  typedef StdArenaAllocator<V> Allocator;

  ArenaUnorderedSet(const Allocator &allocator) : std::unordered_set<V, Hasher, KeyEq, Allocator>(allocator) {}
};

template<typename K, typename V, typename Hasher = std::hash<K>, typename KeyEq = std::equal_to<K>>
struct ArenaUnorderedMap : public std::unordered_map<K, V, Hasher, KeyEq, StdArenaAllocator<std::pair<const K, V>>> {

  typedef StdArenaAllocator<std::pair<const K, V>> Allocator;

  ArenaUnorderedMap(const Allocator &allocator) : std::unordered_map<K, V, Hasher, KeyEq, Allocator>(allocator) {}
};
