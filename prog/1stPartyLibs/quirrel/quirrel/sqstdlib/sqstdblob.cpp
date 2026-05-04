/* see copyright notice in squirrel.h */
#include <new>
#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>
#include <sqstdblob.h>
#include <sqstdaux.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

#define SQSTD_BLOB_TYPE_TAG ((SQUnsignedInteger)(SQSTD_STREAM_TYPE_TAG | 0x00000002))

//Blob


#define SETUP_BLOB(v) \
    SQBlob *self = NULL; \
    { if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) \
        return sq_throwerror(v,"invalid type tag");  } \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,"the blob is invalid");


static SQInteger _blob_resize(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger size;
    sq_getinteger(v,2,&size);
    if(!self->Resize(size))
        return sqstd_throwerrorf(v,"resize failed, cur size=%d, requested=%d", int(self->Len()), int(size));
    return 0;
}

static void __swap_dword(unsigned int *n)
{
    *n=(unsigned int)(((*n&0xFF000000)>>24)  |
            ((*n&0x00FF0000)>>8)  |
            ((*n&0x0000FF00)<<8)  |
            ((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
    *n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static SQInteger _blob_swap4(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger num=(self->Len()-(self->Len()%4))>>2;
    unsigned int *t=(unsigned int *)self->GetBuf();
    for(SQInteger i = 0; i < num; i++) {
        __swap_dword(&t[i]);
    }
    return 0;
}

static SQInteger _blob_swap2(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger num=(self->Len()-(self->Len()%2))>>1;
    unsigned short *t = (unsigned short *)self->GetBuf();
    for(SQInteger i = 0; i < num; i++) {
        __swap_word(&t[i]);
    }
    return 0;
}

static SQInteger _blob__set(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger idx,val;
    sq_getinteger(v,2,&idx);
    sq_getinteger(v,3,&val);
    if(idx < 0 || idx >= self->Len())
        return sq_throwerror(v,"index out of range");
    ((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
    sq_push(v,3);
    return 1;
}

static SQInteger _blob__get(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger idx;

    if ((sq_gettype(v, 2) & SQOBJECT_NUMERIC) == 0)
    {
        sq_pushnull(v);
        return sq_throwobject(v);
    }
    sq_getinteger(v,2,&idx);
    if(idx < 0 || idx >= self->Len())
        return sq_throwerror(v,"index out of range");
    sq_pushinteger(v,((unsigned char *)self->GetBuf())[idx]);
    return 1;
}

static SQInteger _blob__nexti(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    if(sq_gettype(v,2) == OT_NULL) {
        sq_pushinteger(v, 0);
        return 1;
    }
    SQInteger idx;
    if(SQ_SUCCEEDED(sq_getinteger(v, 2, &idx))) {
        if(idx+1 < self->Len()) {
            sq_pushinteger(v, idx+1);
            return 1;
        }
        sq_pushnull(v);
        return 1;
    }
    return sq_throwerror(v,"internal error (_nexti) wrong argument type");
}

static SQInteger _blob__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,"blob",-1);
    return 1;
}

static SQInteger _blob_releasehook(HSQUIRRELVM SQ_UNUSED_ARG(vm), SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQBlob *self = (SQBlob*)p;
    SQAllocContext alloc_ctx = self->_alloc_ctx;
    self->~SQBlob();
    sq_free(alloc_ctx, self, sizeof(SQBlob));
    return 1;
}

static SQInteger _blob_constructor(HSQUIRRELVM v)
{
    SQInteger nparam = sq_gettop(v);
    SQInteger size = 0;
    if(nparam == 2) {
        sq_getinteger(v, 2, &size);
    }
    if(size < 0) return sq_throwerror(v, "cannot create blob with negative size");
    //SQBlob *b = new SQBlob(size);

    SQAllocContext alloc_ctx = sq_getallocctx(v);
    SQBlob *b = new (sq_malloc(alloc_ctx, sizeof(SQBlob)))SQBlob(alloc_ctx, size);
    if(SQ_FAILED(sq_setinstanceup(v,1,b))) {
        b->~SQBlob();
        sq_free(alloc_ctx,b,sizeof(SQBlob));
        return sq_throwerror(v, "cannot create blob");
    }
    sq_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

static SQInteger _blob__cloned(HSQUIRRELVM v)
{
    SQBlob *other = NULL;
    {
        if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&other,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
            return SQ_ERROR;
    }
    //SQBlob *thisone = new SQBlob(other->Len());
    SQAllocContext alloc_ctx = sq_getallocctx(v);
    SQBlob *thisone = new (sq_malloc(alloc_ctx,sizeof(SQBlob))) SQBlob(alloc_ctx, other->Len());
    memcpy(thisone->GetBuf(),other->GetBuf(),thisone->Len());
    if(SQ_FAILED(sq_setinstanceup(v,1,thisone))) {
        thisone->~SQBlob();
        sq_free(alloc_ctx,thisone,sizeof(SQBlob));
        return sq_throwerror(v, "cannot clone blob");
    }
    sq_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

static SQInteger _blob_as_string(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    sq_pushstring(v, (const char *)self->GetBuf(), self->Len() / sizeof(char));
    return 1;
}

static const SQRegFunctionFromStr _blob_methods[] = {
    { _blob_constructor, "constructor([size: int]): instance",      "Creates a blob of the given size (default 0)" },
    { _blob_resize,      "instance.resize(size: int)",              "Resizes the blob to the given size" },
    { _blob_swap2,       "instance.swap2()",                        "Byte-swaps the blob contents as an array of 16-bit values" },
    { _blob_swap4,       "instance.swap4()",                        "Byte-swaps the blob contents as an array of 32-bit values" },
    { _blob_as_string,   "instance.as_string(): string",            "Returns the blob contents as a string" },
    { _blob__set,        "instance._set(idx: int, val: int): int",  "Sets the byte at the given index" },
    { _blob__get,        "instance._get(idx: int): int",            "Returns the byte at the given index" },
    { _blob__typeof,     "instance._typeof(): string",              "Returns 'blob'" },
    { _blob__nexti,      "instance._nexti(prev): int|null",         "Iterator support: returns the next index or null" },
    { _blob__cloned,     "instance._cloned(other: instance)",       "Clones the given blob into this instance" },
    { NULL, NULL, NULL }
};



//GLOBAL FUNCTIONS

static SQInteger _g_blob_casti2f(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    sq_pushfloat(v,*((const SQFloat *)&i));
    return 1;
}

static SQInteger _g_blob_castf2i(HSQUIRRELVM v)
{
    SQFloat f;
    sq_getfloat(v,2,&f);
    SQInteger result = 0;
    memcpy(&result, &f, sizeof(f));
    sq_pushinteger(v, result);
    return 1;
}

static SQInteger _g_blob_swap2(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    unsigned short s = (unsigned short)i;
    sq_pushinteger(v, ((s << 8) | ((s >> 8) & 0x00FFu)) & 0xFFFFu);
    return 1;
}

static SQInteger _g_blob_swap4(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    unsigned int t4 = (unsigned int)i;
    __swap_dword(&t4);
    sq_pushinteger(v,(SQInteger)t4);
    return 1;
}

static SQInteger _g_blob_swapfloat(HSQUIRRELVM v)
{
    SQFloat f;
    sq_getfloat(v,2,&f);
    __swap_dword((unsigned int *)&f);
    sq_pushfloat(v,f);
    return 1;
}

static const SQRegFunctionFromStr bloblib_funcs[] = {
    { _g_blob_casti2f,   "pure casti2f(i: int): float",        "Reinterprets the bits of an integer as a float" },
    { _g_blob_castf2i,   "pure castf2i(f: number): int",       "Reinterprets the bits of a float as an integer" },
    { _g_blob_swap2,     "pure swap2(val: number): int",       "Byte-swaps a 16-bit value" },
    { _g_blob_swap4,     "pure swap4(val: number): int",       "Byte-swaps a 32-bit value" },
    { _g_blob_swapfloat, "pure swapfloat(val: number): float", "Byte-swaps the bits of a float" },
    { NULL, NULL, NULL }
};

SQRESULT sqstd_getblob(HSQUIRRELVM v,SQInteger idx,SQUserPointer *ptr)
{
    SQBlob *blob;
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
        return -1;
    *ptr = blob->GetBuf();
    return SQ_OK;
}

SQInteger sqstd_getblobsize(HSQUIRRELVM v,SQInteger idx)
{
    SQBlob *blob;
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
        return -1;
    return blob->Len();
}

SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size)
{
    SQInteger top = sq_gettop(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,"std_blob",-1);
    if(SQ_SUCCEEDED(sq_get(v,-2))) {
        sq_remove(v,-2); //removes the registry
        sq_push(v,1); // push the this
        sq_pushinteger(v,size); //size
        SQBlob *blob = NULL;
        if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQFalse))
            && SQ_SUCCEEDED(sq_getinstanceup(v,-1,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) {
            sq_remove(v,-2);
            return blob->GetBuf();
        }
    }
    sq_settop(v,top);
    return NULL;
}

SQRESULT sqstd_register_bloblib(HSQUIRRELVM v)
{
    return declare_stream(v,"blob",(SQUserPointer)SQSTD_BLOB_TYPE_TAG,"std_blob",_blob_methods,bloblib_funcs);
}

