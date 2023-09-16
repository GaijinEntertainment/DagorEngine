#pragma once

#include <squirrel.h>

SQInteger _sq_string_strip_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_lstrip_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_rstrip_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_split_by_chars_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_escape_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_startswith_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
SQInteger _sq_string_endswith_impl(HSQUIRRELVM v, SQInteger arg_stack_start);
