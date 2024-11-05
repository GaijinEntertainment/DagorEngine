#include "daScript/daScript.h"

using namespace das;

extern int MAIN_FUNC_NAME ( int, char * argv[] );

int main(int argc, char * argv[]) {
    // request our custom module
    NEED_MODULE(Module_TestProfile);
    // call daScript for main.das
    string dasRoot = getDasRoot() + "/examples/profile/main.das";
    vector<const char *> newArgv;
    newArgv.push_back(argv[0]);
    newArgv.push_back(dasRoot.c_str());
    bool noJIT = false;
    for ( int i=1; i<argc; ++i ) {
        if ( strcmp(argv[i],"-html")==0 ) {
            noJIT = true;
        }
    }
    if ( !noJIT ) {
        newArgv.push_back("-jit");
        newArgv.push_back("--");
    }
    for ( int i=1; i<argc; ++i ) {
        newArgv.push_back(argv[i]);
    }
    return MAIN_FUNC_NAME(newArgv.size(),(char **)newArgv.data());
}
