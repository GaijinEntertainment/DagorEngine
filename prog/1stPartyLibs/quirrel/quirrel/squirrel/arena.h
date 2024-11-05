#pragma once

#include "squtils.h"

#include <cstring>
#include <memory>

#include <map>
#include <set>
#include <unordered_map>

#define ARENA_USE_SYSTEM_ALLOC 0

#if ARENA_USE_SYSTEM_ALLOC

#if defined(_WIN32)  || defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <memoryapi.h>
#else // __unix__
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#  define MAP_ANONYMOUS	0x20
#endif
#endif // defined(_WIN32)  || defined(_WIN64)

#ifdef Yield
#undef Yield
#endif

#ifdef max
#undef max
#endif


#endif // ARENA_USE_SYSTEM_ALLOC


#define PAGE_SIZE 4096
#define ALIGN_SIZE(len, align) (((len)+(align - 1)) & ~((align)-1))
#define ALIGN_PTR(ptr, align) (((~((uintptr_t)(ptr))) + 1) & ((align) - 1))
#define ALIGN_SIZE_TO_PAGE(len) ALIGN_SIZE(len, PAGE_SIZE)
#define ALIGN_SIZE_TO_WORD(len) ALIGN_SIZE(len, 0x8)

class Arena {
public:

    Arena(SQAllocContext alloc_ctx, const SQChar *name, size_t chunkSize = 4 * PAGE_SIZE)
        : _alloc_ctx(alloc_ctx), _name(name), _chunks(NULL), _bigChunks(NULL)
        , _chunkSize(ALIGN_SIZE_TO_PAGE(chunkSize)) {
    }

    ~Arena() {
        release();
    }

    uint8_t *allocate(size_t size) {
        size = ALIGN_SIZE_TO_WORD(size);

        if (size > maxChunkSize) {
            return allocateBigChunk(size);
        }
        else {
            return allocatePull(size);
        }
    }

    void release() {
        struct BigChunk *bch = _bigChunks;
        while (bch) {
            struct BigChunk *cur = bch;
            bch = cur->_next;
            cur->~BigChunk();
            SQ_FREE(_alloc_ctx, cur, sizeof(BigChunk));
        }
        _bigChunks = NULL;

        struct Chunk *ch = _chunks;
        while (ch) {
            struct Chunk *cur = ch;
            ch = cur->_next;
            releasePage(cur->_start, cur->_size);
            SQ_FREE(_alloc_ctx, cur, sizeof(Chunk));
        }
        _chunks = NULL;
    }

private:
    struct Chunk {
        struct Chunk *_next;
        uint8_t *_start;
        uint8_t *_ptr;
        size_t _size;

        size_t allocated() const { return _ptr - _start; }
        size_t left() const { return _size - allocated(); }

        Chunk(struct Chunk *n, uint8_t *p, size_t s)
            : _next(n), _start(p), _ptr(p), _size(s) {
        }

        uint8_t *allocate(size_t size) {
            assert(size <= left());
            uint8_t *res = _ptr;
            _ptr += size;
            return res;
        }
    };

    uint8_t *allocateBigChunk(size_t size) {
        void *mem = SQ_MALLOC(_alloc_ctx, sizeof(BigChunk));
        struct BigChunk *ch = new(mem) BigChunk(_alloc_ctx, _bigChunks, size);
        _bigChunks = ch;
        return ch->_ptr;
    }


    struct Chunk *findChunk(size_t size) {
        struct Chunk *ch = _chunks;
        while (ch) {
            if (size <= ch->left()) return ch;
            ch = ch->_next;
        }

        void *mem = SQ_MALLOC(_alloc_ctx, sizeof(Chunk));
        uint8_t *data = allocatePage(_chunkSize);
        ch = new(mem) Chunk(_chunks, data, _chunkSize);
        _chunks = ch;
        return ch;
    }

    uint8_t *allocatePull(size_t size) {
        struct Chunk *chunk = findChunk(size);
        assert(size <= chunk->left());
        return chunk->allocate(size);
    }

#if ARENA_USE_SYSTEM_ALLOC

    uint8_t *allocatePage(size_t size) {
#if defined(_WIN32)  || defined(_WIN64)
        void *result = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else // __unix__
        void *result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (result == MAP_FAILED) {
            fprintf(stderr, "Cannot allocate %zu bytes via mmap, %s\n", size, strerror(errno));
            exit(ERR_MMAP);
        }
#endif // defined(_WIN32)  || defined(_WIN64)
        memset(result, 0, size);
        return (uint8_t *)result;
    }

    void releasePage(void *ptr, size_t size) {
#if defined(_WIN32)  || defined(_WIN64)
        (void)size;
        VirtualFree(ptr, 0, MEM_RELEASE);
#else // __unix__
        munmap(ptr, size);
#endif // defined(_WIN32)  || defined(_WIN64)
    }

#else // ARENA_USE_SYSTEM_ALLOC

    uint8_t *allocatePage(size_t size) {
        return (uint8_t *)SQ_MALLOC(_alloc_ctx, size);
    }

    void releasePage(void *ptr, size_t size) {
        SQ_FREE(_alloc_ctx, ptr, size);
    }

#endif // ARENA_USE_SYSTEM_ALLOC


    const static size_t maxChunkSize = 0x1000;

    struct Chunk *_chunks;

    struct BigChunk {

        BigChunk(SQAllocContext alloc_ctx, struct BigChunk *next, size_t size) : _alloc_ctx(alloc_ctx), _next(next), _size(size) {
            _ptr = (uint8_t *)SQ_MALLOC(_alloc_ctx, size);
            memset(_ptr, 0, size);
        }

        ~BigChunk() {
            SQ_FREE(_alloc_ctx, _ptr, _size);
        }

        struct BigChunk *_next;
        SQAllocContext _alloc_ctx;
        uint8_t *_ptr;
        size_t _size;
    };

    SQAllocContext _alloc_ctx;

    struct BigChunk *_bigChunks;

    const SQChar *_name;

    size_t _chunkSize;

};


class ArenaObj {
protected:
    ArenaObj() = default;
    virtual ~ArenaObj() = default;

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

    ArenaVector(Arena *arena) :
    _arena(arena),
    _vals(NULL),
    _size(0),
    _allocated(0) {

    }

    inline T &push_back(const T& val = T())
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
    bool empty() const { return (_size <= 0); }

    inline T &back() const { return _vals[_size - 1]; }
    inline T& operator[](size_type pos) const { return _vals[pos]; }

    void insert(size_type pos, T v) {
      assert(pos <= _size);
      if (_size + 1 > _allocated) resize(_size + 1);
      memmove(&_vals[pos + 1], &_vals[pos], (_size - pos) * sizeof(T));
      _vals[pos] = v;
      _size += 1;
    }

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin() { return &_vals[0]; }
    const_iterator begin() const { return &_vals[0]; }
    iterator end() { return &_vals[_size]; }
    const_iterator end() const { return &_vals[_size]; }

    void resize(size_type newSize) {
      if (newSize < _allocated) return;
      _realloc(newSize);
    }

private:

    void _realloc(size_type newsize)
    {
        newsize = (newsize > 0) ? newsize : 4;
        T *newPtr = (T *)_arena->allocate(newsize * sizeof(T));
        memcpy(newPtr, _vals, _size * sizeof(T));
        _vals = newPtr;
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

template<typename K, typename V, typename Hasher = std::hash<K>, typename KeyEq = std::equal_to<K>>
struct ArenaUnorederMap : public std::unordered_map<K, V, Hasher, KeyEq, StdArenaAllocator<std::pair<const K, V>>> {

  typedef StdArenaAllocator<std::pair<const K, V>> Allocator;

  ArenaUnorederMap(const Allocator &allocator) : std::unordered_map<K, V, Hasher, KeyEq, Allocator>(allocator) {}
};
