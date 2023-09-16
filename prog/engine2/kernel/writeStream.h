#pragma once

#include <stdio.h>
#include <util/dag_stdint.h>

namespace debug_internal
{

typedef void *write_stream_t; // if lower bit is set then it's FILE*, otherwise it's IGenSave*

void write_stream_set_fmode(const char *fmode = "wb");

inline write_stream_t file2stream(FILE *fp) { return fp ? (write_stream_t)((uintptr_t)fp | 1) : NULL; }

write_stream_t write_stream_open(const char *path, int buf_size, FILE *fp = NULL, bool reopen = false);
void write_stream_close(write_stream_t stream);
void write_stream_flush(write_stream_t stream);
void write_stream_write(const void *ptr, size_t len, write_stream_t stream);

} // namespace debug_internal
