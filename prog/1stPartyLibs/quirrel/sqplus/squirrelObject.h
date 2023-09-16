#ifndef _SQUIRREL_OBJECT_H_
#define _SQUIRREL_OBJECT_H_

class SquirrelVM;
namespace SqPlus {}

class SquirrelObject
{
  friend class SquirrelVM;
public:
  SquirrelObject();
  virtual ~SquirrelObject();
  SquirrelObject(const SquirrelObject &o);
  SquirrelObject(HSQOBJECT o);
  SquirrelObject & operator =(const SquirrelObject &o);
  SquirrelObject & operator =(int n);
  void AttachToStackObject(int idx);
  void Reset(void); // Release (any) reference and reset _o.
  SquirrelObject Clone();
  SQBool SetValue(const SquirrelObject &key,const SquirrelObject &val);

  SQBool SetValue(SQInteger key,const SquirrelObject &val);
  SQBool SetValue(SQInteger key,bool b); // Compiler treats SQBool as SQInteger.
  SQBool SetValue(SQInteger key,SQInt32 n);
  SQBool SetValue(SQInteger key,SQInteger n);
  SQBool SetValue(SQInteger key,SQFloat f);
  SQBool SetValue(SQInteger key,const SQChar *s);

  SQBool SetValue(const SQChar *key,const SquirrelObject &val);
  SQBool SetValue(const SQChar *key,bool b);
  SQBool SetValue(const SQChar *key,SQInt32 n);
  SQBool SetValue(const SQChar *key,SQInteger n);
  SQBool SetValue(const SQChar *key,SQFloat f);
  SQBool SetValue(const SQChar *key,const SQChar *s);

  SQBool SetUserPointer(const SQChar * key,SQUserPointer up);
  SQUserPointer GetUserPointer(const SQChar * key);
  SQBool SetUserPointer(SQInteger key,SQUserPointer up);
  SQUserPointer GetUserPointer(SQInteger key);

  SQBool NewUserData(const SQChar * key,SQInteger size,SQUserPointer * typetag=0);
  SQBool GetUserData(const SQChar * key,SQUserPointer * data,SQUserPointer * typetag=0);
  SQBool RawGetUserData(const SQChar * key,SQUserPointer * data,SQUserPointer * typetag=0);

  SQBool NewBlob(const SQChar * key,SQInteger size);
  SQBool GetBlob(const SQChar * key,SQUserPointer * data,SQInteger* size = 0);

  SQBool NewBlob(SQInteger key,SQInteger size);
  SQBool GetBlob(SQInteger key,SQUserPointer * data,SQInteger* size = 0);

  // === BEGIN Arrays ===

  SQBool ArrayResize(SQInteger newSize);
  SQBool ArrayExtend(SQInteger amount);
  SQBool ArrayReverse(void);
  SquirrelObject ArrayPop(SQBool returnPoppedVal=SQTrue);

  void ArrayAppend(const SquirrelObject &o);
  void ArrayAppend(bool b);
  void ArrayAppend(SQInt32 n);
  void ArrayAppend(SQInteger n);
  void ArrayAppend(SQFloat f);
  void ArrayAppend(const SQChar *s);
  void ArrayAppendStr(const char *str);

  // === END Arrays ===

  SQBool SetInstanceUP(SQUserPointer up);
  SQBool IsNull() const;
  SQBool IsNumeric() const;
  int Len() const;
  const SQChar* ToString();
  bool ToBool();
  SQInteger ToInteger();
  SQFloat ToFloat();
  SQUserPointer GetInstanceUP(SQUserPointer tag) const;
  SquirrelObject GetValue(const SQChar *key) const;
  SQBool Exists(const SQChar *key) const;
  SQFloat GetFloat(const SQChar *key, SQFloat def=0.0f) const;
  SQInteger GetInt(const SQChar *key, SQInteger def=0) const;
  const SQChar *GetString(const SQChar *key, const SQChar *def=NULL) const;
  bool GetBool(const SQChar *key, bool def=SQFalse) const;
  SquirrelObject GetValue(SQInteger key) const;
  SQFloat GetFloat(SQInteger key) const;
  SQInteger GetInt(SQInteger key) const;
  const SQChar *GetString(SQInteger key) const;
  bool GetBool(SQInteger key) const;
  SQObjectType GetType();
  HSQOBJECT & GetObjectHandle(){return _o;}
  const HSQOBJECT & GetObjectHandle() const {return _o;}
  SQBool BeginIteration();
  SQBool Next(SquirrelObject &key,SquirrelObject &value);
  void EndIteration();

  SQBool GetTypeTag(SQUserPointer * typeTag);

  // === Get the type name of item/object through string key in a table or class. Returns NULL if the type name is not set (not an SqPlus registered type).
  const SQChar * GetTypeName(const SQChar * key);

  // === BEGIN code suggestion from the Wiki ===
  template<typename _ty> inline _ty Get(void);
  // === END code suggestion from the Wiki ===

private:
  SQBool GetSlot(const SQChar *name) const;
  SQBool RawGetSlot(const SQChar *name) const;
  SQBool GetSlot(SQInteger key) const;
  HSQOBJECT _o;
};

struct StackHandler {
  StackHandler(HSQUIRRELVM vm) {
    _top = sq_gettop(vm);
    v = vm;
  }
  SQFloat GetFloat(int idx) {
    SQFloat x = 0.0f;
    if(idx > 0 && idx <= _top) {
      sq_getfloat(v,idx,&x);
    }
    return x;
  }
  SQInteger GetInt(int idx) {
    SQInteger x = 0;
    if(idx > 0 && idx <= _top) {
      sq_getinteger(v,idx,&x);
    }
    return x;
  }
  HSQOBJECT GetObjectHandle(int idx) {
    HSQOBJECT x;
    sq_resetobject(&x);
    if(idx > 0 && idx <= _top) {
      sq_getstackobj(v,idx,&x);
    }
    return x;
  }
  const SQChar *GetString(int idx)
  {
    const SQChar *x = NULL;
    if(idx > 0 && idx <= _top) {
      sq_getstring(v,idx,&x);
    }
    return x;
  }
  SQUserPointer GetUserPointer(int idx)
  {
    SQUserPointer x = 0;
    if(idx > 0 && idx <= _top) {
      sq_getuserpointer(v,idx,&x);
    }
    return x;
  }
  SQUserPointer GetInstanceUp(int idx,SQUserPointer tag)
  {
    SQUserPointer self;
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)&self,tag)))
      return NULL;
    return self;
  }
  SQUserPointer GetUserData(int idx,SQUserPointer tag=0)
  {
    SQUserPointer otag;
    SQUserPointer up;
    if(idx > 0 && idx <= _top) {
      if(SQ_SUCCEEDED(sq_getuserdata(v,idx,&up,&otag))) {
        if(tag == otag)
          return up;
      }
    }
    return NULL;
  }
  SQBool GetBool(int idx)
  {
    SQBool ret;
    if(idx > 0 && idx <= _top) {
      if(SQ_SUCCEEDED(sq_getbool(v,idx,&ret)))
        return ret;
    }
    return SQFalse;
  }
  int GetType(int idx)
  {
    if(idx > 0 && idx <= _top) {
      return sq_gettype(v,idx);
    }
    return -1;
  }

  int GetParamCount() {
    return _top;
  }
  int Return(const SQChar *s)
  {
    sq_pushstring(v,s,-1);
    return 1;
  }
  int Return(SQFloat f)
  {
    sq_pushfloat(v,f);
    return 1;
  }
  int Return(SQInteger i)
  {
    sq_pushinteger(v,i);
    return 1;
  }
  int Return(bool b)
  {
    sq_pushbool(v,b);
    return 1;
  }
  int Return(SQUserPointer p) {
    sq_pushuserpointer(v,p);
    return 1;
  }
  int Return(SquirrelObject &o)
  {
    sq_pushobject(v,o.GetObjectHandle());
    return 1;
  }
  int Return() { return 0; }
  int ThrowError(const SQChar *error) {
    return sq_throwerror(v,error);
  }
  HSQUIRRELVM GetVMPtr() { return v; }
private:
  int _top;
  HSQUIRRELVM v;
};

#endif //_SQUIRREL_OBJECT_H_
