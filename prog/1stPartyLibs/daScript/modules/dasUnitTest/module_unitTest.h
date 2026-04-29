#include "daScript/daScript.h"

using namespace das;

class Module_UnitTest : public Module {
public:
    Module_UnitTest();
    void addEnumTest(ModuleLibrary &);
    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
    bool appendCompiledFunctions();
};
