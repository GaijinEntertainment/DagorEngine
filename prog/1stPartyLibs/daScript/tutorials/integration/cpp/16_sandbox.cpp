// Tutorial 16 — Sandbox (C++ integration)
//
// Demonstrates how to restrict what daslang scripts can do:
//   - CodeOfPolicies — compile-time language restrictions
//   - Memory limits — heap and stack size caps
//   - Custom FileAccess — restrict which modules can be required
//   - Subclassing FileAccess virtual methods for fine-grained control
//   - .das_project — policy file written in daslang itself
//
// Two sandboxing approaches are shown:
//   A) C++ subclass of FileAccess (Demos 1–4) — policies defined in code
//   B) .das_project file (Demos 5–6) — policies defined in a daslang file
//
// Sandboxing is essential when running untrusted scripts (user mods,
// plugin systems, online code playgrounds).

#include "daScript/daScript.h"

using namespace das;

// -----------------------------------------------------------------------
// Custom file access — restrict which modules scripts can require
// -----------------------------------------------------------------------

class SandboxFileAccess : public FsFileAccess {
    das_set<string> allowedModules;
public:
    SandboxFileAccess() {
        // Only allow these modules to be required
        allowedModules.insert("$");        // built-in module (always needed)
        allowedModules.insert("math");
        allowedModules.insert("strings");
        allowedModules.insert("rtti");
    }

    // Called for each "require" — return false to block the module
    virtual bool isModuleAllowed(const string & mod,
                                 const string &) const override {
        bool allowed = allowedModules.count(mod) > 0;
        if (!allowed) {
            printf("  [sandbox] BLOCKED module: %s\n", mod.c_str());
        }
        return allowed;
    }

    // No module can use unsafe blocks
    virtual bool canModuleBeUnsafe(const string &,
                                   const string &) const override {
        return false;
    }

    // Block dangerous options
    virtual bool isOptionAllowed(const string & opt,
                                 const string &) const override {
        if (opt == "persistent_heap") return false;
        if (opt == "no_global_heap") return false;
        return true;
    }
};

// -----------------------------------------------------------------------
// Helper: compile + simulate + run a script with given policies
// -----------------------------------------------------------------------

static bool run_script(const char * label,
                       const string & scriptPath,
                       const FileAccessPtr & fAccess,
                       CodeOfPolicies policies,
                       TextPrinter & tout) {
    tout << "--- " << label << " ---\n";

    ModuleGroup dummyLibGroup;
    auto program = compileDaScript(scriptPath, fAccess, tout,
                                   dummyLibGroup, policies);
    if (program->failed()) {
        tout << "  Compilation FAILED (expected in sandbox demo):\n";
        int shown = 0;
        for (auto & err : program->errors) {
            if (shown++ >= 3) {
                tout << "  ... and " << (int(program->errors.size()) - 3)
                     << " more error(s)\n";
                break;
            }
            tout << "  " << reportError(err.at, err.what, err.extra,
                                        err.fixme, err.cerr);
        }
        return false;
    }

    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "  Simulation failed\n";
        return false;
    }

    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "  Function 'test' not found\n";
        return false;
    }

    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "  Exception: " << ex << "\n";
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------
// Script that uses unsafe — should be blocked by sandbox
// -----------------------------------------------------------------------

static const char * UNSAFE_SCRIPT = R"(
options gen2
[export]
def test() {
    unsafe {
        print("This should not run!\n")
    }
}
)";

// -----------------------------------------------------------------------
// Script that tries to require a blocked module
// -----------------------------------------------------------------------

static const char * BLOCKED_MODULE_SCRIPT = R"(
options gen2
require daslib/json
[export]
def test() {
    print("This should not run!\n")
}
)";

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/16_sandbox.das"

void tutorial() {
    TextPrinter tout;
    string scriptPath = getDasRoot() + SCRIPT_NAME;

    // ================================================================
    // Demo 1: No restrictions (normal compilation)
    // ================================================================
    tout << "=== Demo 1: No restrictions ===\n";
    {
        auto fAccess = make_smart<FsFileAccess>();
        CodeOfPolicies policies;
        run_script("Normal mode", scriptPath, fAccess, policies, tout);
    }

    // ================================================================
    // Demo 2: CodeOfPolicies restrictions
    // ================================================================
    tout << "\n=== Demo 2: Policies — no_unsafe ===\n";
    {
        auto fAccess = make_smart<FsFileAccess>();

        // Policy: forbid unsafe blocks
        CodeOfPolicies policies;
        policies.no_unsafe = true;

        // The safe script should compile fine
        run_script("Safe script with no_unsafe", scriptPath,
                   fAccess, policies, tout);

        // An unsafe script should fail to compile
        tout << "\n";
        auto fAccessUnsafe = make_smart<FsFileAccess>();
        fAccessUnsafe->setFileInfo("unsafe_test.das",
            make_unique<TextFileInfo>(UNSAFE_SCRIPT,
                                     uint32_t(strlen(UNSAFE_SCRIPT)),
                                     false));
        run_script("Unsafe script with no_unsafe",
                   "unsafe_test.das", fAccessUnsafe, policies, tout);
    }

    // ================================================================
    // Demo 3: Memory limits
    // ================================================================
    tout << "\n=== Demo 3: Memory limits ===\n";
    {
        auto fAccess = make_smart<FsFileAccess>();

        CodeOfPolicies policies;
        policies.stack = 8 * 1024;                    // 8 KB stack
        policies.max_heap_allocated = 1024 * 1024;    // 1 MB heap
        policies.max_string_heap_allocated = 256*1024; // 256 KB strings

        tout << "  Stack:       " << policies.stack << " bytes\n";
        tout << "  Max heap:    " << policies.max_heap_allocated
             << " bytes\n";
        tout << "  Max strings: " << policies.max_string_heap_allocated
             << " bytes\n";

        run_script("Memory-limited", scriptPath, fAccess, policies, tout);
    }

    // ================================================================
    // Demo 4: Custom FileAccess — module restrictions
    // ================================================================
    tout << "\n=== Demo 4: Module restrictions ===\n";
    {
        auto fAccess = make_smart<SandboxFileAccess>();

        CodeOfPolicies policies;
        policies.no_unsafe = true;

        // Our safe script doesn't require any blocked modules
        run_script("Allowed script", scriptPath, fAccess, policies, tout);

        // A script that requires a blocked module should fail
        tout << "\n";
        auto fAccessBlocked = make_smart<SandboxFileAccess>();
        fAccessBlocked->setFileInfo("blocked_test.das",
            make_unique<TextFileInfo>(BLOCKED_MODULE_SCRIPT,
                                     uint32_t(strlen(BLOCKED_MODULE_SCRIPT)),
                                     false));
        run_script("Blocked module script",
                   "blocked_test.das", fAccessBlocked, policies, tout);
    }

    // ================================================================
    // Demo 5: .das_project — sandbox via policy file
    //
    // Instead of subclassing FileAccess in C++, you can write the
    // sandbox policy in daslang itself.  A .das_project file is a
    // regular .das script that exports callback functions:
    //   module_get()           — resolve require paths (REQUIRED)
    //   module_allowed()       — whitelist loadable modules
    //   module_allowed_unsafe()— control unsafe blocks
    //   option_allowed()       — whitelist options directives
    //   annotation_allowed()   — whitelist annotations
    //
    // The host loads it via:
    //   FsFileAccess(projectPath, make_smart<FsFileAccess>())
    // which compiles the project file, simulates it, and looks up
    // the exported callbacks by name.
    // ================================================================
    tout << "\n=== Demo 5: .das_project sandbox ===\n";
    {
        string projectPath = getDasRoot()
            + "/tutorials/integration/cpp/16_sandbox.das_project";

        auto fAccess = make_smart<FsFileAccess>(projectPath,
                                                make_smart<FsFileAccess>());

        CodeOfPolicies policies;
        // The .das_project handles module/unsafe/option restrictions,
        // but we can still layer CodeOfPolicies on top
        run_script("Script under .das_project sandbox",
                   scriptPath, fAccess, policies, tout);
    }

    // ================================================================
    // Demo 6: .das_project blocks violations
    // ================================================================
    tout << "\n=== Demo 6: .das_project blocks violations ===\n";
    {
        string projectPath = getDasRoot()
            + "/tutorials/integration/cpp/16_sandbox.das_project";

        // Try a script that uses unsafe — blocked by module_allowed_unsafe()
        {
            auto fAccess = make_smart<FsFileAccess>(projectPath,
                                                    make_smart<FsFileAccess>());
            fAccess->setFileInfo("unsafe_test.das",
                make_unique<TextFileInfo>(UNSAFE_SCRIPT,
                                         uint32_t(strlen(UNSAFE_SCRIPT)),
                                         false));
            CodeOfPolicies policies;
            run_script("Unsafe script under .das_project",
                       "unsafe_test.das", fAccess, policies, tout);
        }

        // Try a script that requires a blocked module
        tout << "\n";
        {
            auto fAccess = make_smart<FsFileAccess>(projectPath,
                                                    make_smart<FsFileAccess>());
            fAccess->setFileInfo("blocked_test.das",
                make_unique<TextFileInfo>(BLOCKED_MODULE_SCRIPT,
                                         uint32_t(strlen(BLOCKED_MODULE_SCRIPT)),
                                         false));
            CodeOfPolicies policies;
            run_script("Blocked module under .das_project",
                       "blocked_test.das", fAccess, policies, tout);
        }
    }

    // ================================================================
    // Summary of sandbox policies
    // ================================================================
    tout << "\n=== Available sandbox approaches ===\n";
    tout << "  Approach A — C++ (CodeOfPolicies + FileAccess subclass):\n";
    tout << "    CodeOfPolicies:\n";
    tout << "      no_unsafe              — forbid unsafe blocks\n";
    tout << "      no_global_variables    — forbid module-scope var\n";
    tout << "      no_global_heap         — forbid heap from globals\n";
    tout << "      no_init                — forbid [init] functions\n";
    tout << "      max_heap_allocated     — cap heap memory\n";
    tout << "      max_string_heap_allocated — cap string memory\n";
    tout << "      stack                  — set stack size\n";
    tout << "      threadlock_context     — thread-safe context\n";
    tout << "    FileAccess overrides:\n";
    tout << "      isModuleAllowed()      — block specific modules\n";
    tout << "      canModuleBeUnsafe()    — prevent unsafe in modules\n";
    tout << "      isOptionAllowed()      — block script options\n";
    tout << "      isAnnotationAllowed()  — block annotations\n";
    tout << "  Approach B — .das_project file:\n";
    tout << "    module_get()             — resolve require paths\n";
    tout << "    module_allowed()         — whitelist modules\n";
    tout << "    module_allowed_unsafe()  — control unsafe blocks\n";
    tout << "    option_allowed()         — whitelist options\n";
    tout << "    annotation_allowed()     — whitelist annotations\n";
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
