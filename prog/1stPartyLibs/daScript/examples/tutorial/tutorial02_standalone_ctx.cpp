#include "daScript/daScript.h"
#include "_standalone_ctx_generated/tutorial02.das.h"

int main( int, char * [] ) {
    auto ctx = das::tutorial02::Standalone();
    ctx.test();
    return 0;
}
