/*  see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include "squtils.h"

#define UINT32_MINUS_ONE (0xFFFFFFFF)

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct SQSharedState;

#define METAMETHODS_LIST \
    MM_IMPL(MT_ADD      ,"_add")\
    MM_IMPL(MT_SUB      ,"_sub")\
    MM_IMPL(MT_MUL      ,"_mul")\
    MM_IMPL(MT_DIV      ,"_div")\
    MM_IMPL(MT_UNM      ,"_unm")\
    MM_IMPL(MT_MODULO   ,"_modulo")\
    MM_IMPL(MT_SET      ,"_set")\
    MM_IMPL(MT_GET      ,"_get")\
    MM_IMPL(MT_TYPEOF   ,"_typeof")\
    MM_IMPL(MT_NEXTI    ,"_nexti")\
    MM_IMPL(MT_CMP      ,"_cmp")\
    MM_IMPL(MT_CALL     ,"_call")\
    MM_IMPL(MT_CLONED   ,"_cloned")\
    MM_IMPL(MT_NEWSLOT  ,"_newslot")\
    MM_IMPL(MT_DELSLOT  ,"_delslot")\
    MM_IMPL(MT_TOSTRING ,"_tostring")\
    MM_IMPL(MT_LOCK     ,"_lock")\


#define MM_IMPL(mm, name) mm,

enum SQMetaMethod{
    METAMETHODS_LIST
    MT_NUM_METHODS
};

#undef MM_IMPL


#define _CONSTRUCT_VECTOR(type,size,ptr) { \
    for(SQInteger n = 0; n < ((SQInteger)size); n++) { \
            new (&ptr[n]) type(); \
        } \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
    for(SQInteger nl = 0; nl < ((SQInteger)size); nl++) { \
            ptr[nl].~type(); \
    } \
}

#define _COPY_VECTOR(dest,src,size) { \
    for(SQInteger _n_ = 0; _n_ < ((SQInteger)size); _n_++) { \
        dest[_n_] = src[_n_]; \
    } \
}

#define _NULL_SQOBJECT_VECTOR(vec,size) { \
    for(SQInteger _n_ = 0; _n_ < ((SQInteger)size); _n_++) { \
        vec[_n_].Null(); \
    } \
}


struct SQRefCounted
{
    SQUnsignedInteger _uiRef;
    struct SQWeakRef *_weakref;
    SQRefCounted() { _uiRef = 0; _weakref = NULL; }
    virtual ~SQRefCounted();
    SQWeakRef *GetWeakRef(SQAllocContext alloc_ctx, SQObjectType type, SQObjectFlags flags);
    virtual void Release()=0;

};

struct SQWeakRef : SQRefCounted
{
    void Release();
    SQObject _obj;
    SQAllocContext _alloc_ctx;
};

#define _realval(o) (sq_type((o)) != OT_WEAKREF?(SQObject)o:_weakref(o)->_obj)

struct SQObjectPtr;

#define __AddRef(type,unval) if(ISREFCOUNTED(type)) \
        { \
            unval.pRefCounted->_uiRef++; \
        }

#define __Release(type,unval) if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)==0))  \
        {   \
            unval.pRefCounted->Release();   \
        }

#define __ObjRelease(obj) { \
    if((obj)) { \
        (obj)->_uiRef--; \
        if((obj)->_uiRef == 0) \
            (obj)->Release(); \
        (obj) = NULL;   \
    } \
}

#define __ObjAddRef(obj) { \
    (obj)->_uiRef++; \
}

#define is_delegable(t) (sq_type(t)&SQOBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((SQDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _outer(obj) ((obj)._unVal.pOuter)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) ((SQUserPointer)sq_aligning((obj)._unVal.pUserData + 1))

#define tofloat(num) ((sq_type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
#define tointeger(num) ((sq_type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
#define SQ_REFOBJECT_INIT() SQ_OBJECT_RAWINIT()
#else
#define SQ_REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
    explicit SQObjectPtr(_class * __restrict x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _flags=0; \
        _unVal.sym = x; \
        assert(_unVal.pTable); \
        _unVal.pRefCounted->_uiRef++; \
    } \
    inline SQObjectPtr& operator=(_class *__restrict x) \
    {  \
        SQObjectType  tOldType = _type; \
        SQObjectValue unOldVal = _unVal; \
        _type = type; \
        _flags = 0; \
        SQ_REFOBJECT_INIT() \
        _unVal.sym = x; \
        _unVal.pRefCounted->_uiRef++; \
        __Release(tOldType,unOldVal); \
        return *this; \
    }

#define _SCALAR_TYPE_DECL(type,_class,sym) \
    explicit SQObjectPtr(_class x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _flags = 0; \
        _unVal.sym = x; \
    } \
    inline SQObjectPtr& operator=(_class x) \
    {  \
        __Release(_type,_unVal); \
        _type = type; \
        _flags = 0; \
        SQ_OBJECT_RAWINIT() \
        _unVal.sym = x; \
        return *this; \
    }
struct SQObjectPtr : public SQObject
{
    SQObjectPtr()
    {
        memset(this, 0, sizeof(SQObjectPtr));
        _type=OT_NULL;
    }
    SQObjectPtr(const SQObjectPtr &__restrict o)
    {
        memcpy(this, &o, sizeof(o));
        __AddRef(_type,_unVal);
    }
    explicit SQObjectPtr(const SQObject &__restrict o)
    {
        memcpy(this, &o, sizeof(o));
        __AddRef(_type,_unVal);
    }
    _REF_TYPE_DECL(OT_TABLE,SQTable,pTable)
    _REF_TYPE_DECL(OT_CLASS,SQClass,pClass)
    _REF_TYPE_DECL(OT_INSTANCE,SQInstance,pInstance)
    _REF_TYPE_DECL(OT_ARRAY,SQArray,pArray)
    _REF_TYPE_DECL(OT_CLOSURE,SQClosure,pClosure)
    _REF_TYPE_DECL(OT_NATIVECLOSURE,SQNativeClosure,pNativeClosure)
    _REF_TYPE_DECL(OT_OUTER,SQOuter,pOuter)
    _REF_TYPE_DECL(OT_GENERATOR,SQGenerator,pGenerator)
    _REF_TYPE_DECL(OT_STRING,SQString,pString)
    _REF_TYPE_DECL(OT_USERDATA,SQUserData,pUserData)
    _REF_TYPE_DECL(OT_WEAKREF,SQWeakRef,pWeakRef)
    _REF_TYPE_DECL(OT_THREAD,SQVM,pThread)
    _REF_TYPE_DECL(OT_FUNCPROTO,SQFunctionProto,pFunctionProto)

    _SCALAR_TYPE_DECL(OT_INTEGER,SQInteger,nInteger)
    _SCALAR_TYPE_DECL(OT_FLOAT,SQFloat,fFloat)
    _SCALAR_TYPE_DECL(OT_USERPOINTER,SQUserPointer,pUserPointer)

    SQObjectPtr(SQVM *vm, const SQChar *str, SQInteger len = -1);

    explicit SQObjectPtr(bool bBool)
    {
        memset(this, 0, sizeof(SQObjectPtr));
        _type = OT_BOOL;
        if (bBool)
            _unVal.nInteger = 1;
    }
    inline SQObjectPtr& operator=(bool b)
    {
        __Release(_type,_unVal);
        SQ_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _flags = 0;
        _unVal.nInteger = b?1:0;
        return *this;
    }

    ~SQObjectPtr()
    {
        __Release(_type,_unVal);
    }

    inline SQObjectPtr& operator=(const SQObjectPtr& __restrict obj)
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal =_unVal;
        memcpy(this, &obj, sizeof(SQObjectPtr));
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline SQObjectPtr& operator=(const SQObject& __restrict obj)
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal =_unVal;
        memcpy(this, &obj, sizeof(SQObject));
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline void Null()
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal = _unVal;
        memset(this,0, sizeof(SQObjectPtr));
        _type = OT_NULL;
        __Release(tOldType ,unOldVal);
    }
    private:
        SQObjectPtr(const SQChar *){} //safety
};


inline void _Swap(SQObject &a,SQObject &b)
{
    SQObject t = a;
    a = b;
    b = t;
}

/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
struct SQCollectable : public SQRefCounted {
    SQCollectable *_gc_next;
    SQCollectable *_gc_prev;
    SQSharedState *_sharedstate;
    virtual SQObjectType GetType()=0;
    virtual void Release()=0;
    virtual void Mark(SQCollectable **chain)=0;
    void UnMark();
    virtual void Finalize()=0;
    static void AddToChain(SQCollectable **chain,SQCollectable *c);
    static void RemoveFromChain(SQCollectable **chain,SQCollectable *c);
};


#define ADD_TO_CHAIN(chain,obj) AddToChain(chain,obj)
#define REMOVE_FROM_CHAIN(chain,obj) {if(!(_uiRef&MARK_FLAG))RemoveFromChain(chain,obj);}
#define CHAINABLE_OBJ SQCollectable
#define INIT_CHAIN() {_gc_next=NULL;_gc_prev=NULL;_sharedstate=ss;}
#else

// Need this to keep SQSharedState pointer to access alloc_ctx
// Otherwise, just use SQRefCounted as CHAINABLE_OBJ in the way it was initially
struct SQRefCountedWithSharedState : public SQRefCounted
{
    SQSharedState *_sharedstate;
};
#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ SQRefCountedWithSharedState
#define INIT_CHAIN() {_sharedstate=ss;}

#endif

struct SQDelegable : public CHAINABLE_OBJ {
    bool SetDelegate(SQTable *m);
    virtual bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);
    SQTable *_delegate;
};

inline SQUnsignedInteger TranslateIndex(const SQObjectPtr &idx)
{
    switch(sq_type(idx)){
        case OT_NULL:
            return 0;
        case OT_INTEGER:
            return (SQUnsignedInteger)_integer(idx);
        default: assert(0); break;
    }
    return 0;
}

typedef sqvector<SQObjectPtr> SQObjectPtrVec;
typedef sqvector<SQInteger> SQIntVec;
const SQChar *GetTypeName(const SQObject &obj1);
const SQChar *IdType2Name(SQObjectType type);



#endif //_SQOBJECT_H_
