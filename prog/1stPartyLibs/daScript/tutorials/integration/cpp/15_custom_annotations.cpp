// Tutorial 15 — Custom Annotations (C++ integration)
//
// Demonstrates how to create custom annotations in C++ that modify
// daslang compilation behavior:
//   - FunctionAnnotation — hooks into function compilation
//   - StructureAnnotation — hooks into struct compilation
//   - MarkFunctionAnnotation — convenience base for function annotations
//   - Adding fields to structs at compile time via touch()
//
// Annotations are the foundation of daslang's metaprogramming from
// the C++ side.  They let the host application control, validate, or
// transform script code during compilation.

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"

using namespace das;

// -----------------------------------------------------------------------
// [log_calls] — a FunctionAnnotation that prints at compile time
// -----------------------------------------------------------------------
//
// When a function is annotated with [log_calls], the compiler calls
// apply() during parsing and finalize() after type inference.
// We print a message at each stage to show the annotation lifecycle.

struct LogCallsAnnotation : FunctionAnnotation {
    LogCallsAnnotation() : FunctionAnnotation("log_calls") {}

    // Called during parsing — can modify function flags
    virtual bool apply(const FunctionPtr & func, ModuleGroup &,
                       const AnnotationArgumentList &,
                       string &) override {
        printf("  [log_calls] apply:    %s\n", func->name.c_str());
        return true;
    }

    // Not supported on blocks
    virtual bool apply(ExprBlock *, ModuleGroup &,
                       const AnnotationArgumentList &,
                       string & err) override {
        err = "[log_calls] is not supported for blocks";
        return false;
    }

    // Called after type inference — can validate the function
    virtual bool finalize(const FunctionPtr & func, ModuleGroup &,
                          const AnnotationArgumentList &,
                          const AnnotationArgumentList &,
                          string &) override {
        printf("  [log_calls] finalize: %s (args: %d)\n",
               func->name.c_str(), (int)func->arguments.size());
        return true;
    }

    // Not supported on blocks
    virtual bool finalize(ExprBlock *, ModuleGroup &,
                          const AnnotationArgumentList &,
                          const AnnotationArgumentList &,
                          string &) override {
        return true;
    }
};

// -----------------------------------------------------------------------
// [add_field] — a StructureAnnotation that adds a field at compile time
// -----------------------------------------------------------------------
//
// When a struct is annotated with [add_field], the compiler calls
// touch() BEFORE type inference.  We add an "id" field of type int.
// After inference, look() is called for read-only validation.

struct AddFieldAnnotation : StructureAnnotation {
    AddFieldAnnotation() : StructureAnnotation("add_field") {}

    // Called BEFORE type inference — can modify the struct
    virtual bool touch(const StructurePtr & st, ModuleGroup &,
                       const AnnotationArgumentList &,
                       string &) override {
        // Guard: don't add if already present (touch can be called
        // multiple times during compilation)
        if (!st->findField("id")) {
            st->fields.emplace_back(
                "id",                               // field name
                make_smart<TypeDecl>(Type::tInt),    // type: int
                nullptr,                            // no initializer
                AnnotationArgumentList(),           // no field annotations
                false,                              // no move semantics
                LineInfo()                          // source location
            );
            // IMPORTANT: clear the field lookup cache after modifying
            // the fields vector, otherwise findField uses stale data
            st->fieldLookup.clear();
            printf("  [add_field] added 'id : int' to struct '%s'\n",
                   st->name.c_str());
        }
        return true;
    }

    // Called AFTER type inference — read-only validation
    virtual bool look(const StructurePtr & st, ModuleGroup &,
                      const AnnotationArgumentList &,
                      string &) override {
        printf("  [add_field] look: struct '%s' has %d fields\n",
               st->name.c_str(), (int)st->fields.size());
        return true;
    }
};

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial15 : public Module {
public:
    Module_Tutorial15() : Module("tutorial_15_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // Register both annotations — scripts can now use
        // [log_calls] and [add_field]
        addAnnotation(make_smart<LogCallsAnnotation>());
        addAnnotation(make_smart<AddFieldAnnotation>());
    }
};

REGISTER_MODULE(Module_Tutorial15);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/15_custom_annotations.das"

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    tout << "--- Compilation (annotation hooks fire here) ---\n";
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
        tout << "Simulation failed\n";
        return;
    }

    tout << "\n--- Running script ---\n";
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
    NEED_MODULE(Module_Tutorial15);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
