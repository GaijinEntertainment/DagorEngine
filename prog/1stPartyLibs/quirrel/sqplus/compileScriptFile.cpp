#include "sqplus.h"
#include <sqstdio.h>

bool SquirrelVM::CompileScript(const SQChar *s, SquirrelObject& outObject)
{
#define MAX_EXPANDED_PATH 1023
  if(SQ_SUCCEEDED(sqstd_loadfile(_VM,s,1))) {
    outObject.AttachToStackObject(-1);
    sq_pop(_VM,1);
    return true;
  }
  return false;
}
