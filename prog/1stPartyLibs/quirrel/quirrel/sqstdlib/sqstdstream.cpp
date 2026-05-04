/* see copyright notice in squirrel.h */
#include <squirrel/sqpcheader.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"
#include "sqstdserialization.h"
#include <squirrel/sqvm.h>

#define SETUP_STREAM(v) \
    SQStream *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)((SQUnsignedInteger)SQSTD_STREAM_TYPE_TAG)))) \
        return sq_throwerror(v,"invalid type tag"); \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,"the stream is invalid");

SQInteger _stream_readblob(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQUserPointer data,blobp;
    SQInteger size,res;
    sq_getinteger(v,2,&size);
    if(size > self->Len()) {
        size = self->Len();
    }
    data = sq_getscratchpad(v,size);
    res = self->Read(data,size);
    if(res <= 0)
        return sq_throwerror(v,"no data left to read");
    blobp = sqstd_createblob(v,res);
    memcpy(blobp,data,res);
    return 1;
}

#define SAFE_READN(ptr,len) { \
    if(self->Read(ptr,len) != len) return sq_throwerror(v,"io error"); \
    }
SQInteger _stream_readn(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger format;
    sq_getinteger(v, 2, &format);
    switch(format) {
    case 'l': {
        SQInteger i;
        SAFE_READN(&i, sizeof(i));
        sq_pushinteger(v, i);
              }
        break;
    case 'i': {
        SQInt32 i;
        SAFE_READN(&i, sizeof(i));
        sq_pushinteger(v, i);
              }
        break;
    case 's': {
        short s;
        SAFE_READN(&s, sizeof(short));
        sq_pushinteger(v, s);
              }
        break;
    case 'w': {
        unsigned short w;
        SAFE_READN(&w, sizeof(unsigned short));
        sq_pushinteger(v, w);
              }
        break;
    case 'c': {
        signed char c;
        SAFE_READN(&c, sizeof(signed char));
        sq_pushinteger(v, c);
              }
        break;
    case 'b': {
        unsigned char c;
        SAFE_READN(&c, sizeof(unsigned char));
        sq_pushinteger(v, c);
              }
        break;
    case 'f': {
        float f;
        SAFE_READN(&f, sizeof(float));
        sq_pushfloat(v, f);
              }
        break;
    case 'd': {
        double d;
        SAFE_READN(&d, sizeof(double));
        sq_pushfloat(v, (SQFloat)d);
              }
        break;
    default:
        return sq_throwerror(v, "invalid format");
    }
    return 1;
}

SQInteger _stream_writeblob(HSQUIRRELVM v)
{
    SQUserPointer data;
    SQInteger size;
    SETUP_STREAM(v);
    if(SQ_FAILED(sqstd_getblob(v,2,&data)))
        return sq_throwerror(v,"invalid parameter");
    size = sqstd_getblobsize(v,2);
    if(self->Write(data,size) != size)
        return sq_throwerror(v,"io error");
    sq_pushinteger(v,size);
    return 1;
}

SQInteger _stream_writestring(HSQUIRRELVM v)
{
    const char * str;
    SQInteger len;
    SETUP_STREAM(v);
    if (SQ_FAILED(sq_getstringandsize(v, 2, &str, &len)))
        return sq_throwerror(v, "invalid parameter");

    SQInteger size = len * sizeof(char);
    if (self->Write((void *)str, size) != size)
        return sq_throwerror(v,"io error");
    sq_pushinteger(v, len);
    return 1;
}

SQInteger _stream_writen(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger format, ti;
    SQFloat tf;
    sq_getinteger(v, 3, &format);
    switch(format) {
    case 'l': {
        SQInteger i;
        sq_getinteger(v, 2, &ti);
        i = ti;
        self->Write(&i, sizeof(SQInteger));
              }
        break;
    case 'i': {
        SQInt32 i;
        sq_getinteger(v, 2, &ti);
        i = (SQInt32)ti;
        self->Write(&i, sizeof(SQInt32));
              }
        break;
    case 's': {
        short s;
        sq_getinteger(v, 2, &ti);
        s = (short)ti;
        self->Write(&s, sizeof(short));
              }
        break;
    case 'w': {
        unsigned short w;
        sq_getinteger(v, 2, &ti);
        w = (unsigned short)ti;
        self->Write(&w, sizeof(unsigned short));
              }
        break;
    case 'c': {
        signed char c;
        sq_getinteger(v, 2, &ti);
        c = (signed char)ti;
        self->Write(&c, sizeof(signed char));
                  }
        break;
    case 'b': {
        unsigned char b;
        sq_getinteger(v, 2, &ti);
        b = (unsigned char)ti;
        self->Write(&b, sizeof(unsigned char));
              }
        break;
    case 'f': {
        float f;
        sq_getfloat(v, 2, &tf);
        f = (float)tf;
        self->Write(&f, sizeof(float));
              }
        break;
    case 'd': {
        double d;
        sq_getfloat(v, 2, &tf);
        d = tf;
        self->Write(&d, sizeof(double));
              }
        break;
    default:
        return sq_throwerror(v, "invalid format");
    }
    return 0;
}

SQInteger _stream_seek(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger offset, origin = SQ_SEEK_SET;
    sq_getinteger(v, 2, &offset);
    if(sq_gettop(v) > 2) {
        SQInteger t;
        sq_getinteger(v, 3, &t);
        switch(t) {
            case 'b': origin = SQ_SEEK_SET; break;
            case 'c': origin = SQ_SEEK_CUR; break;
            case 'e': origin = SQ_SEEK_END; break;
            default: return sq_throwerror(v,"invalid origin");
        }
    }
    sq_pushinteger(v, self->Seek(offset, origin));
    return 1;
}

SQInteger _stream_tell(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    sq_pushinteger(v, self->Tell());
    return 1;
}

SQInteger _stream_len(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    sq_pushinteger(v, self->Len());
    return 1;
}

SQInteger _stream_flush(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    if(!self->Flush())
        sq_pushinteger(v, 1);
    else
        sq_pushnull(v);
    return 1;
}

SQInteger _stream_eos(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    if(self->EOS())
        sq_pushinteger(v, 1);
    else
        sq_pushnull(v);
    return 1;
}


SQInteger _stream_writeobject(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQObjectPtr obj = stack_get(v, 2);
    SQObjectPtr availableClasses;
    if (sq_gettop(v) > 2)
        availableClasses = stack_get(v, 3);

    SQRESULT res = sqstd_serialize_object_to_stream(v, self, obj, availableClasses);
    return SQ_FAILED(res) ? res : 0;
}

SQInteger _stream_readobject(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQObjectPtr availableClasses;
    if (sq_gettop(v) > 1)
        availableClasses = stack_get(v, 2);
    SQRESULT res = sqstd_deserialize_object_from_stream(v, self, availableClasses);
    return SQ_FAILED(res) ? res : 1;
}


SQInteger _stream__cloned(HSQUIRRELVM v)
{
    return sq_throwerror(v,"this object cannot be cloned");
}

static const SQRegFunctionFromStr _stream_methods[] = {
    { _stream_readblob,    "instance.readblob(size: int): instance",         "Reads up to size bytes and returns them as a blob" },
    { _stream_readn,       "instance.readn(format: int): number",            "Reads a value of the given numeric format and returns it" },
    { _stream_writeblob,   "instance.writeblob(blob: instance): int",        "Writes the given blob and returns the number of bytes written" },
    { _stream_writestring, "instance.writestring(str: string): int",         "Writes the string and returns the number of characters written" },
    { _stream_writen,      "instance.writen(value: number, format: int)",    "Writes a numeric value in the given format" },
    { _stream_seek,        "instance.seek(offset: int, [origin: int]): int", "Seeks to the given offset; origin is 'b' (begin), 'c' (current) or 'e' (end)" },
    { _stream_tell,        "instance.tell(): int",                           "Returns the current stream position" },
    { _stream_len,         "instance.len(): int",                            "Returns the stream length" },
    { _stream_eos,         "instance.eos(): int|null",                       "Returns non-null if the stream is at end-of-stream" },
    { _stream_flush,       "instance.flush(): int|null",                     "Flushes the stream and returns non-null on success" },
    { _stream_writeobject, "instance.writeobject(obj, [classes: table|null])",    "Serializes the object to the stream" },
    { _stream_readobject,  "instance.readobject([classes: table|null]): any",     "Deserializes an object from the stream" },
    { _stream__cloned,     "instance._cloned(other)",                        "Stream cloning is not supported" },
    { NULL, NULL, NULL }
};

SQRESULT sqstd_init_streamclass(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);
    sq_pushstring(v,"std_stream",-1);
    if(SQ_FAILED(sq_get(v,-2))) {
        sq_pushstring(v,"std_stream",-1);
        sq_newclass(v,SQFalse);
        sq_settypetag(v,-1,(SQUserPointer)((SQUnsignedInteger)SQSTD_STREAM_TYPE_TAG));
        SQInteger i = 0;
        while(_stream_methods[i].f) {
            sq_new_closure_slot_from_decl_string(v, _stream_methods[i].f, 0, _stream_methods[i].declstring, _stream_methods[i].docstring);
            i++;
        }
        sq_newslot(v,-3,SQFalse); // put to registry table

        sq_pushstring(v,"stream",-1);
        sq_pushstring(v,"std_stream",-1);
        sq_get(v,-3);
        sq_newslot(v,-4,SQFalse); // put to destination table (and name the class 'stream')
    }
    else {
        sq_pop(v,1); //result
    }
    sq_pop(v,1);
    return SQ_OK;
}

SQRESULT declare_stream(HSQUIRRELVM v,const char* name,SQUserPointer typetag,const char* reg_name,const SQRegFunctionFromStr *methods,const SQRegFunctionFromStr *globals)
{
    if(sq_gettype(v,-1) != OT_TABLE)
        return sq_throwerror(v,"table expected");
    SQInteger top = sq_gettop(v);
    //create base stream class
    sqstd_init_streamclass(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,reg_name,-1);
    sq_pushstring(v,"std_stream",-1);
    if(SQ_SUCCEEDED(sq_get(v,-3))) {
        sq_newclass(v,SQTrue);
        sq_settypetag(v,-1,typetag);
        SQInteger i = 0;
        while(methods[i].f) {
            sq_new_closure_slot_from_decl_string(v, methods[i].f, 0, methods[i].declstring, methods[i].docstring);
            i++;
        }
        sq_newslot(v,-3,SQFalse);
        sq_pop(v,1);

        i = 0;
        while(globals[i].f) {
            sq_new_closure_slot_from_decl_string(v, globals[i].f, 0, globals[i].declstring, globals[i].docstring);
            i++;
        }
        //register the class in the target table
        sq_pushstring(v,name,-1);
        sq_pushregistrytable(v);
        sq_pushstring(v,reg_name,-1);
        sq_get(v,-2);
        sq_remove(v,-2);
        sq_newslot(v,-3,SQFalse);

        sq_settop(v,top);
        return SQ_OK;
    }
    sq_settop(v,top);
    return SQ_ERROR;
}
