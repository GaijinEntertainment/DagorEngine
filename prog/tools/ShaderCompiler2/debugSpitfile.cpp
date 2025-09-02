// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "debugSpitfile.h"
#include <stdio.h>
#include <atomic>
#include <inttypes.h>

const char *debug_output_dir = nullptr;
const char *debug_output_dir_shader_name = nullptr;

void spitfile(const char *name, const char *entry, const char *postfix, uint64_t shader_variant_hash, const void *ptr, size_t len,
  const char *output)
{
  if (!output)
  {
    return;
  }

  char str[256];
  snprintf(str, 256, "%s/%s~%s~%s~0x%" PRIx64 ".txt", output, name, entry, postfix, shader_variant_hash);

  FILE *f = fopen(str, "wb");
  if (f)
  {
    fwrite(ptr, len, 1, f);
    fclose(f);
  }
}
