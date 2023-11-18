#pragma once

#define CLASS_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                \
  {                                                                                                                  \
    using memberMethod = DAS_CALL_MEMBER(FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(FUNC_NAME)); \
  }
#define CLASS_MEMBER_SIGNATURE(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)          \
  {                                                                                 \
    using memberMethod = das::das_call_member<SIGNATURE, &FUNC_NAME>;               \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, \
      "das_call_member<" #SIGNATURE ", &" #FUNC_NAME ">::invoke");                  \
  }
#define BIND_FUNCTION(function, sinonim, side_effect) \
  das::addExtern<DAS_BIND_FUN(function)>(*this, lib, sinonim, side_effect, #function);

#define BIND_FUNCTION_SIGNATURE(function, sinonim, side_effect, signature) \
  das::addExtern<signature, &function>(*this, lib, sinonim, side_effect, #function);
