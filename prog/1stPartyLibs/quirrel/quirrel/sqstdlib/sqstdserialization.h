/*  see copyright notice in squirrel.h */
#pragma once
#include <squirrel/sqpcheader.h>

SQRESULT sqstd_serialize_object_to_stream(HSQUIRRELVM vm, SQStream *dest, SQObjectPtr obj, SQObjectPtr available_classes);
SQRESULT sqstd_deserialize_object_from_stream(HSQUIRRELVM vm, SQStream *src, SQObjectPtr available_classes);
