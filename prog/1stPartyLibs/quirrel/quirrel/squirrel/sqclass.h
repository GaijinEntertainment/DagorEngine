/*  see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

#define SLOT_STATUS_OK         0
#define SLOT_STATUS_NO_MATCH   1
#define SLOT_STATUS_ERROR      2

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
#define MEMBER_TYPE_NATIVE_FIELD (0x03<<MEMBER_MAX_COUNT_BIT_SHIFT)
#define MEMBER_KIND_MASK (0x03<<MEMBER_MAX_COUNT_BIT_SHIFT)
#define MEMBER_MAX_COUNT ((1<<MEMBER_MAX_COUNT_BIT_SHIFT)-1)

// NOTE: _isfieldi uses exact comparison, not bitwise AND.
// (0x03 & 0x01) != 0, so bitwise AND would match NATIVE_FIELD as FIELD.
#define _isfieldi(o) (((o)&MEMBER_KIND_MASK) == MEMBER_TYPE_FIELD)
#define _isnativefieldi(o) (((o)&MEMBER_KIND_MASK) == MEMBER_TYPE_NATIVE_FIELD)
#define _member_idxi(o) ((o)&MEMBER_MAX_COUNT)
#define _isfield(o) (_isfieldi(_integer(o)))
#define _isnativefield(o) (_isnativefieldi(_integer(o)))
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|(i)))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|(i)))
#define _make_native_field_idx(i) ((SQInteger)(MEMBER_TYPE_NATIVE_FIELD|(i)))
#define _member_idx(o) (_member_idxi(_integer(o)))

// Native field type constants are defined in squirrel.h as SQNFT_FLOAT32..SQNFT_BOOL

struct SQNativeFieldDesc {
    uint16_t offset;    // byte offset within inline userdata
    uint8_t type;       // SQNFT_* constant
};

struct SQClass : public CHAINABLE_OBJ
{
    SQClass(SQSharedState *ss, SQClass *base);
public:
    static SQClass* Create(SQVM *v,SQClass *base);
    ~SQClass();
    bool NewSlot(SQSharedState *ss, const SQObjectPtr &key,const SQObjectPtr &val,bool bstatic);
    bool RegisterNativeField(SQSharedState *ss, const SQObjectPtr &key, uint16_t offset, uint8_t type);
    bool Get(const SQObjectPtr &key,SQObjectPtr &val) const {
        if(_members->Get(key,val)) {
            if(_isfield(val)) {
                SQObjectPtr &o = _defaultvalues[_member_idx(val)].val;
                val = _realval(o);
            }
            else if(_isnativefield(val)) {
                val.Null(); // no instance context
            }
            else {
                val = _methods[_member_idx(val)].val;
            }
            return true;
        }
        return false;
    }
    // for compiler use
    bool GetStr(const char *key, int keylen, SQObjectPtr &val) const {
        if(_members->GetStr(key, keylen, val)) {
            if(_isfield(val)) {
                SQObjectPtr &o = _defaultvalues[_member_idx(val)].val;
                val = _realval(o);
            }
            else if(_isnativefield(val)) {
                val.Null(); // no instance context
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
    bool Lock(SQVM *v);
    void Release() {
        if (_hook) { _hook(_thread(_sharedstate->_root_vm),_typetag,0);}
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
      static_assert(sizeof(SQClass)>=256);
      return hint&((1ull<<uint64_t(CLASS_BITS)) - 1);
    }
    uint64_t currentHint() const {return classTypeFromHint(uintptr_t(this)>>uintptr_t(8));}
    void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable ** );
    SQObjectType GetType() {return OT_CLASS;}
#endif
    SQInteger Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
    SQInstance *CreateInstance(SQVM *v);
    SQTable *_members;
    SQClass *_base;
    SQClassMemberVec _defaultvalues;
    SQClassMemberVec _methods;
    sqvector<SQNativeFieldDesc> _nativefields;
    SQObjectPtr _metamethods[MT_NUM_METHODS];
    SQUserPointer _typetag;
    SQRELEASEHOOK _hook;
    SQInteger _constructoridx;
    SQInteger _udsize;
    uint64_t _lockedTypeId;
    bool _is_builtin_type;
    SQObjectType _builtin_type_id;  // only valid if _is_builtin_type is true
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

    // Pointer to the inline userdata area at the tail of the SQInstance allocation.
    // Computed from the allocation layout, immune to sq_setinstanceup() overrides.
    inline void *_inlineud() const {
        return ((unsigned char *)const_cast<SQInstance*>(this)) + (_memsize - _class->_udsize);
    }

    inline void GetNativeField(uint32_t fieldIdx, SQObjectPtr &val) const {
        const SQNativeFieldDesc &desc = _class->_nativefields[fieldIdx];
        const char *p = (const char *)_inlineud() + desc.offset;
        switch (desc.type) {
        case SQNFT_FLOAT32: val = SQObjectPtr((SQFloat)*(const float *)p); break;
        case SQNFT_FLOAT64: val = SQObjectPtr((SQFloat)*(const double *)p); break;
        case SQNFT_INT32:   val = SQObjectPtr((SQInteger)*(const int32_t *)p); break;
        case SQNFT_INT64:   val = SQObjectPtr((SQInteger)*(const int64_t *)p); break;
        case SQNFT_BOOL:    val._type = OT_BOOL; val._unVal.nInteger = *(const bool *)p ? 1 : 0; break;
        }
    }

    inline bool SetNativeField(uint32_t fieldIdx, const SQObjectPtr &val) {
        const SQNativeFieldDesc &desc = _class->_nativefields[fieldIdx];
        char *p = (char *)_inlineud() + desc.offset;
        SQObjectType vt = sq_type(val);
        switch (desc.type) {
        case SQNFT_FLOAT32:
            if (vt == OT_FLOAT) *(float *)p = (float)_float(val);
            else if (vt == OT_INTEGER) *(float *)p = (float)_integer(val);
            else return false;
            break;
        case SQNFT_FLOAT64:
            if (vt == OT_FLOAT) *(double *)p = (double)_float(val);
            else if (vt == OT_INTEGER) *(double *)p = (double)_integer(val);
            else return false;
            break;
        case SQNFT_INT32:
            if (vt == OT_INTEGER) *(int32_t *)p = (int32_t)_integer(val);
            else if (vt == OT_FLOAT) *(int32_t *)p = (int32_t)_float(val);
            else return false;
            break;
        case SQNFT_INT64:
            if (vt == OT_INTEGER) *(int64_t *)p = (int64_t)_integer(val);
            else if (vt == OT_FLOAT) *(int64_t *)p = (int64_t)_float(val);
            else return false;
            break;
        case SQNFT_BOOL:
            if (vt == OT_BOOL || vt == OT_INTEGER) *(bool *)p = _integer(val) != 0;
            else return false;
            break;
        }
        return true;
    }

    inline void GetMember(uint32_t idx, SQObjectPtr &val) const {
        uint32_t kind = idx & MEMBER_KIND_MASK;
        if (kind == MEMBER_TYPE_FIELD)
            val = _realval(_values[_member_idxi(idx)]);
        else if (kind == MEMBER_TYPE_NATIVE_FIELD)
            GetNativeField(_member_idxi(idx), val);
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
    void SetMemberField(uint32_t idx, const SQObjectPtr &val) {
        _values[_member_idxi(idx)] = val;
    }
    SQInteger Set(const SQObjectPtr &key,const SQObjectPtr &val) {
        SQObjectPtr idx;
        if(_class->_members->Get(key,idx)) {
            if (_isfield(idx)) {
                SetMemberField(_integer(idx), val);
                return SLOT_STATUS_OK;
            }
            if (_isnativefield(idx)) {
                return SetNativeField(_member_idx(idx), val)
                    ?  SLOT_STATUS_OK : SLOT_STATUS_ERROR;
            }
        }
        return SLOT_STATUS_NO_MATCH;
    }
    void Release() {
        _uiRef++;
        if (_hook) { _hook(_thread(_sharedstate->_root_vm),_userpointer,0);}
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
    bool InstanceOf(const SQClass *trg) const;
    bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);

    SQClass *_class;
    SQUserPointer _userpointer;
    SQRELEASEHOOK _hook;
    SQAllocContext _alloc_ctx;
    SQInteger _memsize;
    SQObjectPtr _values[1];
};

#endif //_SQCLASS_H_
