// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/containers/tab_sort.h>

#include <util/dag_string.h>

#include <string.h>


int tab_sort_strings(const String *a, const String *b) { return strcmp((const char *)*a, (const char *)*b); }


int tab_sort_stringsi(const String *a, const String *b) { return stricmp((const char *)*a, (const char *)*b); }
