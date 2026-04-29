#pragma once

#include "daScript/ast/ast.h"

namespace das {
    class Module_BuiltIn : public Module {
    public:
        Module_BuiltIn();
        virtual ~Module_BuiltIn();
    protected:
        void addRuntime(ModuleLibrary & lib);
        void addRuntimeSort(ModuleLibrary & lib);
        void addVectorTypes(ModuleLibrary & lib);
        void addVectorCtor(ModuleLibrary & lib);
        void addArrayTypes(ModuleLibrary & lib);
        void addTime(ModuleLibrary & lib);
        void addMiscTypes(ModuleLibrary & lib);
        virtual ModuleAotType aotRequire ( TextWriter &tw ) const override {
            tw << "#include \"daScript/simulate/bin_serializer.h\"\n";
            tw << "#include \"daScript/simulate/runtime_profile.h\"\n";
            tw << "#include \"daScript/misc/performance_time.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}
