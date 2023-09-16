#ifndef __GAIJIN_DAG_UTIL_H__
#define __GAIJIN_DAG_UTIL_H__
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <libTools/util/progressInd.h>
#include <libTools/dtx/dtx.h>

class CoolConsole;


bool import_dag(const char *dag_src, const char *dag_dst, CoolConsole *con = NULL);
bool replace_dag(const char *dag_src, const char *dag_dst, CoolConsole *con = NULL);
bool remap_dag_textures(const char *dag_name, const Tab<String> &original_tex, const Tab<String> &actual_tex);
bool erase_local_dag_textures(const char *dag_wildcard);
bool texture_names_are_equal(const char *name1, const char *name2);

const char *get_dagutil_last_error();


bool copy_dag_file(const char *src_name, const char *dst_name, Tab<String> &slotNames, Tab<String> &orgTexNames);

bool get_dag_textures(const char *path, Tab<String> &list);

#endif // __GAIJIN_DAG_UTIL_H__
