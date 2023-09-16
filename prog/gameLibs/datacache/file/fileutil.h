#pragma once
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_tab.h>

int find_files_recursive(const char *dir_path, Tab<alefind_t> &out_list, char tmpPath[DAGOR_MAX_PATH]);
