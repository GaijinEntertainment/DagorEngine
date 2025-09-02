// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>

class SimpleString;

/**
 * @brief Get the timestamp of the most recent commit for list of files
 * @param file_paths The list of file paths
 * @return The timestamp of the most recent commit
 * @note The timestamp is in milliseconds
 * @note If any of the files are locally modified or there are local commits not present in remote, the function will return 0
 */
int64_t get_git_files_last_commit_timestamp(dag::ConstSpan<SimpleString> file_path);
