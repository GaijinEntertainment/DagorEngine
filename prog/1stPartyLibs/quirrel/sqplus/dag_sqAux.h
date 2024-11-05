//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
#include <string.h>


typedef void(*dag_sq_error_handler_t)(HSQUIRRELVM vm, const char *err, const char *src_name, int line);

void sqdagor_seterrorhandlers(HSQUIRRELVM v);
SQInteger sqdagor_formatcallstack(HSQUIRRELVM v, SQChar *out_buf, SQInteger out_buf_size, bool is_short_form = false);
void sqdagor_printcallstack(HSQUIRRELVM v, bool is_short_form = false);

void sqdagor_get_exception_source_and_line(HSQUIRRELVM v, const char** src, int& line);

void sqdagor_override_error_handler(SQVM *vm, dag_sq_error_handler_t handler, char *buf, size_t buf_size);
bool dag_sq_is_errhandler_overriden(HSQUIRRELVM v);
