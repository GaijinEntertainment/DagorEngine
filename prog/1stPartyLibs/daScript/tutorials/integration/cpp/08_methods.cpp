// Tutorial 08 — Binding Methods (C++ integration)
//
// Demonstrates how to expose C++ class/struct methods to daslang:
//   - DAS_CALL_MEMBER / DAS_CALL_MEMBER_CPP — wrapping member functions
//   - addExtern with method wrappers — registering methods as free functions
//   - Const and non-const method overloads
//   - addCtorAndUsing — binding constructors
//
// In daslang, "methods" are free functions whose first argument is the
// struct instance.  DAS_CALL_MEMBER generates a static wrapper that
// forwards the call to the C++ member function.

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>
#include <cstring>
#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// C++ class with methods to expose
// -----------------------------------------------------------------------

struct Counter {
    int32_t value;
    int32_t step;

    void increment() { value += step; }
    void decrement() { value -= step; }
    void add(int32_t amount) { value += amount; }
    int32_t get() const { return value; }
    void reset() { value = 0; }
    void setStep(int32_t s) { step = s; }
    bool isPositive() const { return value > 0; }
    bool isZero() const { return value == 0; }
};

Counter make_counter(int32_t initial, int32_t step) {
    Counter c;
    c.value = initial;
    c.step = step;
    return c;
}

// A class with const and non-const method overloads
struct StringBuffer {
    char data[256];
    int32_t length;

    void append(const char * text) {
        int32_t tlen = (int32_t)strlen(text);
        if (length + tlen < 255) {
            memcpy(data + length, text, tlen);
            length += tlen;
            data[length] = '\0';
        }
    }

    void clear() {
        length = 0;
        data[0] = '\0';
    }

    const char * c_str() const { return data; }
    int32_t size() const { return length; }
    bool empty() const { return length == 0; }
};

StringBuffer make_string_buffer() {
    StringBuffer sb;
    sb.length = 0;
    sb.data[0] = '\0';
    return sb;
}

// -----------------------------------------------------------------------
// MAKE_TYPE_FACTORY
// -----------------------------------------------------------------------

MAKE_TYPE_FACTORY(Counter, Counter);
MAKE_TYPE_FACTORY(StringBuffer, StringBuffer);

// -----------------------------------------------------------------------
// ManagedStructureAnnotation
// -----------------------------------------------------------------------

namespace das {

struct CounterAnnotation : ManagedStructureAnnotation<Counter, false> {
    CounterAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Counter", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(value)>("value", "value");
        addField<DAS_BIND_MANAGED_FIELD(step)>("step", "step");
    }
};

struct StringBufferAnnotation : ManagedStructureAnnotation<StringBuffer, false> {
    StringBufferAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("StringBuffer", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(length)>("length", "length");
    }
};

} // namespace das

// -----------------------------------------------------------------------
// DAS_CALL_MEMBER — create method wrappers
// -----------------------------------------------------------------------
// Step 1: Create a `using` alias with DAS_CALL_MEMBER.
//   This generates a `das_call_member` wrapper struct with a static
//   `invoke(ClassName &, args...)` method that forwards to the member.
//
// Step 2: Register with addExtern<DAS_CALL_METHOD(alias)>.
//   The CPP name uses DAS_CALL_MEMBER_CPP for AOT compatibility.

// Counter methods
using method_increment = DAS_CALL_MEMBER(Counter::increment);
using method_decrement = DAS_CALL_MEMBER(Counter::decrement);
using method_add = DAS_CALL_MEMBER(Counter::add);
using method_get = DAS_CALL_MEMBER(Counter::get);
using method_reset = DAS_CALL_MEMBER(Counter::reset);
using method_setStep = DAS_CALL_MEMBER(Counter::setStep);
using method_isPositive = DAS_CALL_MEMBER(Counter::isPositive);
using method_isZero = DAS_CALL_MEMBER(Counter::isZero);

// StringBuffer methods
using method_append = DAS_CALL_MEMBER(StringBuffer::append);
using method_clear = DAS_CALL_MEMBER(StringBuffer::clear);
using method_c_str = DAS_CALL_MEMBER(StringBuffer::c_str);
using method_size = DAS_CALL_MEMBER(StringBuffer::size);
using method_empty = DAS_CALL_MEMBER(StringBuffer::empty);

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial08 : public Module {
public:
    Module_Tutorial08() : Module("tutorial_08_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Register types ---
        addAnnotation(make_smart<CounterAnnotation>(lib));
        addAnnotation(make_smart<StringBufferAnnotation>(lib));

        // --- Factory functions ---
        addExtern<DAS_BIND_FUN(make_counter), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_counter",
            SideEffects::none, "make_counter")
                ->args({"initial", "step"});

        addExtern<DAS_BIND_FUN(make_string_buffer), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_string_buffer",
            SideEffects::none, "make_string_buffer");

        // --- Counter methods ---
        // Non-const methods that modify the object: SideEffects::modifyArgument
        addExtern<DAS_CALL_METHOD(method_increment)>(*this, lib, "increment",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(Counter::increment))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_decrement)>(*this, lib, "decrement",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(Counter::decrement))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_add)>(*this, lib, "add",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(Counter::add))
                ->args({"self", "amount"});

        // Const methods: SideEffects::none
        addExtern<DAS_CALL_METHOD(method_get)>(*this, lib, "get",
            SideEffects::none, DAS_CALL_MEMBER_CPP(Counter::get))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_reset)>(*this, lib, "reset",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(Counter::reset))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_setStep)>(*this, lib, "set_step",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(Counter::setStep))
                ->args({"self", "step"});

        addExtern<DAS_CALL_METHOD(method_isPositive)>(*this, lib, "is_positive",
            SideEffects::none, DAS_CALL_MEMBER_CPP(Counter::isPositive))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_isZero)>(*this, lib, "is_zero",
            SideEffects::none, DAS_CALL_MEMBER_CPP(Counter::isZero))
                ->args({"self"});

        // --- StringBuffer methods ---
        addExtern<DAS_CALL_METHOD(method_append)>(*this, lib, "append",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(StringBuffer::append))
                ->args({"self", "text"});

        addExtern<DAS_CALL_METHOD(method_clear)>(*this, lib, "clear",
            SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(StringBuffer::clear))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_c_str)>(*this, lib, "c_str",
            SideEffects::none, DAS_CALL_MEMBER_CPP(StringBuffer::c_str))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_size)>(*this, lib, "buf_size",
            SideEffects::none, DAS_CALL_MEMBER_CPP(StringBuffer::size))
                ->args({"self"});

        addExtern<DAS_CALL_METHOD(method_empty)>(*this, lib, "buf_empty",
            SideEffects::none, DAS_CALL_MEMBER_CPP(StringBuffer::empty))
                ->args({"self"});
    }
};

REGISTER_MODULE(Module_Tutorial08);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/08_methods.das"

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                   fAccess, tout, dummyLibGroup);
    if (program->failed()) {
        tout << "Compilation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found\n";
        return;
    }

    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "Exception: " << ex << "\n";
    }
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    NEED_MODULE(Module_Tutorial08);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
