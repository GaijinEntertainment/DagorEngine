/*  see copyright notice in squirrel.h */
#ifndef _SQARRAY_H_
#define _SQARRAY_H_

#include "vartrace.h"

struct SQArray : public CHAINABLE_OBJ
{
private:
    SQArray(SQSharedState *ss,SQInteger nsize) :
      _values(ss->_alloc_ctx)
      VT_DECL_CTR(ss->_alloc_ctx)
    {
        _values.resize(nsize); VT_RESIZE(nsize); INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
    }
    ~SQArray()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
public:
    static SQArray* Create(SQSharedState *ss,SQInteger nInitialSize){
        SQArray *newarray=(SQArray*)SQ_MALLOC(ss->_alloc_ctx, sizeof(SQArray));
        new (newarray) SQArray(ss,nInitialSize);
        return newarray;
    }
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    SQObjectType GetType() {return OT_ARRAY;}
#endif
    void Finalize(){
        _values.resize(0);
        VT_RESIZE(0);
    }

    VT_CODE(VarTrace * GetVarTracePtr(const SQInteger nidx) { return &varTrace[nidx]; })

    bool Get(const SQInteger nidx,SQObjectPtr &val)
    {
        if((SQUnsignedInteger)nidx<(SQUnsignedInteger)_values.size()){
            SQObjectPtr &o = _values[nidx];
            val = _realval(o);
            return true;
        }
        else return false;
    }
    bool Set(const SQInteger nidx,const SQObjectPtr &val)
    {
        if((SQUnsignedInteger)nidx<(SQUnsignedInteger)_values.size()){
            _values[nidx]=val;
            VT_TRACE(nidx, val, _ss(this)->_root_vm);
            return true;
        }
        else return false;
    }
    SQInteger Next(const SQObjectPtr &refpos,SQObjectPtr &outkey,SQObjectPtr &outval)
    {
        SQUnsignedInteger idx=TranslateIndex(refpos);
        if (idx<_values.size()) {
            //first found
            outkey=(SQInteger)idx;
            SQObjectPtr &o = _values[idx];
            outval = _realval(o);
            //return idx for the next iteration
            return ++idx;
        }
        //nothing to iterate anymore
        return -1;
    }
    SQArray *Clone(){SQArray *anew=Create(_opt_ss(this),0); anew->_values.copy(_values); VT_CLONE_TO(anew); return anew; }
    SQInteger Size() const {return _values.size();}
    void Resize(SQInteger size)
    {
        SQObjectPtr _null;
        Resize(size,_null);
    }
    void Resize(SQInteger size,SQObjectPtr &fill) { _values.resize(size,fill); VT_RESIZE(size); ShrinkIfNeeded(); }
    void Reserve(SQInteger size) { _values.reserve(size); VT_RESERVE(size); }
    void Append(const SQObject &o){_values.push_back(o); VT_PUSHBACK(o, _ss(this)->_root_vm); }
    void Extend(const SQArray *a);
    SQObjectPtr &Top(){return _values.top();}
    void Pop(){_values.pop_back(); VT_POPBACK(); ShrinkIfNeeded(); }
    bool Insert(SQInteger idx,const SQObject &val){
        if(idx < 0 || idx > (SQInteger)_values.size())
            return false;
        _values.insert(idx,val);
        VT_INSERT(idx, val, _ss(this)->_root_vm);
        return true;
    }
    void ShrinkIfNeeded() {
        if(_values.size() <= _values.capacity()>>2) //shrink the array
            _values.shrinktofit();
    }
    bool Remove(SQInteger idx, bool shrink=true){
        if(idx < 0 || idx >= (SQInteger)_values.size())
            return false;
        _values.remove(idx);
        VT_REMOVE(idx);
        if (shrink)
            ShrinkIfNeeded();
        return true;
    }
    void Release()
    {
        SQAllocContext ctx = _values._alloc_ctx;
        sq_delete(ctx, this, SQArray);
    }

    SQObjectPtrVec _values;
    VT_DECL_VEC;
};
#endif //_SQARRAY_H_
