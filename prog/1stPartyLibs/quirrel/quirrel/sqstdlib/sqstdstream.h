/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

SQRESULT declare_stream(HSQUIRRELVM v,const char* name,SQUserPointer typetag,const char* reg_name,const SQRegFunctionFromStr *methods,const SQRegFunctionFromStr *globals);

#endif /*_SQSTD_STREAM_H_*/
