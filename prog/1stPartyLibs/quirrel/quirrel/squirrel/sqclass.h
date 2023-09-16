/*  see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

struct SQInstance;

struct SQClassMember {
    SQObjectPtr val;
    void Null() {
        val.Null();
    }
};

typedef sqvector<SQClassMember> SQClassMemberVec;

#define MEMBER_MAX_COUNT_BIT_SHIFT 22
#define MEMBER_TYPE_FIELD (0x01<<MEMBER_MAX_COUNT_BIT_SHIFT)
#define MEMBER_TYPE_METHOD (0x02<<MEMBER_MAX_COUNT_BIT_SHIFT)
#define MEMBER_MAX_COUNT ((1<<MEMBER_MAX_COUNT_BIT_SHIFT)-1)

#define _isfieldi(o) ((o)&MEMBER_TYPE_FIELD)
#define _member_idxi(o) ((o)&MEMBER_MAX_COUNT)
#define _isfield(o) (_isfieldi(_integer(o)))
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|(i)))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|(i)))
#define _member_idx(o) (_member_idxi(_integer(o)))

struct SQClass : public CHAINABLE_OBJ
{
    SQClass(SQSharedState *ss,SQClass *base);
public:
    static SQClass* Create(SQSharedState *ss,SQClass *base) {
        SQClass *newclass = (SQClass *)SQ_MALLOC(ss->_alloc_ctx, sizeof(SQClass));
        new (newclass) SQClass(ss, base);
        return newclass;
    }
    ~SQClass();
    bool NewSlot(SQSharedState *ss, const SQObjectPtr &key,const SQObjectPtr &val,bool bstatic);
    bool Get(const SQObjectPtr &key,SQObjectPtr &val) const {
        if(_members->Get(key,val)) {
            if(_isfield(val)) {
                SQObjectPtr &o = _defaultvalues[_member_idx(val)].val;
                val = _realval(o);
            }
            else {
                val = _methods[_member_idx(val)].val;
            }
            return true;
        }
        return false;
    }
    bool GetConstructor(SQObjectPtr &ctor)
    {
        if(_constructoridx != -1) {
            ctor = _methods[_constructoridx].val;
            return true;
        }
        return false;
    }
    void Lock() { _lockedTypeId = currentHint(); if(_base) _base->Lock(); }
    void Release() {
        if (_hook) { _hook(_typetag,0);}
        SQAllocContext ctx = _methods._alloc_ctx;
        sq_delete(ctx, this, SQClass);
    }
    uint64_t lockedTypeId() const {return _lockedTypeId;}

    enum {CLASS_BITS = 40};
    //all x64 bit platforms have only 48 meaningful bits in pointers. Yet, 3 lsb are always zero (or meaningless, as pointers are aligned)
    //actually, even more bits are meaningless - as any class can't alias with other, there should be at least sizeof(SQClass) difference
    //sizeof is at least 256 on all platforms, so there are no more than 40 meaningful bits (after first 8)
    //still more than 32, unfortunately
    bool isLocked() const {return _lockedTypeId != 0;}
    static uint64_t classTypeFromHint(uint64_t hint)
    {
      SQ_STATIC_ASSERT(sizeof(SQClass)>=256);
      return hint&((1ull<<uint64_t(CLASS_BITS)) - 1);
    }
    uint64_t currentHint() const {return classTypeFromHint(uintptr_t(this)>>uintptr_t(8));}
    void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable ** );
    SQObjectType GetType() {return OT_CLASS;}
#endif
    SQInteger Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
    SQInstance *CreateInstance();
    SQTable *_members;
    SQClass *_base;
    SQClassMemberVec _defaultvalues;
    SQClassMemberVec _methods;
    SQObjectPtr _metamethods[MT_LAST];
    SQUserPointer _typetag;
    SQRELEASEHOOK _hook;
    SQInteger _constructoridx;
    SQInteger _udsize;
    uint64_t _lockedTypeId;
};

#define calcinstancesize(_theclass_) \
    (_theclass_->_udsize + sq_aligning(sizeof(SQInstance) +  (sizeof(SQObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))

struct SQInstance : public SQDelegable
{
    void Init(SQSharedState *ss);
    SQInstance(SQSharedState *ss, SQClass *c, SQInteger memsize);
    SQInstance(SQSharedState *ss, SQInstance *c, SQInteger memsize);
public:
    static SQInstance* Create(SQSharedState *ss,SQClass *theclass) {

        SQInteger size = calcinstancesize(theclass);
        SQInstance *newinst = (SQInstance *)SQ_MALLOC(ss->_alloc_ctx, size);
        new (newinst) SQInstance(ss, theclass,size);
        if(theclass->_udsize) {
            newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
        }
        return newinst;
    }
    SQInstance *Clone(SQSharedState *ss)
    {
        SQInteger size = calcinstancesize(_class);
        SQInstance *newinst = (SQInstance *)SQ_MALLOC(ss->_alloc_ctx, size);
        new (newinst) SQInstance(ss, this,size);
        if(_class->_udsize) {
            newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
        }
        return newinst;
    }
    ~SQInstance();
    inline void GetMember(uint32_t idx, SQObjectPtr &val) const {
        if (_isfieldi(idx))
            val = _realval(_values[_member_idxi(idx)]);
        else
            val = _class->_methods[_member_idxi(idx)].val;
    }
    bool Get(const SQObjectPtr &key,SQObjectPtr &val) const {
        if(_class->_members->Get(key,val)) {
            GetMember(_integer(val), val);
            return true;
        }
        return false;
    }
    __forceinline void SetMemberField(uint32_t idx, const SQObjectPtr &val) {
        _values[_member_idxi(idx)] = val;
    }
    bool Set(const SQObjectPtr &key,const SQObjectPtr &val) {
        SQObjectPtr idx;
        if(_class->_members->Get(key,idx) && _isfield(idx)) {
            SetMemberField(_integer(idx), val);
            return true;
        }
        return false;
    }
    void Release() {
        _uiRef++;
        if (_hook) { _hook(_userpointer,0);}
        _uiRef--;
        if(_uiRef > 0) return;
        SQInteger size = _memsize;
        SQAllocContext ctx = _alloc_ctx;
        this->~SQInstance();
        SQ_FREE(ctx, this, size);
    }
    void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable ** );
    SQObjectType GetType() {return OT_INSTANCE;}
#endif
    bool InstanceOf(SQClass *trg);
    bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);

    SQClass *_class;
    SQUserPointer _userpointer;
    SQRELEASEHOOK _hook;
    SQAllocContext _alloc_ctx;
    SQInteger _memsize;
    SQObjectPtr _values[1];
};

#endif //_SQCLASS_H_
