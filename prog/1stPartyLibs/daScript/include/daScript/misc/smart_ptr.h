#pragma once

#ifndef DAS_SMART_PTR_ID
#define DAS_SMART_PTR_ID    0
#endif

#if DAS_SMART_PTR_TRACKER
#include <atomic>
#endif

void os_debug_break();

namespace das {

    struct AstSerializer;

    template<typename T, typename TP>
    class smart_ptr;

    template <typename T>
    struct smart_ptr_policy;

    struct smart_ptr_jit { };

    template <typename T>
    struct smart_ptr_raw {
        smart_ptr_raw ( smart_ptr_jit & p ) : ptr((T *)&p) {}
        explicit operator smart_ptr_jit * () { return (smart_ptr_jit *)ptr; }
        smart_ptr_raw () : ptr(nullptr) {}
        smart_ptr_raw ( T * p ) : ptr(p) {}
        template <typename Y>
        __forceinline smart_ptr_raw ( const smart_ptr_raw<Y> & p ) {
            static_assert( is_base_of<T,Y>::value || is_base_of<Y,T>::value, "can only cast if inherited" );
            ptr = (T *) p.get();
        }
        __forceinline T * get() const {
            return ptr;
        }
        __forceinline T * operator -> () const {
            DAS_ASSERT(ptr && "null pointer dereference");
            return ptr;
        }
        __forceinline T & operator * () const {
            DAS_ASSERT(ptr && "null pointer dereference");
            return *ptr;
        }
        __forceinline operator smart_ptr_raw<void> & () const {
            return *((smart_ptr_raw<void> *)this);
        }
        __forceinline operator bool() const {
            return ptr != nullptr;
        }
        __forceinline smart_ptr<T, smart_ptr_policy<T>> marshal() const {
            if ( ptr ) {
                smart_ptr<T, smart_ptr_policy<T>> res = ptr;
                DAS_VERIFY ( !smart_ptr_policy<T>::delRef(ptr) );
                return res;
            } else {
                return ptr;
            }
        }
        __forceinline smart_ptr<T, smart_ptr_policy<T>> marshal( const smart_ptr<T, smart_ptr_policy<T>> & expr ) const {
            if ( !ptr  ) {
                return nullptr;
            } else if ( ptr!=expr.get() ) {
                smart_ptr<T, smart_ptr_policy<T>> res = ptr;
                DAS_VERIFY ( !smart_ptr_policy<T>::delRef(ptr) );
                return res;
            } else {
                return expr;
            }
        }
        __forceinline operator smart_ptr<T, smart_ptr_policy<T>> & () {
            using SPT = smart_ptr<T, smart_ptr_policy<T>>;
            return *(SPT *)this;
        }
        __forceinline operator const smart_ptr<T, smart_ptr_policy<T>> & () const {
            using SPT = smart_ptr<T, smart_ptr_policy<T>>;
            return *(const SPT *)this;
        }
        T * ptr;
    };

    template <>
    struct smart_ptr_raw<void> {
        smart_ptr_raw ( smart_ptr_jit & p ) : ptr((void *)&p) {}
        explicit operator smart_ptr_jit * () { return (smart_ptr_jit *)ptr; }
        smart_ptr_raw () : ptr(nullptr) {}
        smart_ptr_raw ( void * p ) : ptr(p) {}
        __forceinline void * get() const { return ptr; }
        __forceinline void * operator -> () const { return ptr; }
        void * ptr;
    };

    template <typename T>
    struct smart_ptr_policy {
        __forceinline static void addRef ( T * p ) { p->addRef(); }
        __forceinline static bool delRef ( T * p ) { return p->delRef(); }
        __forceinline static unsigned int use_count ( T * p ) { return p->use_count(); }
        __forceinline static T & get_value ( T * p ) {
            DAS_ASSERT(p && "null pointer dereference");
            return *p;
        }
    };

    template <>
    struct smart_ptr_policy<void> {
        __forceinline static void addRef ( void * ) { }
        __forceinline static bool delRef ( void * ) { return false; }
        __forceinline static unsigned int use_count ( void * ) { return 0; }
        __forceinline static void * get_value ( void * p ) {
            DAS_ASSERT(p && "null pointer dereference");
            return p;
        }
    };

    template<typename T, typename TP = smart_ptr_policy<T>>
    class smart_ptr {
    public:
        using element_type = T;
        using element_type_ptr = T *;
        smart_ptr ( smart_ptr_jit & p ) {
            init((T *)&p);
        }
        explicit operator smart_ptr_jit * () {
            addRef();   // this is only during jit wrapping. we cast to this. old one gets deleted. we need to add ref.
            return (smart_ptr_jit *)ptr;
        }
        __forceinline smart_ptr ( ) {
            ptr = nullptr;
        }
        __forceinline smart_ptr ( const smart_ptr & p ) {
            init(p.ptr);
        }
        template <typename Y>
        __forceinline smart_ptr ( const smart_ptr<Y> & p ) {
            static_assert( is_base_of<T,Y>::value, "can only cast if inherited" );
            init(p.get());
        }
        template <typename Y>
        __forceinline smart_ptr ( const smart_ptr_raw<Y> & p ) {
            static_assert( is_base_of<T,Y>::value, "can only cast if inherited" );
            init(p.get());
        }
        __forceinline operator smart_ptr_raw<void> & () const {
            return *((smart_ptr_raw<void> *)this);
        }
        __forceinline operator smart_ptr_raw<T> & () const {
            return *((smart_ptr_raw<T> *)this);
        }
        __forceinline smart_ptr ( smart_ptr && p ) {
            ptr = p.ptr;
            p.ptr = nullptr;
        }
        __forceinline smart_ptr ( T * p ) {
            init(p);
        }
        __forceinline ~smart_ptr() {
            reset();
        }
        __forceinline smart_ptr & operator = ( const smart_ptr & p ) {
            return set(p.ptr);
        }
        template <typename Y>
        __forceinline smart_ptr & operator = ( const smart_ptr<Y> & p ) {
            return set(p.get());
        }
        __forceinline smart_ptr & operator = ( smart_ptr && p ) {
            set(p.get());
            p.set(nullptr);
            return *this;
        }
        __forceinline smart_ptr & operator = ( T * p ) {
            return set(p);
        }
        __forceinline void reset() {
            T * t = ptr;
            ptr = nullptr;
            if ( t ) TP::delRef(t);
        }
        __forceinline void swap ( smart_ptr & p ) {
            T * t = ptr;
            ptr = p.ptr;
            p.ptr = t;
        }
        __forceinline void reset ( T * p ) {
            set(p);
        }
        __forceinline decltype(TP::get_value(element_type_ptr())) operator * () const {
            return TP::get_value(ptr);
        }
        __forceinline T * operator -> () const {
            DAS_ASSERT(ptr && "null pointer dereference");
            return ptr;
        }
        __forceinline T * get() const {
            return ptr;
        }
        __forceinline T * orphan() {
            T * t = ptr;
            ptr = nullptr;
            return t;
        }
        __forceinline operator bool() const {
            return ptr != nullptr;
        }
        __forceinline bool operator == ( T * p ) const {
            return ptr == p;
        }
        template <typename Y>
        __forceinline bool operator == ( const smart_ptr<Y> & p ) const {
            return ptr == p.get();
        }
        __forceinline bool operator != ( T * p ) const {
            return ptr != p;
        }
        template <typename Y>
        __forceinline bool operator != ( const smart_ptr<Y> & p ) const {
            return ptr != p.get();
        }
        __forceinline bool operator >= ( T * p ) const {
            return ptr >= p;
        }
        template <typename Y>
        __forceinline bool operator >= ( const smart_ptr<Y> & p ) const {
            return ptr >= p.get();
        }
        __forceinline bool operator <= ( T * p ) const {
            return ptr <= p;
        }
        template <typename Y>
        __forceinline bool operator <= ( const smart_ptr<Y> & p ) const {
            return ptr <= p.get();
        }
        __forceinline bool operator > ( T * p ) const {
            return ptr > p;
        }
        template <typename Y>
        __forceinline bool operator > ( const smart_ptr<Y> & p ) const {
            return ptr > p.get();
        }
        __forceinline bool operator < ( T * p ) const {
            return ptr < p;
        }
        template <typename Y>
        __forceinline bool operator < ( const smart_ptr<Y> & p ) const {
            return ptr < p.get();
        }
    protected:
        __forceinline smart_ptr & set ( T * p )  {
            T * t = ptr;
            ptr = p;
            addRef();
            if ( t ) TP::delRef(t);
            return *this;
        }
        __forceinline void init ( T * p = nullptr )  {
            ptr = p;
            addRef();
        }
        __forceinline void addRef() {
            if ( ptr ) TP::addRef(ptr);
        }
        __forceinline void delRef() {
            if ( ptr && TP::delRef(ptr) ) ptr = nullptr;
        }
    protected:
        T * ptr;
    };

    template <typename T>   struct is_smart_ptr { enum { value = false }; };
    template <typename T>   struct is_smart_ptr < smart_ptr<T> > { enum { value = true }; };
    template <typename T>   struct is_smart_ptr < smart_ptr_raw<T> > { enum { value = true }; };

    template <class T, class U>
    __forceinline bool operator == (T * l, const smart_ptr<U> & r) {
        return l == r.get();
    }
    template <class T, class U>
    __forceinline bool operator != (T * l, const smart_ptr<U> & r) {
        return l != r.get();
    }
    template <class T, class U>
    __forceinline bool operator >= (T * l, const smart_ptr<U> & r) {
        return l >= r.get();
    }
    template <class T, class U>
    __forceinline bool operator <= (T * l, const smart_ptr<U> & r) {
        return l <= r.get();
    }
    template <class T, class U>
    __forceinline bool operator > (T * l, const smart_ptr<U> & r) {
        return l > r.get();
    }
    template <class T, class U>
    __forceinline bool operator < (T * l, const smart_ptr<U> & r) {
        return l < r.get();
    }

    template< class T, class... Args >
    __forceinline smart_ptr<T> make_smart ( Args&&... args ) {
        return new T(args...);
    }

    template< class T, class U >
    __forceinline smart_ptr<T> static_pointer_cast(const smart_ptr<U> & r) {
        return static_cast<T *>(r.get());
    }

    template< class T, class U >
    __forceinline smart_ptr<T> dynamic_pointer_cast(const smart_ptr<U> & r) {
        return dynamic_cast<T *>(r.get());
    }

    template< class T, class U >
    __forceinline smart_ptr<T> const_pointer_cast(const smart_ptr<U> & r) {
        return const_cast<T *>(r.get());
    }

    template< class T, class U >
    __forceinline smart_ptr<T> reinterpret_pointer_cast(const smart_ptr<U> & r) {
        return reinterpret_cast<T *>(r.get());
    }

#if DAS_SMART_PTR_ID
    #define DAS_NEW_SMART_PTR_ID \
    { \
        lock_guard<mutex> guard(ref_count_mutex); \
        ref_count_id = ++ref_count_total; \
        ref_count_ids.insert(ref_count_id); \
    }
    #define DAS_DELETE_SMART_PTR_ID \
    { \
        lock_guard<mutex> guard(ref_count_mutex); \
        ref_count_ids.erase(ref_count_id); \
    }
    #define DAS_TRACK_SMART_PTR_ID         if ( ref_count_id==ref_count_track ) os_debug_break();
#else
    #define DAS_NEW_SMART_PTR_ID
    #define DAS_TRACK_SMART_PTR_ID
    #define DAS_DELETE_SMART_PTR_ID
#endif

#if DAS_SMART_PTR_TRACKER
    extern atomic<uint64_t>  g_smart_ptr_total;
    #define DAS_SMART_PTR_NEW     g_smart_ptr_total++;
    #define DAS_SMART_PTR_DELETE  g_smart_ptr_total--;
#else
    #define DAS_SMART_PTR_NEW
    #define DAS_SMART_PTR_DELETE
#endif

    class ptr_ref_count {
    public:
#if DAS_SMART_PTR_ID
        uint64_t                    ref_count_id;
        static uint64_t             ref_count_total;
        static uint64_t             ref_count_track;
        static das_set<uint64_t>    ref_count_ids;
        static mutex                ref_count_mutex;
#endif
    public:
        __forceinline ptr_ref_count () {
            DAS_NEW_SMART_PTR_ID
            DAS_SMART_PTR_NEW
            DAS_TRACK_SMART_PTR_ID
        }
        __forceinline ptr_ref_count ( const ptr_ref_count &  ) {
            DAS_NEW_SMART_PTR_ID
            DAS_SMART_PTR_NEW
            DAS_TRACK_SMART_PTR_ID
        }
        __forceinline ptr_ref_count ( const ptr_ref_count && ) {
            DAS_NEW_SMART_PTR_ID
            DAS_SMART_PTR_NEW
            DAS_TRACK_SMART_PTR_ID
        }
        __forceinline ptr_ref_count & operator = ( const ptr_ref_count & ) { return *this;}
        __forceinline ptr_ref_count & operator = ( ptr_ref_count && ) { return *this; }
        virtual ~ptr_ref_count() {
#if DAS_SMART_PTR_MAGIC
            if ( ref_count!=0 ) DAS_FATAL_ERROR("%p ref_count=%i, can't delete", (void *)this, ref_count);
            if ( magic!=0x1ee7c0de ) DAS_FATAL_ERROR("%p magic=%08x, object was deleted or corrupted", (void *)this, magic);
            magic = 0xdeadbeef;
#else
            DAS_ASSERTF(ref_count == 0, "can only delete when ref_count==0");
#endif
            DAS_DELETE_SMART_PTR_ID
            DAS_SMART_PTR_DELETE
        }
        __forceinline void addRef() {
            DAS_TRACK_SMART_PTR_ID
            ref_count ++;
#if DAS_SMART_PTR_MAGIC
            if ( ref_count==0 || magic!=0x1ee7c0de ) {
                DAS_FATAL_ERROR("%p ref_count=%i, magic=%08x, object was deleted or corrupted", (void *)this, ref_count, magic);
            }
#else
            DAS_ASSERTF(ref_count, "ref_count overflow");
#endif
        }
        __forceinline bool delRef() {
            DAS_TRACK_SMART_PTR_ID
#if DAS_SMART_PTR_MAGIC
            if ( ref_count==0 || magic!=0x1ee7c0de ) {
                DAS_FATAL_ERROR("%p ref_count=%i, magic=%08x, object was deleted or corrupted", (void *)this, ref_count, magic);
            }
#else
            DAS_ASSERTF(ref_count, "deleting reference on the object with ref_count==0");
#endif
            if ( --ref_count==0 ) {
                delete this;
                return true;
            } else {
                return false;
            }
        }
        __forceinline unsigned int use_count() const {
            return ref_count;
        }
        __forceinline bool is_valid() const {
#if DAS_SMART_PTR_MAGIC
            return magic==0x1ee7c0de && ref_count!=0;
#else
            return ref_count!=0;
#endif
        }
        void serialize ( AstSerializer & ser );
    private:
#if DAS_SMART_PTR_MAGIC
        unsigned int magic = 0x1ee7c0de;
#endif
        unsigned int ref_count = 0;
    };

    struct smart_ptr_hash {
        template<typename TT>
        std::size_t operator() ( const das::smart_ptr<TT> & k ) const {
            return hash<void *>()(k.get());
        }
    };
}
