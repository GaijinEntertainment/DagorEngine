#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

using namespace das;

// C++ type exposed to daScript
struct Counter {
    int32_t value = 0;
    int32_t step  = 1;
};

void counter_increment(Counter & c)     { c.value += c.step; }
void counter_decrement(Counter & c)     { c.value -= c.step; }
void counter_reset(Counter & c)         { c.value = 0; }
int32_t counter_get(const Counter & c)  { return c.value; }

Counter make_counter(int32_t step) {
    Counter c;
    c.step = step;
    return c;
}

// Type factory and annotation
MAKE_TYPE_FACTORY(Counter, Counter);

namespace das {

struct CounterAnnotation : ManagedStructureAnnotation<Counter, false> {
    CounterAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Counter", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(value)>("value", "value");
        addField<DAS_BIND_MANAGED_FIELD(step)>("step", "step");
    }
};

} // namespace das

// Module registration
using method_increment = DAS_CALL_MEMBER(Counter::value);  // dummy for DAS_CALL_MEMBER pattern

class Module_Counter : public Module {
public:
    Module_Counter() : Module("counter") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addAnnotation(make_smart<das::CounterAnnotation>(lib));
        addExtern<DAS_BIND_FUN(make_counter), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_counter",
            SideEffects::none, "make_counter")->args({"step"});
        addExtern<DAS_BIND_FUN(counter_increment)>(*this, lib, "increment",
            SideEffects::modifyArgument, "counter_increment")->args({"self"});
        addExtern<DAS_BIND_FUN(counter_decrement)>(*this, lib, "decrement",
            SideEffects::modifyArgument, "counter_decrement")->args({"self"});
        addExtern<DAS_BIND_FUN(counter_reset)>(*this, lib, "reset",
            SideEffects::modifyArgument, "counter_reset")->args({"self"});
        addExtern<DAS_BIND_FUN(counter_get)>(*this, lib, "get",
            SideEffects::none, "counter_get")->args({"self"});
    }
};

REGISTER_MODULE(Module_Counter);
REGISTER_DYN_MODULE(Module_Counter, Module_Counter);
