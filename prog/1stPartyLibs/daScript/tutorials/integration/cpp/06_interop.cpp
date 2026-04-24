// Tutorial 06 — Low-Level Interop (C++ integration)
//
// Demonstrates `addInterop` — the low-level alternative to `addExtern`.
// Unlike `addExtern`, interop functions receive raw simulation arguments
// and can:
//   - Accept "any type" arguments (vec4f template parameter)
//   - Inspect TypeInfo at runtime (call->types[i])
//   - Access call-site debug info (call->debugInfo)
//
// Use `addInterop` when you need type-generic functions that work with
// arbitrary daslang types — like `sprint`, `hash`, or `write`.

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>
#include <cstring>
#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// Interop function 1: describe_type
//
// Takes ANY daslang value and prints its type name and basic info.
// The `vec4f` template parameter means "any type" — the C++ function
// inspects `call->types[0]` at runtime to determine what was passed.
// -----------------------------------------------------------------------

vec4f describe_type(Context & context, SimNode_CallBase * call, vec4f * args) {
    TypeInfo * ti = call->types[0];  // TypeInfo for the first argument

    // NOTE: TypeInfo has a union: structType / enumType / annotation_or_name
    // Which member is valid depends on ti->type:
    //   tStructure   → structType (StructInfo *)
    //   tEnumeration → enumType   (EnumInfo *)
    //   tHandle      → annotation_or_name (use getAnnotation())
    // Accessing the wrong union member is undefined behavior!

    TextWriter tw;

    if (ti->type == Type::tHandle) {
        // Handled types (bound via ManagedStructureAnnotation etc.)
        // das_to_string(tHandle) returns "" — use getAnnotation()->name
        auto ann = ti->getAnnotation();
        tw << "type = handle";
        if (ann) {
            tw << ", name = " << ann->name;
        }
        tw << ", size = " << getTypeSize(ti);
    } else if (ti->type == Type::tStructure && ti->structType) {
        tw << "type = struct";
        tw << ", name = " << ti->structType->name;
        tw << ", fields = " << ti->structType->count;
        tw << ", size = " << getTypeSize(ti);
    } else if (ti->type == Type::tEnumeration || ti->type == Type::tEnumeration8
            || ti->type == Type::tEnumeration16) {
        tw << "type = " << das_to_string(ti->type);
        if (ti->enumType) {
            tw << ", enum name = " << ti->enumType->name;
            tw << ", values = " << ti->enumType->count;
        }
    } else if (ti->type == Type::tArray) {
        tw << "type = array";
        if (ti->firstType) {
            tw << ", element type = " << das_to_string(ti->firstType->type);
        }
    } else {
        tw << "type = " << das_to_string(ti->type);
        tw << ", size = " << getTypeSize(ti);
    }

    auto result = context.allocateString(tw.str(), &call->debugInfo);
    return cast<char *>::from(result);
}

// -----------------------------------------------------------------------
// Interop function 2: debug_print
//
// Takes ANY value and prints it using the daslang debug printer.
// Similar to the built-in `sprint` but simplified for the tutorial.
// -----------------------------------------------------------------------

vec4f debug_print(Context & context, SimNode_CallBase * call, vec4f * args) {
    TypeInfo * ti = call->types[0];
    auto val = args[0];

    // Use the built-in debug_value formatter
    TextWriter tw;
    tw << debug_value(val, ti, PrintFlags::debugger);

    auto result = context.allocateString(tw.str(), &call->debugInfo);
    return cast<char *>::from(result);
}

// -----------------------------------------------------------------------
// Interop function 3: any_hash
//
// Takes ANY value and returns a 64-bit hash.  Uses the built-in
// hash_value() which dispatches on TypeInfo.
// -----------------------------------------------------------------------

vec4f any_hash(Context & context, SimNode_CallBase * call, vec4f * args) {
    auto h = hash_value(context, args[0], call->types[0]);
    return cast<uint64_t>::from(h);
}

// -----------------------------------------------------------------------
// Interop function 4: struct_field_names
//
// Takes ANY struct value and returns a string listing its field names.
// Demonstrates deeper TypeInfo inspection (structType->fields).
// -----------------------------------------------------------------------

vec4f struct_field_names(Context & context, SimNode_CallBase * call, vec4f * args) {
    TypeInfo * ti = call->types[0];

    if (ti->type != Type::tStructure || !ti->structType) {
        context.throw_error_at(call->debugInfo,
            "struct_field_names: expected a daslang struct (not a handled type)");
        return v_zero();
    }

    TextWriter tw;
    for (uint32_t i = 0; i < ti->structType->count; ++i) {
        if (i > 0) tw << ", ";
        tw << ti->structType->fields[i]->name;
    }

    auto result = context.allocateString(tw.str(), &call->debugInfo);
    return cast<char *>::from(result);
}

// -----------------------------------------------------------------------
// Interop function 5: call_site_info
//
// Takes no meaningful arguments — demonstrates accessing call->debugInfo
// to report the source location of the call site.
// -----------------------------------------------------------------------

vec4f call_site_info(Context & context, SimNode_CallBase * call, vec4f *) {
    TextWriter tw;
    if (call->debugInfo.fileInfo) {
        tw << call->debugInfo.fileInfo->name << ":" << call->debugInfo.line;
    } else {
        tw << "(unknown):" << call->debugInfo.line;
    }

    auto result = context.allocateString(tw.str(), &call->debugInfo);
    return cast<char *>::from(result);
}

// -----------------------------------------------------------------------
// A simple struct bound for use in the demo script
// -----------------------------------------------------------------------

struct Particle {
    float x;
    float y;
    float vx;
    float vy;
};

MAKE_TYPE_FACTORY(Particle, Particle);

namespace das {
struct ParticleAnnotation : ManagedStructureAnnotation<Particle, false> {
    ParticleAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Particle", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
        addField<DAS_BIND_MANAGED_FIELD(vx)>("vx", "vx");
        addField<DAS_BIND_MANAGED_FIELD(vy)>("vy", "vy");
    }
};
} // namespace das

Particle make_particle(float x, float y, float vx, float vy) {
    Particle p;
    p.x = x;  p.y = y;  p.vx = vx;  p.vy = vy;
    return p;
}

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial06 : public Module {
public:
    Module_Tutorial06() : Module("tutorial_06_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // Register the Particle type
        addAnnotation(make_smart<ParticleAnnotation>(lib));
        addExtern<DAS_BIND_FUN(make_particle), SimNode_ExtFuncCallAndCopyOrMove>(
            *this, lib, "make_particle",
            SideEffects::none, "make_particle")
                ->args({"x", "y", "vx", "vy"});

        // --- Interop functions ---
        // Note the signature: addInterop<FunctionPtr, ReturnType, ArgTypes...>
        // When ArgType is `vec4f`, it means "any daslang type".

        // describe_type(value) — any type → string
        addInterop<describe_type, char *, vec4f>(*this, lib, "describe_type",
            SideEffects::none, "describe_type")
                ->arg("value");

        // debug_print(value) — any type → string
        addInterop<debug_print, char *, vec4f>(*this, lib, "debug_print",
            SideEffects::none, "debug_print")
                ->arg("value");

        // any_hash(value) — any type → uint64
        addInterop<any_hash, uint64_t, vec4f>(*this, lib, "any_hash",
            SideEffects::none, "any_hash")
                ->arg("value");

        // struct_field_names(value) — struct → string
        addInterop<struct_field_names, char *, vec4f>(*this, lib, "struct_field_names",
            SideEffects::none, "struct_field_names")
                ->arg("value");

        // call_site_info() — no args → string (source location)
        addInterop<call_site_info, char *>(*this, lib, "call_site_info",
            SideEffects::none, "call_site_info");
    }
};

REGISTER_MODULE(Module_Tutorial06);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/06_interop.das"

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
    NEED_MODULE(Module_Tutorial06);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
