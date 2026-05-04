// Tutorial 10 — Custom Modules (C++ integration)
//
// Demonstrates how to split bindings across multiple modules:
//   - Two custom modules: "math_types" and "math_utils"
//   - Module dependencies via addBuiltinDependency
//   - Module::require to look up modules by name
//   - addConstant — exporting compile-time constants
//   - initDependencies() — deferred initialization for robust ordering
//   - NEED_MODULE ordering in the host program

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// Shared C++ types (used by both modules)
// -----------------------------------------------------------------------

struct Vec2 {
    float x, y;
};

Vec2 make_vec2(float x, float y) {
    Vec2 v; v.x = x; v.y = y;
    return v;
}

// -----------------------------------------------------------------------
// Utility functions (depend on Vec2)
// -----------------------------------------------------------------------

float vec2_length(const Vec2 & v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float vec2_dot(const Vec2 & a, const Vec2 & b) {
    return a.x * b.x + a.y * b.y;
}

Vec2 vec2_normalize(const Vec2 & v) {
    float len = vec2_length(v);
    if (len > 0.0f) return { v.x / len, v.y / len };
    return v;
}

Vec2 vec2_add(const Vec2 & a, const Vec2 & b) {
    return { a.x + b.x, a.y + b.y };
}

Vec2 vec2_scale(const Vec2 & v, float s) {
    return { v.x * s, v.y * s };
}

Vec2 vec2_lerp(const Vec2 & a, const Vec2 & b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

// -----------------------------------------------------------------------
// Type factory and annotation
// -----------------------------------------------------------------------

MAKE_TYPE_FACTORY(Vec2, Vec2);

namespace das {

struct Vec2Annotation : ManagedStructureAnnotation<Vec2, false> {
    Vec2Annotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Vec2", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
    }
};

} // namespace das

// -----------------------------------------------------------------------
// Module A: "math_types" — defines types and constants
// -----------------------------------------------------------------------

class Module_MathTypes : public Module {
public:
    Module_MathTypes() : Module("math_types") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Type ---
        addAnnotation(make_smart<Vec2Annotation>(lib));

        // --- Factory ---
        addExtern<DAS_BIND_FUN(make_vec2), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_vec2",
            SideEffects::none, "make_vec2")
                ->args({"x", "y"});

        // --- Constants ---
        // addConstant exports compile-time constants to daslang.
        // These appear as `let` constants in scripts.
        addConstant(*this, "PI",      (float)M_PI);
        addConstant(*this, "TWO_PI",  (float)(2.0 * M_PI));
        addConstant(*this, "HALF_PI", (float)(M_PI / 2.0));
        addConstant(*this, "ORIGIN",  "origin_marker");
    }
};

REGISTER_MODULE(Module_MathTypes);

// -----------------------------------------------------------------------
// Module B: "math_utils" — depends on "math_types"
//
// Uses initDependencies() instead of the constructor for registration.
// This deferred-init pattern is the production standard in daslang
// modules that depend on other custom modules:
//   1. The constructor only sets the module name — nothing else.
//   2. initDependencies() is called later, when all modules exist.
//   3. An `initialized` guard prevents double-initialization.
//   4. Module::require() retrieves the dependency by name.
//   5. Calling dep->initDependencies() ensures the dependency is
//      fully registered before we reference its types / functions.
//
// See dasAudio.cpp (requires "rtti") and dasIMGUI_NODE_EDITOR.cpp
// (requires "imgui") for real-world examples of this pattern.
// -----------------------------------------------------------------------

class Module_MathUtils : public Module {
    bool initialized = false;
public:
    Module_MathUtils() : Module("math_utils") {
        // Empty — all registration is deferred to initDependencies().
    }

    bool initDependencies() override {
        // Guard: avoid double-initialization.
        if (initialized) return true;

        // Look up the dependency by its registered name.
        // Returns nullptr if the module was not loaded.
        auto mod_math_types = Module::require("math_types");
        if (!mod_math_types) return false;

        // Ensure the dependency is fully initialized.
        // This is critical: the dependency's types/functions may not
        // be registered yet if its own initDependencies() hasn't run.
        if (!mod_math_types->initDependencies()) return false;

        // Mark as initialized before registration (prevents re-entry).
        initialized = true;

        // Set up the module library — addBuiltinDependency makes Vec2
        // visible in our function signatures and records the dependency
        // for the compiler.
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addBuiltinDependency(lib, mod_math_types);

        // --- Functions using Vec2 (defined in math_types) ---
        addExtern<DAS_BIND_FUN(vec2_length)>(*this, lib, "length",
            SideEffects::none, "vec2_length")
                ->args({"v"});

        addExtern<DAS_BIND_FUN(vec2_dot)>(*this, lib, "dot",
            SideEffects::none, "vec2_dot")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec2_normalize), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "normalize",
            SideEffects::none, "vec2_normalize")
                ->args({"v"});

        // Operators
        addExtern<DAS_BIND_FUN(vec2_add), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "+",
            SideEffects::none, "vec2_add")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec2_scale), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "*",
            SideEffects::none, "vec2_scale")
                ->args({"v", "s"});

        addExtern<DAS_BIND_FUN(vec2_lerp), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "lerp",
            SideEffects::none, "vec2_lerp")
                ->args({"a", "b", "t"});

        return true;
    }
};

REGISTER_MODULE(Module_MathUtils);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/10_custom_modules.das"

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
    // Both modules must be listed. With initDependencies(), the
    // ordering is handled automatically — Module_MathUtils will
    // call mod_math_types->initDependencies() before proceeding.
    NEED_MODULE(Module_MathTypes);
    NEED_MODULE(Module_MathUtils);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
