#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/src/builtin/module_builtin_rtti.h"
#include "daScript/ast/ast_handle.h"

#include "dasQUIRREL.h"

#include "cb_dasQuirrel.h"

namespace das {

struct SqFuncDesc {
    uint64_t fnHash;
    string paramsCheck;
    int paramsNum;
};

static mutex lock;
static das_hash_map<uint64_t, Context*> bindedFunctions;
static das_hash_map<uint64_t, string> bindedFunctionNames;
static das_hash_map</* module name */string, vector<SqFuncDesc>> bindedFunctionDescs;

void sqdas_bind_func( Context &ctx, uint64_t fnHash, const char * name, const char * moduleName, int paramsNum, const char * paramsCheck) {
    lock_guard<mutex> guard(lock);
    bindedFunctions[fnHash] = &ctx;
    bindedFunctionNames[fnHash] = name;
    bindedFunctionDescs[moduleName].emplace_back(SqFuncDesc{ fnHash, paramsCheck, paramsNum });
}

SQInteger call_binded_func(HSQUIRRELVM vm) {
    SQInteger funcHash;
    sq_getinteger(vm, -1, &funcHash);
    auto fnCtx = bindedFunctions.find(funcHash);
    if (fnCtx == bindedFunctions.end()) {
        return sq_throwerror(vm, string(string::CtorSprintf{}, "Internal error: unknown function with hash '%@'. Maybe function was removed", funcHash).c_str());
    }
    Context *ctx = fnCtx->second;
    if (!ctx) {
        return sq_throwerror(vm, string(string::CtorSprintf{},
                    "Unable to call function '%s', context is null. Unloaded function or compilation error",
                    bindedFunctionNames[funcHash].c_str()).c_str());
    }

    SimFunction *simFn = ctx->fnByMangledName(funcHash);
    if (!simFn) {
        return sq_throwerror(vm, string(string::CtorSprintf{},
                    "Internal error: unable to find function '%s'. Maybe compilation error",
                    bindedFunctionNames[funcHash].c_str()).c_str());
    }

    lock_guard<mutex> guard(lock);
    vec4f args[1];
    args[0] = cast<HSQUIRRELVM>::from(vm);
    vec4f res{};
    ctx->tryRestartAndLock();
    if ( !ctx->ownStack ) {
        StackAllocator sharedStack(8*1024);
        SharedStackGuard guard(*ctx, sharedStack);
        res = ctx->evalWithCatch(simFn, args);
    } else {
        res = ctx->evalWithCatch(simFn, args);
    }
    if (auto exp = ctx->getException()) {
        ctx->stackWalk(&ctx->exceptionAt, true, true);
        ctx->unlock();
        return sq_throwerror(vm, exp);
    }
    ctx->unlock();
    return cast<SQInteger>::to(res);
}

void register_bound_funcs(HSQUIRRELVM vm, function<void(const char *module_name, HSQOBJECT tab)> cb) {
    lock_guard<mutex> guard(lock);
    for (auto &modules : bindedFunctionDescs)
    {
        HSQOBJECT obj;
        sq_newtable(vm);
        sq_getstackobj(vm,-1,&obj);
        for (auto &pair : modules.second) {
            string funcName = bindedFunctionNames[pair.fnHash];
            sq_pushstring(vm, funcName.c_str(), -1);
            sq_pushinteger(vm, pair.fnHash); // push func hash
            sq_newclosure(vm, call_binded_func, 1);
            sq_setparamscheck(vm, pair.paramsNum, pair.paramsCheck.c_str());
            sq_setnativeclosurename(vm, -1, funcName.c_str());
            sq_newslot(vm, -3, SQFalse);
        }
        cb(modules.first.c_str(), obj);
        sq_pop(vm, 1);
    }
}

void Module_dasQUIRREL::initBind() {

    addExtern<DAS_BIND_FUN(sqdas_bind_func)>(*this,lib,"sqdas_bind_func",
        SideEffects::modifyExternal, "sqdas_bind_func");

    das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) {
        lock_guard<mutex> guard(lock);
        for (auto &it : bindedFunctions) {
          if (it.second == ctx) {
            it.second = nullptr;
            LOG tout(LogLevel::info);
            tout << "unlink quirrel binding: " << bindedFunctionNames[it.first] << "\n";
          }
        }
    });
}

}


