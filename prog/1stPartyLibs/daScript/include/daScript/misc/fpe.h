#pragma once

#if defined(DAS_ENABLE_FPE)

#define FPE_ENABLE_ALL  enable_all_fpe();
#define FPE_DISABLE     FpeDisable  __no_fp_##__LINE__;

#ifdef _MSC_VEC
#include <float.h>
#endif

namespace das {
    __forceinline void enable_all_fpe() {
        unsigned int dummy;
        _clearfp();
        __control87_2(_EM_INEXACT | _EM_UNDERFLOW | _EM_DENORMAL, _MCW_EM, &dummy, NULL);
    }
    class FpeDisable {
    public:
        __forceinline FpeDisable() {
            unsigned int dummy;
            __control87_2(0, 0, &flags2, NULL);
            __control87_2(_MCW_EM, _MCW_EM, &dummy, NULL);
        }
        __forceinline ~FpeDisable() {
            unsigned int dummy;
            _clearfp();
            __control87_2(flags2, _MCW_EM, &dummy, NULL);
        }
    private:
        unsigned int flags2;
    };
}

#else

#define FPE_ENABLE_ALL
#define FPE_DISABLE

#endif

