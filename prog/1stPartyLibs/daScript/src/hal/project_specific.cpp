#include "daScript/misc/platform.h"
#include "daScript/daScriptModule.h"

using namespace das;

void require_project_specific_modules() {

    #if defined(_EMSCRIPTEN_VER)
    return;
    #else
    NEED_MODULE(Module_UnitTest);
    #endif
}
