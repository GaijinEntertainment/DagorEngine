#include "daScript/daScript.h"

// we need RTTI to bind StructInfo *
#include "module_builtin_rtti.h"

using namespace das;

// base class which is part of C++ class hierarchy
class BaseClass {
public:
    virtual void update ( float dt ) = 0;
    virtual float3 getPosition() = 0;
};

// list of all objects on the C++ side
vector<shared_ptr<BaseClass>>   g_allObjects;

// updates all objects and returns average position
float3 tick ( float dt ) {
    // in case of empty list of objects return float3(0,0,0)
    if ( g_allObjects.empty() ) {
        return float3(0.0f);
    }
    float3 pos = float3(0.0f);
    for ( auto & obj : g_allObjects ) {
        obj->update(dt);                        // call virtual update
        float3 objPos = obj->getPosition();     // call virtual getPosition
        pos.x += objPos.x;
        pos.y += objPos.y;
        pos.z += objPos.z;
    }
    // average the position
    float iCount = 1.0f / float(g_allObjects.size());
    pos.x *= iCount;
    pos.y *= iCount;
    pos.z *= iCount;
    return pos;
}

// we include generated C++ bindings for the class adapter
#include "tutorial04_gen.inc"

// here we derive both from the original base class, as well as generated adapter
// functions in the generated adapter require classPtr and context to be invoked
class BaseClassAdapter : public BaseClass, public TutorialBaseClass {
public:
    // in the constructor we store pointer to the original class and context
    // we also pass StructInfo of the daScript class to the generated class
    BaseClassAdapter ( char * pClass, const StructInfo * info, Context * ctx )
        : TutorialBaseClass(info), classPtr(pClass), context(ctx) { }
    virtual void update ( float dt ) override {
        // we check if daScript class has 'update' function
        if ( auto fn = get_update(classPtr) ) {
            // we invoke it and pass it 'dt'
            invoke_update(context, fn, classPtr, dt);
        }
    }
    virtual float3 getPosition() override {
        // we check if daScript class has 'get_position'
        if ( auto fn = get_get_position(classPtr) ) {
            // we invoke it, and return it's result
            return invoke_get_position(context, fn, classPtr);
        } else {
            return float3(0.0f);
        }
    }
protected:
    void *      classPtr;   // stored pointer to the daScript class
    Context *   context;    // stored pointer to the daScript context (its optional, if we know the context by other means)
};

// we expose this function to das, so that it can 'register' derived classes with C++
void addObject ( const void * pClass, const StructInfo * info, Context * context ) {
    auto obj = make_shared<BaseClassAdapter>((char *)pClass,info,context);
    g_allObjects.emplace_back(obj);
}

// we use XDD to convert tutorial04module.das into C++ .inc file
#include "tutorial04module.das.inc"

// making custom builtin module
class Module_Tutorial04 : public Module {
public:
    Module_Tutorial04() : Module("tutorial_04") {   // module name, when used from das file
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addBuiltinDependency(lib, Module::require("rtti"));   // we add RTTI to ModuleLibrary so that we can bind addObject
        // add_object is how daScript will 'register' objects with C++
        addExtern<DAS_BIND_FUN(addObject)>(*this, lib, "add_object",
            SideEffects::modifyExternal, "addObject");
        // expose tick to daScript, for the purposes of example
        addExtern<DAS_BIND_FUN(tick)>(*this, lib, "tick",
            SideEffects::modifyExternal, "tick");
        // add builtin module (tutorial04module.das is converted to .das.inc file and included as embedded string)
        compileBuiltinModule("tutorial04module.das",tutorial04module_das,sizeof(tutorial04module_das));
    }
};

// registering module, so that its available via 'NEED_MODULE' macro
REGISTER_MODULE(Module_Tutorial04);

#define TUTORIAL_NAME   "/examples/tutorial/tutorial04.das"

#include "tutorial.inc"

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // request our custom module
    NEED_MODULE(Module_Tutorial04);
    // Initialize modules
    Module::Initialize();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}
