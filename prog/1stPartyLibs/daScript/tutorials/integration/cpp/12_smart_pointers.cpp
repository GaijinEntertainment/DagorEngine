// Tutorial 12 — Smart Pointers (C++ integration)
//
// Demonstrates how to expose reference-counted C++ types to daslang:
//   - Inheriting from das::ptr_ref_count for reference counting
//   - ManagedStructureAnnotation with canNew/canDelete
//   - smart_ptr<T> in daslang scripts (var inscope)
//   - Factory functions returning smart_ptr
//   - Passing smart_ptr between C++ and daslang

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>

using namespace das;

// -----------------------------------------------------------------------
// A reference-counted C++ type.
// Inheriting from ptr_ref_count gives addRef()/delRef()/use_count().
// This is the ONLY requirement for smart_ptr support in daslang.
// -----------------------------------------------------------------------

class Entity : public ptr_ref_count {
public:
    string name;
    float  x, y;
    int32_t health;

    Entity() : name("unnamed"), x(0), y(0), health(100) {
        printf("  [C++] Entity('%s') constructed\n", name.c_str());
    }

    ~Entity() {
        printf("  [C++] Entity('%s') destroyed\n", name.c_str());
    }

    void move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    float distance_to(const Entity & other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return sqrtf(dx * dx + dy * dy);
    }

    void take_damage(int32_t amount) {
        health -= amount;
        if (health < 0) health = 0;
    }

    bool is_alive() const {
        return health > 0;
    }
};

// Factory — returns a smart_ptr so daslang manages the lifetime
smart_ptr<Entity> make_entity(const char * name, float x, float y) {
    auto e = make_smart<Entity>();
    e->name = name;
    e->x = x;
    e->y = y;
    return e;
}

// -----------------------------------------------------------------------
// Method wrappers (daslang has no member functions — use free functions)
// -----------------------------------------------------------------------

void entity_move(Entity & e, float dx, float dy) {
    e.move(dx, dy);
}

float entity_distance_to(const Entity & a, const Entity & b) {
    return a.distance_to(b);
}

void entity_take_damage(Entity & e, int32_t amount) {
    e.take_damage(amount);
}

// -----------------------------------------------------------------------
// MAKE_TYPE_FACTORY + Annotation
// -----------------------------------------------------------------------

MAKE_TYPE_FACTORY(Entity, Entity);

namespace das {

struct EntityAnnotation : ManagedStructureAnnotation<Entity, true, true> {
    //                                                       ^^^^  ^^^^
    //                                              canNew --+      |
    //                                              canDelete ------+
    // canNew=true:    daslang can call `new Entity()` → allocates + addRef
    // canDelete=true: daslang can call `delete ptr`   → delRef
    EntityAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("Entity", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(name)>("name", "name");
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
        addField<DAS_BIND_MANAGED_FIELD(health)>("health", "health");
        addProperty<DAS_BIND_MANAGED_PROP(is_alive)>("is_alive", "is_alive");
    }
    // isSmart() is auto-detected because Entity inherits ptr_ref_count.
    // No override needed — ManagedStructureAnnotation checks
    // is_base_of<ptr_ref_count, Entity> automatically.
};

} // namespace das

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial12 : public Module {
public:
    Module_Tutorial12() : Module("tutorial_12_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        addAnnotation(make_smart<EntityAnnotation>(lib));

        // Factory returning smart_ptr<Entity>
        addExtern<DAS_BIND_FUN(make_entity)>(*this, lib, "make_entity",
            SideEffects::modifyExternal, "make_entity")
                ->args({"name", "x", "y"});

        // Methods (free functions taking Entity &)
        addExtern<DAS_BIND_FUN(entity_move)>(*this, lib, "move_entity",
            SideEffects::modifyArgument, "entity_move")
                ->args({"entity", "dx", "dy"});

        addExtern<DAS_BIND_FUN(entity_distance_to)>(*this, lib, "distance_to",
            SideEffects::none, "entity_distance_to")
                ->args({"a", "b"});

        addExtern<DAS_BIND_FUN(entity_take_damage)>(*this, lib, "take_damage",
            SideEffects::modifyArgument, "entity_take_damage")
                ->args({"entity", "amount"});
    }
};

REGISTER_MODULE(Module_Tutorial12);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/12_smart_pointers.das"

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
    NEED_MODULE(Module_Tutorial12);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
