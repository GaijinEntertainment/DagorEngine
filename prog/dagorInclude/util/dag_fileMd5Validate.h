//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

KRNLIMP bool validate_file_md5_hash(const char *fname, const char *md5, const char *logerr_prefix);

#include <supp/dag_undef_COREIMP.h>
