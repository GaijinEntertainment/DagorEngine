// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// forward declarations for external classes
class IGenLoad;
class IGenSave;


#include <setjmp.h>
#include <stdio.h>
extern "C"
{
#include <image/jpeg-6b/jpeglib.h>
#include <image/jpeg-6b/jerror.h>
}

struct dagor_jpeg_error_mgr
{
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct dagor_jpeg_error_mgr *my_error_ptr;

void jpeg_stream_src(j_decompress_ptr cinfo, IGenLoad &infile);
void jpeg_stream_dest(j_compress_ptr cinfo, IGenSave &outfile);
