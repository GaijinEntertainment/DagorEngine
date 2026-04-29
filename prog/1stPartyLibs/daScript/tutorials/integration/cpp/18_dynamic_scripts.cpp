// Tutorial 18 — Dynamic Script Generation (C++ integration)
//
// Shows how to build a daslang program from a string at run-time,
// compile it from a virtual file (no disk I/O), and interact with its
// global variables directly through context memory pointers.
//
// Use-case: an expression calculator that lets the host set variable
// values and evaluate arbitrary math expressions at near-native speed.
//
// Key concepts:
//   - TextWriter — build daslang source as a C++ string
//   - TextFileInfo + FsFileAccess::setFileInfo — register a virtual file
//   - compileDaScript on a virtual file name
//   - ctx.findVariable / ctx.getVariable — locate globals in context memory
//   - Direct pointer writes to context globals
//   - shared_ptr<Context> for long-lived context reuse

#include "daScript/daScript.h"

using namespace das;

// ---------------------------------------------------------------------------
// ExprCalc — a simple expression calculator backed by a compiled daslang
// program.  Construction compiles the expression; compute() sets variables
// and evaluates it.
// ---------------------------------------------------------------------------

typedef das_map<string, float> ExprVars;

class ExprCalc {
public:
    ExprCalc(const string & expr, const ExprVars & vars);
    float compute(ExprVars & vars);
private:
    ContextPtr                  ctx;         // shared context
    SimFunction *               fni = nullptr;
    das_map<string, float *>    variables;   // pointers into context global data
};

// Compile the expression into a tiny daslang program.
// Each variable becomes a global "var name = 0.0f".
// A single [export] function "eval" returns the expression result.
ExprCalc::ExprCalc(const string & expr, const ExprVars & vars) {
    TextWriter ss;
    // gen2 syntax — use options gen2 and require math for trig/sqrt/etc.
    ss << "options gen2\n";
    ss << "require math\n\n";
    // Declare each variable as a module-level global.
    for (auto & v : vars) {
        ss << "var " << v.first << " = 0.0f\n";
    }
    // The evaluator function simply returns the expression.
    ss << "\n[export]\n"
       << "def eval() : float {\n"
       << "    return " << expr << "\n"
       << "}\n"
       << "\n";
    string text = ss.str();

    // --- Register the source string as a virtual file ---
    //
    // TextFileInfo wraps a raw character buffer.  setFileInfo tells the
    // file-access layer that "expr.das" resolves to this buffer instead
    // of a real file on disk.
    TextPrinter tout;
    auto fAccess = make_smart<FsFileAccess>();
    auto fileInfo = make_unique<TextFileInfo>(text.c_str(),
                                             uint32_t(text.length()), false);
    fAccess->setFileInfo("expr.das", das::move(fileInfo));

    // --- Compile the virtual file ---
    ModuleGroup dummyLibGroup;
    auto program = compileDaScript("expr.das", fAccess, tout, dummyLibGroup);
    if (program->failed()) {
        tout << "Compilation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // --- Simulate into a shared context ---
    ctx = make_shared<Context>(program->getContextStackSize());
    if (!program->simulate(*ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // --- Look up the function ---
    fni = ctx->findFunction("eval");

    // --- Map each variable name to its address in context memory ---
    //
    // findVariable returns -1 if not found; getVariable returns a raw
    // pointer into the context's global-variable segment.
    for (auto & v : vars) {
        auto idx = ctx->findVariable(v.first.c_str());
        if (idx != -1) {
            variables[v.first] = (float *)ctx->getVariable(idx);
        }
    }
}

// Set variables and evaluate the expression.
float ExprCalc::compute(ExprVars & vars) {
    if (!fni) return 0.0f;
    // Poke new values directly into context global memory.
    for (auto & v : vars) {
        auto it = variables.find(v.first);
        if (it != variables.end()) {
            *(it->second) = v.second;
        }
    }
    auto res = ctx->evalWithCatch(fni, nullptr);
    if (auto ex = ctx->getException()) {
        TextPrinter tout;
        tout << "Exception: " << ex << "\n";
        return 0.0f;
    }
    return cast<float>::to(res);
}

// ---------------------------------------------------------------------------
// main — demonstrate the expression calculator
// ---------------------------------------------------------------------------

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();

    TextPrinter tout;

    // Define the variables used in the expression.
    ExprVars variables;
    variables["a"] = 1.0f;
    variables["b"] = 2.0f;
    variables["c"] = 3.0f;

    // Build a calculator for "a * b + c".
    ExprCalc calc("a * b + c", variables);

    // Verify initial evaluation: 1*2+3 = 5.
    float result = calc.compute(variables);
    tout << "a=1 b=2 c=3  =>  a*b+c = " << result << "\n";

    // Sweep through values and verify correctness.
    int checks = 0;
    for (float a = 1.0f; a < 4.0f; a += 1.0f) {
        variables["a"] = a;
        for (float b = 1.0f; b < 5.0f; b += 1.0f) {
            variables["b"] = b;
            for (float c = 1.0f; c < 6.0f; c += 1.0f) {
                variables["c"] = c;
                float expected = a * b + c;
                float got      = calc.compute(variables);
                if (expected != got) {
                    tout << "MISMATCH: a=" << a << " b=" << b
                         << " c=" << c << " expected=" << expected
                         << " got=" << got << "\n";
                    Module::Shutdown();
                    return 1;
                }
                checks++;
            }
        }
    }

    tout << "All " << checks << " evaluations correct\n";

    // Build a second calculator using math functions.
    ExprVars trigVars;
    trigVars["x"] = 0.0f;
    ExprCalc sinCalc("sin(x) + cos(x)", trigVars);

    trigVars["x"] = 0.0f;
    tout << "sin(0)+cos(0) = " << sinCalc.compute(trigVars) << "\n";

    trigVars["x"] = 3.14159265f;
    tout << "sin(pi)+cos(pi) = " << sinCalc.compute(trigVars) << "\n";

    Module::Shutdown();
    return 0;
}
