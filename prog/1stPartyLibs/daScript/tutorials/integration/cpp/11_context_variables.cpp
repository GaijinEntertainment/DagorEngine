// Tutorial 11 — Context Variables (C++ integration)
//
// Demonstrates how to access daslang global variables from C++:
//   - ctx.findVariable() — look up a variable by name
//   - ctx.getVariable() — get a raw pointer to variable data
//   - Reading and writing global variables from the host
//   - Calling daslang functions that use those globals

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include <cstdio>

using namespace das;

// -----------------------------------------------------------------------
// A simple C++ type that will be a global variable in daslang
// -----------------------------------------------------------------------

struct GameConfig {
    float gravity;
    float speed;
    int32_t max_enemies;
};

MAKE_TYPE_FACTORY(GameConfig, GameConfig);

namespace das {

struct GameConfigAnnotation : ManagedStructureAnnotation<GameConfig, false> {
    GameConfigAnnotation(ModuleLibrary & ml)
        : ManagedStructureAnnotation("GameConfig", ml)
    {
        addField<DAS_BIND_MANAGED_FIELD(gravity)>("gravity", "gravity");
        addField<DAS_BIND_MANAGED_FIELD(speed)>("speed", "speed");
        addField<DAS_BIND_MANAGED_FIELD(max_enemies)>("max_enemies", "max_enemies");
    }
    // POD-like: allow local variables without unsafe
    virtual bool isPod() const override { return true; }
    virtual bool isRawPod() const override { return true; }
};

} // namespace das

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial11 : public Module {
public:
    Module_Tutorial11() : Module("tutorial_11_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        addAnnotation(make_smart<GameConfigAnnotation>(lib));
        addCtorAndUsing<GameConfig>(*this, lib, "GameConfig", "GameConfig");
    }
};

REGISTER_MODULE(Module_Tutorial11);

// -----------------------------------------------------------------------
// Host program — reads and writes daslang globals from C++
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/11_context_variables.das"

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

    // ---------------------------------------------------------------
    // 1. Reading scalar globals by name
    // ---------------------------------------------------------------
    printf("=== Reading globals from C++ ===\n");

    // findVariable returns an index, getVariable returns void*
    int idx_score = ctx.findVariable("score");
    if (idx_score >= 0) {
        int32_t * pScore = (int32_t *)ctx.getVariable(idx_score);
        printf("score = %d\n", *pScore);
    }

    int idx_name = ctx.findVariable("player_name");
    if (idx_name >= 0) {
        // String globals are stored as char* in the context
        char ** pName = (char **)ctx.getVariable(idx_name);
        printf("player_name = %s\n", *pName);
    }

    int idx_alive = ctx.findVariable("alive");
    if (idx_alive >= 0) {
        bool * pAlive = (bool *)ctx.getVariable(idx_alive);
        printf("alive = %s\n", *pAlive ? "true" : "false");
    }

    // ---------------------------------------------------------------
    // 2. Reading a struct global
    // ---------------------------------------------------------------
    int idx_config = ctx.findVariable("config");
    if (idx_config >= 0) {
        GameConfig * pConfig = (GameConfig *)ctx.getVariable(idx_config);
        printf("config.gravity     = %.1f\n", pConfig->gravity);
        printf("config.speed       = %.1f\n", pConfig->speed);
        printf("config.max_enemies = %d\n", pConfig->max_enemies);
    }

    // ---------------------------------------------------------------
    // 3. Writing globals from C++ — changes are visible to daslang
    // ---------------------------------------------------------------
    printf("\n=== Writing globals from C++ ===\n");

    if (idx_score >= 0) {
        int32_t * pScore = (int32_t *)ctx.getVariable(idx_score);
        *pScore = 9999;
        printf("C++ set score = 9999\n");
    }

    if (idx_config >= 0) {
        GameConfig * pConfig = (GameConfig *)ctx.getVariable(idx_config);
        pConfig->gravity = 20.0f;
        pConfig->max_enemies = 100;
        printf("C++ set config.gravity = 20.0, max_enemies = 100\n");
    }

    // ---------------------------------------------------------------
    // 4. Call a daslang function that reads the modified globals
    // ---------------------------------------------------------------
    printf("\n=== Calling daslang to verify ===\n");
    auto fnPrint = ctx.findFunction("print_globals");
    if (fnPrint) {
        ctx.evalWithCatch(fnPrint, nullptr);
        if (auto ex = ctx.getException()) {
            tout << "Exception: " << ex << "\n";
        }
    }

    // ---------------------------------------------------------------
    // 5. Enumerate all global variables
    // ---------------------------------------------------------------
    printf("\n=== All global variables ===\n");
    int total = ctx.getTotalVariables();
    for (int i = 0; i < total; i++) {
        auto * info = ctx.getVariableInfo(i);
        if (info) {
            printf("  [%d] %s (size=%d)\n", i, info->name, info->size);
        }
    }
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    NEED_MODULE(Module_Tutorial11);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
