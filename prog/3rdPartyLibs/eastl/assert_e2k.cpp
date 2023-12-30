#include <signal.h>
#include <unistd.h>

void EASTL_DEBUG_BREAK() { kill(getpid(), SIGINT); }
