// Tutorial 09 — Operators and Properties (C++ integration)
//
// Demonstrates how to expose C++ operators and properties to daslang:
//   - addEquNeq — binding == and != operators
//   - Custom operators (+, -, *, etc.) via addExtern
//   - addProperty / addPropertyExtConst — property accessors
//   - Properties appear as field-like access in daslang

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>
#include <cmath>
#include <cstring>

using namespace das;

// -----------------------------------------------------------------------
// C++ type with operators and properties
// -----------------------------------------------------------------------

struct Vec3 {
    float x;
    float y;
    float z;

    // Operators — used by addEquNeq and custom operator bindings
    bool operator==(const Vec3 & o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    bool operator!=(const Vec3 & o) const {
        return !(*this == o);
    }

    // Property methods — will be bound with addProperty
    float length() const {
        return sqrtf(x * x + y * y + z * z);
    }
    float lengthSq() const {
        return x * x + y * y + z * z;
    }
    bool isZero() const {
        return x == 0.0f && y == 0.0f && z == 0.0f;
    }

    // Const/non-const property overloads.
    // Non-const version — called when the object is mutable (var):
    bool editable() { return true; }
    // Const version — called when the object is immutable (let):
    bool editable() const { return false; }
};

// Factory
Vec3 make_vec3(float x, float y, float z) {
    Vec3 v;  v.x = x;  v.y = y;  v.z = z;
    return v;
}

// -----------------------------------------------------------------------
// Non-POD type — has a non-trivial constructor, so isPod() returns false.
// This means mutable local variables (var) require `unsafe` in daslang.
// -----------------------------------------------------------------------

struct Color {
    float r, g, b, a;

    // Non-trivial constructor makes this non-POD
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

    float brightness() const {
        return 0.299f * r + 0.587f * g + 0.114f * b;
    }

    // Const/non-const property — same pattern as Vec3::editable
    bool editable() { return true; }
    bool editable() const { return false; }
};

// Factory
Color make_color(float r, float g, float b, float a) {
    Color c;
    c.r = r;  c.g = g;  c.b = b;  c.a = a;
    return c;
}

// Operator functions — these will be bound with operator names
Vec3 vec3_add(const Vec3 & a, const Vec3 & b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 vec3_sub(const Vec3 & a, const Vec3 & b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 vec3_mul_scalar(const Vec3 & a, float s) {
    return { a.x * s, a.y * s, a.z * s };
}

Vec3 vec3_neg(const Vec3 & a) {
    return { -a.x, -a.y, -a.z };
}

float vec3_dot(const Vec3 & a, const Vec3 & b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(const Vec3 & a, const Vec3 & b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 vec3_normalize(const Vec3 & v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        return { v.x / len, v.y / len, v.z / len };
    }
    return v;
}

// -----------------------------------------------------------------------
// MAKE_TYPE_FACTORY
// -----------------------------------------------------------------------

MAKE_TYPE_FACTORY(Vec3, Vec3);
MAKE_TYPE_FACTORY(Color, Color);

// SafeColor — identical layout to Color but with annotation overrides
// that make it usable without `unsafe` in daslang.
// Defined as a separate struct (not inheriting Color) so that
// member-function pointers resolve to SafeColor, not Color.
struct SafeColor {
    float r, g, b, a;
    SafeColor() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
    float brightness() const {
        return 0.299f * r + 0.587f * g + 0.114f * b;
    }
};
MAKE_TYPE_FACTORY(SafeColor, SafeColor);

// -----------------------------------------------------------------------
// Annotation with properties
// -----------------------------------------------------------------------

namespace das {

struct Vec3Annotation : ManagedStructureAnnotation<Vec3, false> {
    Vec3Annotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Vec3", ml)
    {
        // Regular fields — accessible as vec.x, vec.y, vec.z
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
        addField<DAS_BIND_MANAGED_FIELD(z)>("z", "z");

        // Properties — accessible like fields but call C++ methods
        // `vec.length` calls Vec3::length() under the hood
        addProperty<DAS_BIND_MANAGED_PROP(length)>("length", "length");
        addProperty<DAS_BIND_MANAGED_PROP(lengthSq)>("lengthSq", "lengthSq");
        addProperty<DAS_BIND_MANAGED_PROP(isZero)>("isZero", "isZero");

        // Const/non-const property — addPropertyExtConst
        // When accessed on a mutable (var) object, calls the non-const overload.
        // When accessed on an immutable (let) object, calls the const overload.
        // Template params: <NonConstSig, &Method, ConstSig, &Method>
        addPropertyExtConst<
            bool (Vec3::*)(),       &Vec3::editable,    // non-const overload
            bool (Vec3::*)() const, &Vec3::editable     // const overload
        >("editable", "editable");
    }
};

// ColorAnnotation — NO overrides.
// Color has a non-trivial constructor, so isLocal() returns false.
// Local variables of type Color require `unsafe`, or use the
// `using` pattern: Color() <| $(var c) { ... }
struct ColorAnnotation : ManagedStructureAnnotation<Color, false> {
    ColorAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Color", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(r)>("r", "r");
        addField<DAS_BIND_MANAGED_FIELD(g)>("g", "g");
        addField<DAS_BIND_MANAGED_FIELD(b)>("b", "b");
        addField<DAS_BIND_MANAGED_FIELD(a)>("a", "a");
        addProperty<DAS_BIND_MANAGED_PROP(brightness)>("brightness", "brightness");
    }
};

// SafeColorAnnotation — same C++ type, but with annotation overrides
// that tell daslang "treat this as POD-like".
// This makes local variables work without `unsafe`.
struct SafeColorAnnotation : ManagedStructureAnnotation<SafeColor, false> {
    SafeColorAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("SafeColor", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(r)>("r", "r");
        addField<DAS_BIND_MANAGED_FIELD(g)>("g", "g");
        addField<DAS_BIND_MANAGED_FIELD(b)>("b", "b");
        addField<DAS_BIND_MANAGED_FIELD(a)>("a", "a");
        addProperty<DAS_BIND_MANAGED_PROP(brightness)>("brightness", "brightness");
    }
    // Override: treat as POD even though C++ has a non-trivial ctor.
    // isLocal() = isPod() && !hasNonTrivialCtor() && ...
    // Without these, error[30108] fires for local variables.
    virtual bool isPod() const override { return true; }
    virtual bool isRawPod() const override { return false; }
    virtual bool hasNonTrivialCtor() const override { return false; }
};

} // namespace das

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial09 : public Module {
public:
    Module_Tutorial09() : Module("tutorial_09_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Register types ---
        addAnnotation(make_smart<Vec3Annotation>(lib));
        addAnnotation(make_smart<ColorAnnotation>(lib));
        addAnnotation(make_smart<SafeColorAnnotation>(lib));

        // --- Factory ---
        addExtern<DAS_BIND_FUN(make_vec3), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_vec3",
            SideEffects::none, "make_vec3")
                ->args({"x", "y", "z"});

        // --- Color factory + constructor ---
        addExtern<DAS_BIND_FUN(make_color), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_color",
            SideEffects::none, "make_color")
                ->args({"r", "g", "b", "a"});

        // addCtorAndUsing registers TWO things:
        //   1. A constructor: Color() / SafeColor() — placement new
        //   2. A `using` function: Color() <| $(var c) { ... }
        // Color local vars need `unsafe` (no annotation overrides),
        // but `using` works safely. SafeColor has overrides, so
        // `let sc = SafeColor()` works without `unsafe`.
        addCtorAndUsing<Color>(*this, lib, "Color", "Color");
        addCtorAndUsing<SafeColor>(*this, lib, "SafeColor", "SafeColor");

        // --- Equality operators (== and !=) ---
        // addEquNeq requires operator== and operator!= on the type.
        addEquNeq<Vec3>(*this, lib);

        // --- Arithmetic operators ---
        // Bind C++ functions with operator names (+, -, *, etc.)
        // daslang resolves operators by name, so "+" becomes the + operator.
        addExtern<DAS_BIND_FUN(vec3_add), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "+",
            SideEffects::none, "vec3_add")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec3_sub), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "-",
            SideEffects::none, "vec3_sub")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec3_mul_scalar), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "*",
            SideEffects::none, "vec3_mul_scalar")
                ->args({"a", "s"});

        addExtern<DAS_BIND_FUN(vec3_neg), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "-",
            SideEffects::none, "vec3_neg")
                ->args({"a"});

        // --- Utility functions (not operators) ---
        addExtern<DAS_BIND_FUN(vec3_dot)>(*this, lib, "dot",
            SideEffects::none, "vec3_dot")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec3_cross), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "cross",
            SideEffects::none, "vec3_cross")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(vec3_normalize), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "normalize",
            SideEffects::none, "vec3_normalize")
                ->args({"v"});
    }
};

REGISTER_MODULE(Module_Tutorial09);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/09_operators_and_properties.das"

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
    NEED_MODULE(Module_Tutorial09);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
