/*  see copyright notice in squirrel.h */
#ifndef _SQUTILS_H_
#define _SQUTILS_H_

#include <memory>

typedef struct SQAllocContextT * SQAllocContext;

void sq_vm_init_alloc_context(SQAllocContext * ctx);
void sq_vm_destroy_alloc_context(SQAllocContext * ctx);
void sq_vm_assign_to_alloc_context(SQAllocContext ctx, HSQUIRRELVM vm);
void *sq_vm_malloc(SQAllocContext ctx, SQUnsignedInteger size);
void *sq_vm_realloc(SQAllocContext ctx, void *p,SQUnsignedInteger oldsize,SQUnsignedInteger size);
void sq_vm_free(SQAllocContext ctx, void *p,SQUnsignedInteger size);

#define sq_new(__ctx,__ptr,__type, ...) {__ptr=(__type *)sq_vm_malloc((__ctx),sizeof(__type));new (__ptr) __type(__VA_ARGS__);}
#define sq_delete(__ctx,__ptr,__type) {__ptr->~__type();sq_vm_free((__ctx),__ptr,sizeof(__type));}
#define SQ_MALLOC(__ctx,__size) sq_vm_malloc((__ctx),(__size));
#define SQ_FREE(__ctx,__ptr,__size) sq_vm_free((__ctx),(__ptr),(__size));
#define SQ_REALLOC(__ctx,__ptr,__oldsize,__size) sq_vm_realloc((__ctx),(__ptr),(__oldsize),(__size));

#define sq_aligning(v) (((size_t)(v) + (SQ_ALIGNMENT-1)) & (~(SQ_ALIGNMENT-1)))

#ifndef SQ_LIKELY
  #if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
    #if defined(__cplusplus)
      #define SQ_LIKELY(x)   __builtin_expect(!!(x), true)
      #define SQ_UNLIKELY(x) __builtin_expect(!!(x), false)
    #else
      #define SQ_LIKELY(x)   __builtin_expect(!!(x), 1)
      #define SQ_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #endif
  #else
    #define SQ_LIKELY(x)   (x)
    #define SQ_UNLIKELY(x) (x)
  #endif
#endif

#define HAVE_STATIC_ASSERT() ((_MSC_VER >= 1600 && !defined(__INTEL_COMPILER)) || (__cplusplus > 199711L))

#if !defined(SQ_STATIC_ASSERT) && HAVE_STATIC_ASSERT()
#define SQ_STATIC_ASSERT(x) static_assert((x), "assertion failed: " #x)
#endif

#ifndef SQ_STATIC_ASSERT
#define SQ_STATIC_ASSERT(x) if (sizeof(char[2*((x)?1:0)-1])) ; else
#endif

#undef HAVE_STATIC_ASSERT

//sqvector mini vector class, supports objects by value
template<typename T> class sqvector
{
public:
    sqvector(SQAllocContext ctx)
        : _vals(NULL)
        ,  _size(0)
        , _allocated(0)
        , _alloc_ctx(ctx)
    {
    }
    sqvector(const sqvector<T>& v)
        : _vals(NULL)
        , _size(0)
        , _allocated(0)
        , _alloc_ctx(v._alloc_ctx)
    {
        copy(v);
    }
    void copy(const sqvector<T>& v)
    {
        if (_alloc_ctx != v._alloc_ctx) {
            _releasedata();
            _vals = NULL;
            _allocated = 0;
            _size = 0;
            _alloc_ctx = v._alloc_ctx;
        }
        else if(_size) {
            resize(0); //destroys all previous stuff
        }
        //resize(v._size);
        if(v._size > _allocated) {
            _realloc(v._size);
        }
        for(SQUnsignedInteger i = 0; i < v._size; i++) {
            new ((void *)&_vals[i]) T(v._vals[i]); //-V522
        }
        _size = v._size;
    }
    ~sqvector()
    {
        _releasedata();
    }
    void reserve(SQUnsignedInteger newsize) { _realloc(newsize); }
    void resize(SQUnsignedInteger newsize, const T& fill = T())
    {
        if(newsize > _allocated)
            _realloc(newsize);
        if(newsize > _size) {
            while(_size < newsize) {
                new ((void *)&_vals[_size]) T(fill);
                _size++;
            }
        }
        else{
            for(SQUnsignedInteger i = newsize; i < _size; i++) {
                _vals[i].~T();
            }
            _size = newsize;
        }
    }
    void shrinktofit() { if(_size > 4) { _realloc(_size); } }
    T& top() const { return _vals[_size - 1]; }
    inline SQUnsignedInteger size() const { return _size; }
    bool empty() const { return (_size <= 0); }
    inline T &push_back(const T& val = T())
    {
        if(_allocated <= _size)
            _realloc(_size * 2);
        return *(new ((void *)&_vals[_size++]) T(val));
    }
    inline void pop_back()
    {
        _size--; _vals[_size].~T();
    }
    void insert(SQUnsignedInteger idx, const T& val)
    {
        resize(_size + 1);
        for(SQUnsignedInteger i = _size - 1; i > idx; i--) {
            _vals[i] = _vals[i - 1];
        }
        _vals[idx] = val;
    }
    void remove(SQUnsignedInteger idx)
    {
        _vals[idx].~T();
        if(idx < (_size - 1)) {
            memmove(&_vals[idx], &_vals[idx+1], sizeof(T) * (_size - idx - 1));
        }
        _size--;
    }
    SQUnsignedInteger capacity() { return _allocated; }
    inline T &back() const { return _vals[_size - 1]; }
    inline T& operator[](SQUnsignedInteger pos) const{ return _vals[pos]; }
    T* _vals;
    SQAllocContext _alloc_ctx;

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin() { return &_vals[0]; }
    const_iterator begin() const { return &_vals[0]; }
    iterator end() { return &_vals[_size]; }
    const_iterator end() const { return &_vals[_size]; }
private:
    void _realloc(SQUnsignedInteger newsize)
    {
        newsize = (newsize > 0)?newsize:4;
        _vals = (T*)SQ_REALLOC(_alloc_ctx, _vals, _allocated * sizeof(T), newsize * sizeof(T));
        _allocated = newsize;
    }
    void _releasedata()
    {
        if(_allocated) {
            for(SQUnsignedInteger i = 0; i < _size; i++)
                _vals[i].~T();
            SQ_FREE(_alloc_ctx, _vals, (_allocated * sizeof(T)));
        }
    }
    SQUnsignedInteger _size;
    SQUnsignedInteger _allocated;
};

#endif //_SQUTILS_H_
