#include "daScript/daScript.h"

using namespace das;

const char * tutorial_text = R""""(
[export]
def test
    print("this is nano tutorial\n")
)"""";

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // Initialize modules
    Module::Initialize();
    // make file access, introduce string as if it was a file
    auto fAccess = make_smart<FsFileAccess>();
    auto fileInfo = make_unique<TextFileInfo>(tutorial_text, uint32_t(strlen(tutorial_text)), false);
    fAccess->setFileInfo("dummy.das", das::move(fileInfo));
    // compile script
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto program = compileDaScript("dummy.das", fAccess, tout, dummyLibGroup);
    if ( program->failed() ) return -1;
    // create context
    Context ctx(program->getContextStackSize());
    if ( !program->simulate(ctx, tout) ) return -2;
    // find function. its up to application to check, if function is not null
    auto function = ctx.findFunction("test");
    if ( !function ) return -3;
    // call context function
    ctx.evalWithCatch(function, nullptr);
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}
