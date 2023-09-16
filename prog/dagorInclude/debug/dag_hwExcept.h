//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class DagorException;

#include <time.h>
#include <supp/dag_define_COREIMP.h>

namespace DagorHwException
{

struct ProductInfo
{
  ProductInfo()
  {
    buildNumber = -1;
    productName[0] = 0x0;
    version[0] = 0x0;
    startupTime = 0;
    modes = 0;
  }


  int buildNumber;
  char productName[64];
  char version[64];
  time_t startupTime;
  unsigned modes;
};

KRNLIMP int setHandler(const char *thread);
KRNLIMP void removeHandler(int id);
KRNLIMP void reportException(DagorException &caught_except, bool terminate = true, const char *add_dump_fname = 0);

KRNLIMP void forceDump(const char *msg = NULL, const char *file = NULL, int line = 0);

KRNLIMP void registerProductInfo(ProductInfo *info);
KRNLIMP void updateProductInfo(ProductInfo *info);

KRNLIMP void cleanup();

//! returns _se_translator_function currently set for current thread
KRNLIMP void *getHandler();
}; // namespace DagorHwException

#include <supp/dag_undef_COREIMP.h>
