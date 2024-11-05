/*
see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include <assert.h>

#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(_BitScanReverse)
#endif

#define _FAST_CLONE

#define MINPOWER2 1

#define CLASS_TYPE_HASH_INIT    14695981039346656037ul
#define CLASS_TYPE_HASH_PRIME   1099511628211ul


static inline uint64_t class_type_hash_update_1(uint64_t hashval, uint8_t x) {
    return (hashval ^ x) * CLASS_TYPE_HASH_PRIME;
}

static inline uint64_t class_type_hash_update_4(uint64_t hashval, uint32_t x) {
    // Unrolled FNV1
    const uint8_t *bytes = (const uint8_t *)&x;
    hashval = (hashval ^ bytes[0]) * CLASS_TYPE_HASH_PRIME;
    hashval = (hashval ^ bytes[1]) * CLASS_TYPE_HASH_PRIME;
    hashval = (hashval ^ bytes[2]) * CLASS_TYPE_HASH_PRIME;
    hashval = (hashval ^ bytes[3]) * CLASS_TYPE_HASH_PRIME;
    return hashval;
}


SQTable::SQTable(SQSharedState *ss,SQInteger nInitialSize)
    : _alloc_ctx(ss->_alloc_ctx)
    , _classTypeId(0)
{
    SQInteger pow2size=MINPOWER2;
    while(nInitialSize>pow2size)pow2size=pow2size<<1;
    AllocNodes(pow2size);
    _usednodes = 0;
    _delegate = NULL;
    INIT_CHAIN();
    ADD_TO_CHAIN(&_sharedstate->_gc_chain,this);
}

void SQTable::Remove(const SQObjectPtr &key)
{
    _HashNode *n = _Get(key, HashObj(key) & _numofnodes_minus_one);
    if (n) {
        n->val.Null();
        n->key.Null();
        VT_CLEAR_SINGLE(n);
        _usednodes--;
        Rehash(false);
        _classTypeId = 0;
    }
}

void SQTable::AllocNodes(SQInteger nSize)
{
    assert((nSize & (nSize-1)) == 0); // pow2
    _HashNode *nodes=(_HashNode *)SQ_MALLOC(_alloc_ctx, sizeof(_HashNode)*nSize);
    _numofnodes_minus_one=(uint32_t)(nSize-1);
    _nodes=nodes;
    _firstfree=&_nodes[_numofnodes_minus_one];
    for (_HashNode *i = nodes, *e = i + nSize; i != e; i++)
        new (i) _HashNode;
}

void SQTable::Rehash(bool force)
{
    SQInteger oldsize=_numofnodes_minus_one+1;
    //prevent problems with the integer division
    if (oldsize < MINPOWER2) oldsize = MINPOWER2;
    _HashNode *nold=_nodes;
    SQInteger nelems=CountUsed();
    if (nelems >= oldsize - oldsize/4)  /* using more than 3/4? */
        AllocNodes(oldsize*2);
    else if (nelems < oldsize/4 &&  /* less than 1/4? */
        oldsize > MINPOWER2)
        AllocNodes(oldsize/2);
    else if (force)
    {
        assert(oldsize > 0);
#if !defined(_MSC_VER) || defined(__clang__)
        unsigned log2 = 31 - __builtin_clz((unsigned)oldsize);
#else
        unsigned long log2;
        _BitScanReverse(&log2, oldsize);
#endif
        AllocNodes(unsigned(1 << (log2 + 1)));
        assert(_numofnodes_minus_one + 1 > oldsize);
    }
    else
        return;
    _usednodes = 0;
    for (SQInteger i=0; i<oldsize; i++) {
        _HashNode *old = nold+i;
        if (sq_type(old->key) != OT_NULL)
            NewSlot(old->key,old->val  VT_REF(old));
    }
    for(SQInteger k=0;k<oldsize;k++)
        nold[k].~_HashNode();
    SQ_FREE(_alloc_ctx, nold, oldsize*sizeof(_HashNode));
}

SQTable *SQTable::Clone()
{
    const uint32_t cnt = _numofnodes_minus_one+1;
    SQTable *__restrict nt=Create(_opt_ss(this), cnt);
#ifdef _FAST_CLONE
    _HashNode *__restrict basesrc = _nodes;
    _HashNode *__restrict basedst = nt->_nodes;
    _HashNode *__restrict src = _nodes;
    _HashNode *__restrict dst = nt->_nodes;
    for(_HashNode *__restrict srcE=src + cnt; src != srcE; src++, dst++) {
        dst->key = src->key;
        dst->val = src->val;
        VT_COPY_SINGLE(src, dst);
        VT_TRACE_SINGLE(dst, dst->val, _ss(this)->_root_vm);
        if(src->next) {
            assert(src->next > basesrc);
            dst->next = basedst + (src->next - basesrc);
            assert(dst != dst->next);
        }
    }
    assert(_firstfree >= basesrc);
    assert(_firstfree != NULL);
    nt->_firstfree = basedst + (_firstfree - basesrc);
    nt->_usednodes = _usednodes;
#else
    SQInteger ridx=0;
    SQObjectPtr key,val;
    while((ridx=Next(true,ridx,key,val))!=-1){
        nt->NewSlot(key,val VT_CODE(VT_COMMA &_nodes[ridx].varTrace));
    }
#endif
    nt->_classTypeId = _classTypeId;
    nt->SetDelegate(_delegate);
    return nt;
}

SQTable::_HashNode *SQTable::_Get(const SQObjectPtr &key) const
{
    if(sq_type(key) == OT_NULL)
        return nullptr;

    if (sq_type(key) == OT_STRING)
        return _GetStr(_rawval(key), _string(key)->_hash & _numofnodes_minus_one);
    else
        return _Get(key, HashObj(key) & _numofnodes_minus_one);
}

bool SQTable::Get(const SQObjectPtr &key,SQObjectPtr &val) const
{
    const _HashNode *n = _Get(key);
    if (n) {
        val = _realval(n->val);
        return true;
    }
    return false;
}

bool SQTable::GetStrToInt(const SQObjectPtr &key,uint32_t &val) const//for class members
{
    assert(sq_type(key) == OT_STRING);
    const _HashNode *n = _GetStr(_rawval(key), _string(key)->_hash & _numofnodes_minus_one);
    if (!n)
      return false;
    assert(sq_type(n->val) == OT_INTEGER);
    val = _integer(n->val);
    return true;
}

#if SQ_VAR_TRACE_ENABLED == 1
VarTrace * SQTable::GetVarTracePtr(const SQObjectPtr &key)
{
  _HashNode *n = _Get(key, HashObj(key) & _numofnodes_minus_one);
  if (n)
    return &(n->varTrace);
  else
    return NULL;
}
#endif


bool SQTable::NewSlot(const SQObjectPtr &__restrict key,const SQObjectPtr &__restrict val  VT_DECL_ARG)
{
    assert(sq_type(key) != OT_NULL);
    SQHash h = HashObj(key) & _numofnodes_minus_one;
    _HashNode *n = _Get(key, h);
    if (n) {
        n->val = val;
        VT_CODE(if (var_trace_arg) n->varTrace = *var_trace_arg);
        VT_TRACE_SINGLE(n, val, _ss(this)->_root_vm);
        return false;
    }
    _HashNode *mp = &_nodes[h];
    n = mp;

    //key not found I'll insert it
    //main pos is not free

    if(sq_type(mp->key) != OT_NULL) {
        n = _firstfree;  /* get a free place */
        SQHash mph = HashObj(mp->key) & _numofnodes_minus_one;
        _HashNode *othern;  /* main position of colliding node */

        if (mp > n && (othern = &_nodes[mph]) != mp){
            /* yes; move colliding node into free position */
            while (othern->next != mp){
                assert(othern->next != NULL);
                othern = othern->next;  /* find previous */
            }
            othern->next = n;  /* redo the chain with `n' in place of `mp' */
            n->key = mp->key;
            n->val = mp->val;/* copy colliding node into free pos. (mp->next also goes) */
            n->next = mp->next;
            VT_COPY_SINGLE(mp, n);
            mp->key.Null();
            mp->val.Null();
            VT_CLEAR_SINGLE(mp);
            mp->next = NULL;  /* now `mp' is free */
        }
        else{
            /* new node will go into free position */
            n->next = mp->next;  /* chain new position */
            mp->next = n;
            mp = n;
        }
    }
    mp->key = key;

    for (;;) {  /* correct `firstfree' */
        if (sq_type(_firstfree->key) == OT_NULL && _firstfree->next == NULL) {
            mp->val = val;
            VT_CODE(if (var_trace_arg) mp->varTrace = *var_trace_arg);
            VT_TRACE_SINGLE(mp, val, _ss(this)->_root_vm);

            // update class type id
            if (sq_isstring(key) && (_numofnodes_minus_one < (1<<TBL_CLASS_TYPE_MEMBER_BITS))) {
                if (_classTypeId != 0 || _usednodes==0) {
                    uint8_t insertPos = mp - _nodes;
                    _classTypeId = class_type_hash_update_1(_classTypeId ? _classTypeId : CLASS_TYPE_HASH_INIT, insertPos);

                    // Use SQString address as identifier, strings are unique, immutable and strored in a table.
                    // That means that all script strings share the same reference to SQString.
                    // 32 provides more than realistic address range.
                    uint32_t strIdBits = uint32_t( (uintptr_t(_string(key)) >> 3) & 0xFFFFFFFF );
                    _classTypeId = class_type_hash_update_4(_classTypeId, strIdBits);
                }
            } else {
                _classTypeId = 0;
            }

            ++_usednodes;

            return true;  /* OK; table still has a free place */
        }
        else if (_firstfree == _nodes) break;  /* cannot decrement from here */
        else (_firstfree)--;
    }
    Rehash(true);
    return NewSlot(key, val  VT_CODE(VT_COMMA var_trace_arg));
}

SQInteger SQTable::Next(bool getweakrefs,const SQObjectPtr &__restrict refpos, SQObjectPtr &__restrict outkey, SQObjectPtr &__restrict outval)
{
    uint32_t idx = (uint32_t)TranslateIndex(refpos);
    while (idx <= _numofnodes_minus_one) {
        if(sq_type(_nodes[idx].key) != OT_NULL) {
            //first found
            _HashNode &n = _nodes[idx];
            outkey = n.key;
            outval = getweakrefs?(SQObject)n.val:_realval(n.val);
            //return idx for the next iteration
            return ++idx;
        }
        ++idx;
    }
    //nothing to iterate anymore
    return -1;
}


bool SQTable::Set(const SQObjectPtr &key, const SQObjectPtr &val)
{
    _HashNode *n = _Get(key);
    if (n) {
        n->val = val;
        VT_TRACE_SINGLE(n, val, _ss(this)->_root_vm);
        return true;
    }
    return false;
}

void SQTable::_ClearNodes()
{
    for (_HashNode *__restrict i = _nodes, *e = i + _numofnodes_minus_one; i <= e; i++) {
      i->key.Null();
      i->val.Null();
      i->next = NULL;
      VT_CLEAR_SINGLE(i);
    }
}

void SQTable::Finalize()
{
    _ClearNodes();
    SetDelegate(NULL);
}

void SQTable::Clear(SQBool rehash)
{
    _ClearNodes();
    _usednodes = 0;
    _classTypeId = 0;
    if (rehash)
      Rehash(true);
    else
      _firstfree=&_nodes[_numofnodes_minus_one];
}

bool SQTable::IsBinaryEqual(SQTable *o)
{
    if (o->_usednodes != _usednodes || o->_classTypeId != _classTypeId)
        return false;
    if (o == this)
        return true;
    if (o->_usednodes == 0)
        return true;

    return memcmp(_nodes, o->_nodes, sizeof(_HashNode) * (_numofnodes_minus_one + 1)) == 0;
}
