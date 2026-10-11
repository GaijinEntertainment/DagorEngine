#ifndef DAS_AST_SIM_CONVERTERS_H
#define DAS_AST_SIM_CONVERTERS_H

#include <daScript/misc/platform.h>
#include <daScript/misc/anyhash.h>
#include <daScript/simulate/simulate.h>
#include <daScript/simulate/aot_library.h>
#include <daScript/simulate/code_of_policies.h>

namespace das {
    using MangledNameHash = uint64_t;

    inline MangledNameHash mangledNameHashOf ( const string & mangledName ) {
        return hash_blockz64((uint8_t *)mangledName.c_str());
    }

    struct FunctionInfo {
        FunctionInfo() = delete;
        FunctionInfo(string name, string mangledName, uint64_t mnh, uint64_t aotHash, uint32_t stackSize,
                     bool unsafeOperation, bool fastCall, bool builtin,
                     bool promoted, bool isResRef, bool pinvoke)
            : name(das::move(name))
            , mnh(mnh)
            , aotHash(aotHash)
            , mangledName(das::move(mangledName))
            , stackSize(stackSize)
            , unsafeOperation(unsafeOperation)
            , fastCall(fastCall)
            , builtin(builtin)
            , promoted(promoted)
            , res_ref(isResRef)
            , pinvoke(pinvoke) {}
        string name;
        uint64_t mnh;
        uint64_t aotHash;
        string mangledName;
        uint32_t stackSize;
        bool unsafeOperation;
        bool fastCall;
        bool builtin;
        bool promoted;
        bool res_ref;
        bool pinvoke;
    };

    struct GlobalVarInfo {
        GlobalVarInfo() = delete;
        GlobalVarInfo(string name, const string &mangledName, uint32_t typeSize, bool globalShared)
            : name(das::move(name))
            , mangledNameHash(mangledNameHashOf(mangledName))
            , typeSize(typeSize)
            , globalShared(globalShared) {}

        string name;
        uint64_t mangledNameHash;
        uint32_t typeSize;
        bool globalShared;
    };
    struct SizeDiff {
        uint64_t sharedSizeDiff;
        uint64_t globalsSizeDiff;
    };

    /**
     * Methods to init aot variables
     */
    DAS_API MangledNameHash InitAotFunction(const Context &ctx, SimFunction* gfun, const FunctionInfo &info);
    DAS_API SizeDiff InitGlobalVariable(const Context &ctx, GlobalVariable* gvar, const GlobalVarInfo &info);
    DAS_API void InitGlobalVar(Context &ctx, GlobalVariable* gvar, const GlobalVarInfo &info);

    /**
     * Set code, aot, aotFunction for all function in @ref functions
     */
    DAS_API void FillFunction(Context &ctx, const AotLibrary &aotLib, vector<pair<uint64_t, SimFunction*>> &functions);
}

#endif
