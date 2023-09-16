//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <supp/dag_define_COREIMP.h>

enum class AsyncWriterMode
{
  TRUNC,
  APPEND
};

KRNLIMP IGenSave *create_async_writer(const char *fname, int buf_size, AsyncWriterMode mode = AsyncWriterMode::TRUNC);
KRNLIMP IGenSave *create_async_writer_temp(char *in_out_fname, int buf_size);

#include <supp/dag_undef_COREIMP.h>
