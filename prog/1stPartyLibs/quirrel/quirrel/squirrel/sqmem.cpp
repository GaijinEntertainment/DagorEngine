/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"

void sq_vm_init_alloc_context(SQAllocContext *) {}
void sq_vm_destroy_alloc_context(SQAllocContext *) {}
void sq_vm_assign_to_alloc_context(SQAllocContext, HSQUIRRELVM) {}

void *sq_vm_malloc(SQAllocContext SQ_UNUSED_ARG(ctx), SQUnsignedInteger size){ return malloc(size); }

void *sq_vm_realloc(SQAllocContext SQ_UNUSED_ARG(ctx), void *p, SQUnsignedInteger SQ_UNUSED_ARG(oldsize), SQUnsignedInteger size){ return realloc(p, size); }

void sq_vm_free(SQAllocContext SQ_UNUSED_ARG(ctx), void *p, SQUnsignedInteger SQ_UNUSED_ARG(size)){ free(p); }
