#include "daScript/daScript.h"

using namespace das;

extern int das_aot_main ( int argc, char * argv[] );

int main(int argc, char * argv[]) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // call original aot das_aot_main from daScript
    return das_aot_main(argc,argv);
}
