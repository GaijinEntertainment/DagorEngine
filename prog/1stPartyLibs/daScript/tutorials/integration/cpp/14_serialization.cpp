// Tutorial 14 — Serialization (C++ integration)
//
// Demonstrates how to serialize a compiled daslang program to a
// binary blob and deserialize it back, skipping recompilation.
//
// Workflow:
//   1. Compile a daslang program normally
//   2. Serialize the compiled program to memory (or to a file)
//   3. Release the original program
//   4. Deserialize from the blob — no parsing, no type inference
//   5. Simulate and run as usual
//
// This is useful for:
//   - Faster startup (skip compilation on subsequent runs)
//   - Distributing pre-compiled scripts
//   - Caching compiled programs in editors / build pipelines

#include "daScript/daScript.h"
#include "daScript/ast/ast_serializer.h"

using namespace das;

// -----------------------------------------------------------------------
// Host program — Serialization demo
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/14_serialization.das"

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    // ================================================================
    // Stage 1: Compile the program from source
    // ================================================================
    tout << "=== Stage 1: Compile from source ===\n";

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
    tout << "  Compiled successfully.\n";

    // We must simulate before serializing — simulation resolves
    // function pointers and initializes the program fully.
    {
        Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            tout << "Simulation failed.\n";
            return;
        }
        tout << "  Simulated successfully.\n";
    }

    // ================================================================
    // Stage 2: Serialize to an in-memory buffer
    // ================================================================
    tout << "\n=== Stage 2: Serialize ===\n";

    auto writeTo = make_unique<SerializationStorageVector>();
    {
        AstSerializer ser(writeTo.get(), true);  // writing = true
        program->serialize(ser);
        ser.moduleLibrary = nullptr;
    }

    size_t blobSize = writeTo->buffer.size();
    tout << "  Serialized size: " << blobSize << " bytes\n";

    // Release the original program — we no longer need it.
    program.reset();
    tout << "  Original program released.\n";

    // ================================================================
    // Stage 3: Deserialize from the buffer
    // ================================================================
    tout << "\n=== Stage 3: Deserialize ===\n";

    auto readFrom = make_unique<SerializationStorageVector>();
    readFrom->buffer = das::move(writeTo->buffer);
    {
        AstSerializer deser(readFrom.get(), false);  // writing = false
        program = make_smart<Program>();
        program->serialize(deser);
        deser.moduleLibrary = nullptr;
    }
    tout << "  Deserialized successfully.\n";

    // ================================================================
    // Stage 4: Simulate and run (same as a freshly compiled program)
    // ================================================================
    tout << "\n=== Stage 4: Simulate and run ===\n";

    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation of deserialized program failed.\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found.\n";
        return;
    }

    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "Exception: " << ex << "\n";
    }
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
