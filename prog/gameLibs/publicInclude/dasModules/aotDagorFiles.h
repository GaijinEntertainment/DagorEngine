//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <osApiWrappers/dag_files.h>

namespace bind_dascript
{
// dummy file, because file_ptr_t is not a strong type
struct DagFile
{};

const DagFile *das_df_open(const char *fname, int flags);
int dag_df_close(const DagFile *fp);
void dag_df_flush(const DagFile *fp);
int dag_df_seek_to(const DagFile *fp, int offset);
int dag_df_seek_rel(const DagFile *fp, int offset);
int dag_df_seek_end(const DagFile *fp, int offset);
int dag_df_tell(const DagFile *fp);
int dag_df_length(const DagFile *fp);
int dag_df_puts(const DagFile *fp, const char *str, das::Context *context);
char *dag_df_gets(const DagFile *fp, int maxLen, das::Context *context, das::LineInfoArg *at);
vec4f dag_builtin_df_read(das::Context &, das::SimNode_CallBase *call, vec4f *args);
vec4f dag_builtin_df_write(das::Context &, das::SimNode_CallBase *call, vec4f *args);
vec4f dag_builtin_df_load(das::Context &context, das::SimNode_CallBase *, vec4f *args);
int dag_builtin_df_write_raw(const DagFile *fp, const void *buf, int32_t len);

int dag_df_stat(const char *path, DagorStat &buf);
int dag_df_fstat(const DagFile *fp, DagorStat &buf);
char *dag_df_get_real_name(const char *fname, das::Context *context, das::LineInfoArg *at);
char *das_dd_get_named_mount_path(const char *mount_name, das::Context *context, das::LineInfoArg *at);
} // namespace bind_dascript
