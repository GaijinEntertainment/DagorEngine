#include <supp/dag_null.h>

extern int __cdecl dagor_memory_real_init(void);
typedef int(__cdecl *_PIFV)(void);

#define SEG ".CRT$XIB"
#pragma section(SEG, long, read)
__declspec(allocate(SEG)) _PIFV __c_dagor_memory_real_init = dagor_memory_real_init;
