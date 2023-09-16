//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_fileIo.h>

#include <supp/dag_define_COREIMP.h>

KRNLIMP void write_zeros(IGenSave &cwr, int byte_num);

// general file->stream copying
KRNLIMP void copy_file_to_stream(file_ptr_t fp, IGenSave &cwr, int size);
KRNLIMP void copy_file_to_stream(file_ptr_t fp, IGenSave &cwr);
KRNLIMP void copy_file_to_stream(const char *fname, IGenSave &cwr);

// general stream->stream copying
KRNLIMP void copy_stream_to_stream(IGenLoad &crd, IGenSave &cwr, int size);

#include <supp/dag_undef_COREIMP.h>
