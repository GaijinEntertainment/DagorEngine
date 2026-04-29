// Tutorial 04 — Binding C++ Types (C++ integration)
//
// Demonstrates how to expose C++ structs/classes to daslang:
//   - MAKE_TYPE_FACTORY — register a C++ type so daslang can see it
//   - ManagedStructureAnnotation — describe struct layout (fields)
//   - addAnnotation — plug the annotation into a module
//   - addExtern for functions taking/returning the bound type
//   - Methods via das_call_member
//
// After binding, scripts can create instances, read/write fields,
// and pass these types to/from C++ functions seamlessly.

#include "daScript/daScript.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>
#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// C++ types that we want to expose to daslang
// -----------------------------------------------------------------------

// A simple 2D vector (POD — no default initializers).
struct Vec2 {
    float x;
    float y;
};

// A color with an alpha component.
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

// A rectangle defined by position and size.
struct Rect {
    Vec2  pos;
    Vec2  size;
};

// -----------------------------------------------------------------------
// Free functions that operate on the types
// -----------------------------------------------------------------------

float vec2_length(const Vec2 & v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vec2 vec2_add(const Vec2 & a, const Vec2 & b) {
    return { a.x + b.x, a.y + b.y };
}

Vec2 vec2_scale(const Vec2 & v, float s) {
    return { v.x * s, v.y * s };
}

float vec2_dot(const Vec2 & a, const Vec2 & b) {
    return a.x * b.x + a.y * b.y;
}

void vec2_normalize(Vec2 & v) {
    float len = vec2_length(v);
    if (len > 0.0f) {
        v.x /= len;
        v.y /= len;
    }
}

// --- Constructor / factory functions ---
// These let scripts create instances without needing `unsafe`.

Vec2 make_vec2(float x, float y) {
    Vec2 v;  v.x = x;  v.y = y;
    return v;
}

Color make_color(int32_t r, int32_t g, int32_t b, int32_t a) {
    Color c;
    c.r = (uint8_t)r;  c.g = (uint8_t)g;
    c.b = (uint8_t)b;  c.a = (uint8_t)a;
    return c;
}

Rect make_rect(float px, float py, float sx, float sy) {
    Rect r;
    r.pos.x  = px;  r.pos.y  = py;
    r.size.x = sx;  r.size.y = sy;
    return r;
}

float rect_area(const Rect & r) {
    return r.size.x * r.size.y;
}

bool rect_contains(const Rect & r, const Vec2 & p) {
    return p.x >= r.pos.x && p.x <= r.pos.x + r.size.x
        && p.y >= r.pos.y && p.y <= r.pos.y + r.size.y;
}

// -----------------------------------------------------------------------
// MAKE_TYPE_FACTORY — file-scope declarations
// -----------------------------------------------------------------------
// Each MAKE_TYPE_FACTORY creates:
//   - typeFactory<CppType> so addExtern can map parameters automatically
//   - typeName<CppType>   so daslang knows the type's name

MAKE_TYPE_FACTORY(Vec2,  Vec2);
MAKE_TYPE_FACTORY(Color, Color);
MAKE_TYPE_FACTORY(Rect,  Rect);

// -----------------------------------------------------------------------
// ManagedStructureAnnotation — describe each type's layout
// -----------------------------------------------------------------------

namespace das {

struct Vec2Annotation : ManagedStructureAnnotation<Vec2, false> {
    Vec2Annotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Vec2", ml)
    {
        // addField<DAS_BIND_MANAGED_FIELD(member)>("das_name")
        // binds a C++ struct member so scripts can read/write it.
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
    }
};

struct ColorAnnotation : ManagedStructureAnnotation<Color, false> {
    ColorAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Color", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(r)>("r", "r");
        addField<DAS_BIND_MANAGED_FIELD(g)>("g", "g");
        addField<DAS_BIND_MANAGED_FIELD(b)>("b", "b");
        addField<DAS_BIND_MANAGED_FIELD(a)>("a", "a");
    }
};

struct RectAnnotation : ManagedStructureAnnotation<Rect, false> {
    RectAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Rect", ml)
    {
        // Fields can themselves be bound types (Vec2).
        addField<DAS_BIND_MANAGED_FIELD(pos)>("pos",   "pos");
        addField<DAS_BIND_MANAGED_FIELD(size)>("size", "size");
    }
};

} // namespace das

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial04 : public Module {
public:
    Module_Tutorial04() : Module("tutorial_04_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Register type annotations ---
        // Order matters: Vec2 must come before Rect (which uses Vec2).
        addAnnotation(make_smart<Vec2Annotation>(lib));
        addAnnotation(make_smart<ColorAnnotation>(lib));
        addAnnotation(make_smart<RectAnnotation>(lib));

        // --- Bind functions that take/return bound types ---
        // The typeFactory<> created by MAKE_TYPE_FACTORY lets addExtern
        // automatically resolve Vec2, Color, Rect in function signatures.

        addExtern<DAS_BIND_FUN(vec2_length)>(*this, lib, "vec2_length",
            SideEffects::none, "vec2_length")
                ->args({"v"});

        // vec2_add returns Vec2 by value → needs SimNode_ExtFuncCallAndCopyOrMove
        addExtern<DAS_BIND_FUN(vec2_add), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "vec2_add",
            SideEffects::none, "vec2_add")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec2_scale), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "vec2_scale",
            SideEffects::none, "vec2_scale")
                ->args({"v", "s"});

        addExtern<DAS_BIND_FUN(vec2_dot)>(*this, lib, "vec2_dot",
            SideEffects::none, "vec2_dot")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec2_normalize)>(*this, lib, "vec2_normalize",
            SideEffects::modifyArgument, "vec2_normalize")
                ->args({"v"});

        // --- Factory functions (return by value → SimNode_ExtFuncCallAndCopyOrMove) ---
        addExtern<DAS_BIND_FUN(make_vec2), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_vec2",
            SideEffects::none, "make_vec2")
                ->args({"x", "y"});

        addExtern<DAS_BIND_FUN(make_color), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_color",
            SideEffects::none, "make_color")
                ->args({"r", "g", "b", "a"});

        addExtern<DAS_BIND_FUN(make_rect), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_rect",
            SideEffects::none, "make_rect")
                ->args({"px", "py", "sx", "sy"});

        addExtern<DAS_BIND_FUN(rect_area)>(*this, lib, "rect_area",
            SideEffects::none, "rect_area")
                ->args({"r"});

        addExtern<DAS_BIND_FUN(rect_contains)>(*this, lib, "rect_contains",
            SideEffects::none, "rect_contains")
                ->args({"r", "p"});
    }
};

REGISTER_MODULE(Module_Tutorial04);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/04_binding_types.das"

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
    NEED_MODULE(Module_Tutorial04);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
