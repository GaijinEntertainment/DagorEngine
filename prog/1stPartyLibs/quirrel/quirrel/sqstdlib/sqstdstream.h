/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

SQInteger _stream_readblob(HSQUIRRELVM v);
SQInteger _stream_readline(HSQUIRRELVM v);
SQInteger _stream_readn(HSQUIRRELVM v);
SQInteger _stream_writeblob(HSQUIRRELVM v);
SQInteger _stream_writestring(HSQUIRRELVM v);
SQInteger _stream_writen(HSQUIRRELVM v);
SQInteger _stream_seek(HSQUIRRELVM v);
SQInteger _stream_tell(HSQUIRRELVM v);
SQInteger _stream_len(HSQUIRRELVM v);
SQInteger _stream_eos(HSQUIRRELVM v);
SQInteger _stream_flush(HSQUIRRELVM v);
SQInteger _stream_writeobject(HSQUIRRELVM v);
SQInteger _stream_readobject(HSQUIRRELVM v);

SQRESULT declare_stream(HSQUIRRELVM v,const char* name,SQUserPointer typetag,const char* reg_name,const SQRegFunctionFromStr *methods,const SQRegFunctionFromStr *globals);
#endif /*_SQSTD_STREAM_H_*/
