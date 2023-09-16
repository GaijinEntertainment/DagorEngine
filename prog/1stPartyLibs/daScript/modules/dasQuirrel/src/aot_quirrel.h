#pragma once

#include "cb_dasQuirrel.h"
#include "need_dasQUIRREL.h"

namespace das {

__forceinline SQRESULT sq_getstring(HSQUIRRELVM v,SQInteger idx,const char * const *c) {
    return sq_getstring(v,idx,(const SQChar **)c);
}

void sqdas_register(HSQUIRRELVM v);
void sqdas_bind_func( Context &ctx, uint64_t fnHash, const char * name, const char * module_name, int argsNum, const char * paramsCheck);
SQInteger call_binded_func(HSQUIRRELVM vm);
void register_bound_funcs(HSQUIRRELVM vm, function<void(const char *module_name, HSQOBJECT tab)> cb);

}