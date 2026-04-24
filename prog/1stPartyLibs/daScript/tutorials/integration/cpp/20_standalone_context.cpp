// Tutorial 20 — Standalone Context (C++ integration)
//
// Shows how to use a pre-compiled daslang program as a standalone
// C++ context with zero runtime compilation overhead.
//
// Normal daslang embedding compiles .das files at startup:
//   compileDaScript → simulate → evalWithCatch
//
// A standalone context eliminates the compile step entirely.  The AOT
// tool generates a .das.h and .das.cpp that contain all function code,
// type info, and initialization logic baked into C++.  The host just
// instantiates the generated Standalone class and calls methods directly.
//
// Build pipeline (handled by CMake):
//   1. daslang -aot -ctx 20_standalone_context.das  →  .das.h + .das.cpp
//   2. Compile .das.cpp into the tutorial executable
//   3. The main() below just uses the generated class
//
// Key concepts:
//   - DAS_AOT_CTX CMake macro — generates standalone context artifacts
//   - Generated Standalone class (extends Context)
//   - Direct method calls — test() is a C++ function, no lookup needed
//   - Zero startup cost — no parsing, no type checking, no simulation

#include "daScript/daScript.h"

// The generated header declares the Standalone class in a namespace
// derived from the script file name.
#include "_standalone_ctx_generated/standalone_context.das.h"

using namespace das;

int main(int, char * []) {
    // No NEED_ALL_DEFAULT_MODULES, no Module::Initialize needed for
    // standalone contexts — everything is self-contained.

    TextPrinter tout;
    tout << "Creating standalone context (no runtime compilation)...\n";

    // Instantiate the standalone context.
    // The constructor sets up all functions, globals, and type info
    // from pre-generated AOT data.
    auto ctx = standalone_context::Standalone();

    // Call the test function directly — this is a normal C++ method call,
    // not a lookup + eval.  Maximum performance, minimum overhead.
    tout << "Calling test():\n";
    ctx.test();

    tout << "Done.\n";
    return 0;
}
