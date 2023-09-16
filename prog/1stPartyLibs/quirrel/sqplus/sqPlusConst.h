// SqPlusConst.h
// SqPlus constant type and constant member function support created by Simon Michelmore.
// Modular integration 11/14/05 jcs.

#ifdef SQPLUS_DECLARE_INSTANCE_TYPE_CONST
#undef SQPLUS_DECLARE_INSTANCE_TYPE_CONST

#define SQPLUS_DECLARE_TYPE_OPERATIONS_CONST(TYPE, NAME) \
  inline bool Match(TypeWrapper<const TYPE &>,HSQUIRRELVM v,int idx) { \
    return  GetInstance<TYPE,false>(v,idx) != NULL; } \
  inline const TYPE & Get(TypeWrapper<const TYPE &>,HSQUIRRELVM v,int idx) { \
    return *GetInstance<TYPE,true>(v,idx); } \
  inline void PushNative(HSQUIRRELVM v,const TYPE * value) { \
    if (!value)  sq_pushnull(v); \
    else if (!CreateNativeClassInstance(v,GetTypeName(*value),(TYPE*)value,0)) \
      DAGOR_THROW ( SquirrelError(_SC("Push(): could not create INSTANCE of " #NAME))); } \
  inline void PushNative(HSQUIRRELVM v,const TYPE & value) { \
    if (!CreateCopyInstance(v, value)) \
      DAGOR_THROW ( SquirrelError(_SC("Push(): could not create INSTANCE copy of " #NAME))); }

#define SQPLUS_DECLARE_TYPE_PUSH_NATIVE_CONST(TYPE, NAME) \
  template<> struct functor<const TYPE*> { static inline void push(HSQUIRRELVM v,const TYPE * value) { PushNative(v, value); } }; \
  template<> struct functor<const TYPE&> { static inline void push(HSQUIRRELVM v,const TYPE & value) { PushNative(v, value); } };

#define DECLARE_INSTANCE_TYPE_NAME_CONST(TYPE,NAME) \
DECLARE_INSTANCE_TYPE_NAME_(TYPE,NAME) \
namespace SqPlus { \
  SQPLUS_DECLARE_TYPE_OPERATIONS_CONST(TYPE, NAME) \
  SQPLUS_DECLARE_TYPE_PUSH_NATIVE_CONST(TYPE, NAME) \
} // nameSpace SqPlus

#define DECLARE_INSTANCE_TYPE(TYPE) DECLARE_INSTANCE_TYPE_NAME_CONST(TYPE,TYPE)
#define DECLARE_INSTANCE_TYPE_NAME(TYPE,NAME) DECLARE_INSTANCE_TYPE_NAME_CONST(TYPE,NAME)

#endif

#ifdef SQPLUS_CALL_CONST_MFUNC_RET0
#undef SQPLUS_CALL_CONST_MFUNC_RET0
template <typename Callee>
static int Call(Callee & callee,RT (Callee::*func)() const,HSQUIRRELVM v,int /*index*/) {
  RT ret = (callee.*func)();
  Push(v,ret);
  return 1;
}

template <typename Callee,typename P1>
static int Call(Callee & callee,RT (Callee::*func)(P1) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2,typename P3>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  sq_argassert(6,index + 5);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4),
    Get(TypeWrapper<P6>(),v,index + 5)
    );
  Push(v,ret);
  return 1;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  sq_argassert(6,index + 5);
  sq_argassert(7,index + 6);
  RT ret = (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4),
    Get(TypeWrapper<P6>(),v,index + 5),
    Get(TypeWrapper<P7>(),v,index + 6)
    );
  Push(v,ret);
  return 1;
}
#endif

#ifdef SQPLUS_CALL_CONST_MFUNC_NORET
#undef SQPLUS_CALL_CONST_MFUNC_NORET
template<typename Callee>
static int Call(Callee & callee,void (Callee::*func)() const,HSQUIRRELVM,int /*index*/) {
  (callee.*func)();
  return 0;
}

template<typename Callee,typename P1>
static int Call(Callee & callee,void (Callee::*func)(P1) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2>
static int Call(Callee & callee,void (Callee::*func)(P1,P2) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2,typename P3>
static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4>
static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5>
static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5,P6) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  sq_argassert(6,index + 5);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4),
    Get(TypeWrapper<P6>(),v,index + 5)
    );
  return 0;
}

template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5,P6,P7) const,HSQUIRRELVM v,int index) {
  sq_argassert(1,index + 0);
  sq_argassert(2,index + 1);
  sq_argassert(3,index + 2);
  sq_argassert(4,index + 3);
  sq_argassert(5,index + 4);
  sq_argassert(6,index + 5);
  sq_argassert(7,index + 6);
  (callee.*func)(
    Get(TypeWrapper<P1>(),v,index + 0),
    Get(TypeWrapper<P2>(),v,index + 1),
    Get(TypeWrapper<P3>(),v,index + 2),
    Get(TypeWrapper<P4>(),v,index + 3),
    Get(TypeWrapper<P5>(),v,index + 4),
    Get(TypeWrapper<P6>(),v,index + 5),
    Get(TypeWrapper<P7>(),v,index + 6)
    );
  return 0;
}
#endif

#ifdef SQPLUS_CALL_CONST_MFUNC_RET1
#undef SQPLUS_CALL_CONST_MFUNC_RET1
template<typename Callee,typename RT>
int Call(Callee & callee, RT (Callee::*func)() const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1>
int Call(Callee & callee,RT (Callee::*func)(P1) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2>
int Call(Callee & callee,RT (Callee::*func)(P1,P2) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7) const,HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}
#undef SQPLUS_CALL_CONST_MFUNC_RET1
#endif


// SqPlusConst.h
