#pragma once
#include <stdint.h>

namespace das
{
    extern DAS_THREAD_LOCAL unsigned ModuleKarma;
    class Module;
};

#define NEED_MODULE(ClassName) \
    extern das::Module * register_##ClassName (); \
    das::ModuleKarma += unsigned(intptr_t(register_##ClassName()));

#define NEED_ALL_DEFAULT_MODULES \
    NEED_MODULE(Module_BuiltIn); \
    NEED_MODULE(Module_Math); \
    NEED_MODULE(Module_Raster); \
    NEED_MODULE(Module_Strings); \
    NEED_MODULE(Module_Rtti); \
    NEED_MODULE(Module_Ast); \
    NEED_MODULE(Module_Debugger); \
    NEED_MODULE(Module_Jit); \
    NEED_MODULE(Module_FIO); \
    NEED_MODULE(Module_DASBIND); \
    NEED_MODULE(Module_Network);

