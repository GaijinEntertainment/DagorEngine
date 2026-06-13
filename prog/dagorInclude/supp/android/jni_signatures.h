//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#define JNI_OBJECT(name)         "L" name ";"
#define JNI_STRING               JNI_OBJECT("java/lang/String")
#define JNI_VOID                 "V"
#define JNI_BOOL                 "Z"
#define JNI_BYTE                 "B"
#define JNI_CHAR                 "C"
#define JNI_SHORT                "S"
#define JNI_INT                  "I"
#define JNI_LONG                 "J"
#define JNI_FLOAT                "F"
#define JNI_DOUBLE               "D"
#define JNI_ARRAY(type)          "[" type
#define JNI_SIGNATURE(ret, args) "(" args ")" ret
