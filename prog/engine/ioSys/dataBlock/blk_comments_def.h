// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// comment param/block name is encoded as 3 characters (2 bytes prefix and 1 byte suffix)
// this encoding helps minimizing impact of performance
#define COMMENT_NAME_PREFIX     "@\xC"
#define COMMENT_NAME_PREFIX_LEN 2

#define COMMENT_PRE_SUFFIX_CPP  "\x1"  // CPP-style comment before commented statement
#define COMMENT_PRE_SUFFIX_C    "\x2"  // C-style comment before commented statement
#define COMMENT_POST_SUFFIX_CPP "\x11" // CPP-style comment after commented statement (same line)
#define COMMENT_POST_SUFFIX_C   "\x12" // C-style comment after commented statement (same line)

#define COMMENT_PRE_CPP  COMMENT_NAME_PREFIX COMMENT_PRE_SUFFIX_CPP
#define COMMENT_PRE_C    COMMENT_NAME_PREFIX COMMENT_PRE_SUFFIX_C
#define COMMENT_POST_CPP COMMENT_NAME_PREFIX COMMENT_POST_SUFFIX_CPP
#define COMMENT_POST_C   COMMENT_NAME_PREFIX COMMENT_POST_SUFFIX_C

#define CHECK_COMMENT_PREFIX(NM) strncmp(NM, COMMENT_NAME_PREFIX, COMMENT_NAME_PREFIX_LEN) == 0

#define IS_COMMENT_PRE_CPP(NM) ((NM)[2] == *COMMENT_PRE_SUFFIX_CPP)
#define IS_COMMENT_PRE_C(NM)   ((NM)[2] == *COMMENT_PRE_SUFFIX_C)
#define IS_COMMENT_POST(NM)    (((NM)[2] == *COMMENT_POST_SUFFIX_CPP) || ((NM)[2] == *COMMENT_POST_SUFFIX_C))
#define IS_COMMENT_CPP(NM)     (((NM)[2] == *COMMENT_POST_SUFFIX_CPP) || ((NM)[2] == *COMMENT_PRE_SUFFIX_CPP))
#define IS_COMMENT_C(NM)       (((NM)[2] == *COMMENT_POST_SUFFIX_C) || ((NM)[2] == *COMMENT_PRE_SUFFIX_C))
