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
        bool appendCompiledFunctions();
        virtual ModuleAotType aotRequire ( TextWriter & ) const override { return ModuleAotType::cpp; }
    };
}
