#include <util/dag_hierBitMemPool.h>

void *hierbit_allocmem(int sz) { return memalloc(sz, midmem); }
void hierbit_freemem(void *p, int /*sz*/) { memfree(p, midmem); }

#define EXPORT_PULL dll_pull_baseutil_hierBitMem
#include <supp/exportPull.h>
