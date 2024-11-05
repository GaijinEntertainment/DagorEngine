//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct be_node;

be_node *yup_load(const char *yup_path);
void yup_free(be_node *node);

// keypath is '/' separated
be_node *yup_get(be_node *yup, const char *keypath);
const char *yup_get_str(be_node *yup, const char *keypath);
const long long *yup_get_int(be_node *yup, const char *keypath);
