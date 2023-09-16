/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_SYSTEMLIB_H_
#define _SQSTD_SYSTEMLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQRESULT sqstd_register_command_line_args(HSQUIRRELVM v, int argc, char ** argv);
SQUIRREL_API SQRESULT sqstd_register_systemlib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _SQSTD_SYSTEMLIB_H_ */
