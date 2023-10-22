#include <osApiWrappers/dag_fileIoErr.h>

void (*dag_on_file_open)(const char *fname, void *file_handle, int flags) = NULL;
void (*dag_on_file_close)(void *file_handle) = NULL;
void (*dag_on_file_was_erased)(const char *fname) = NULL;

void (*dag_on_file_not_found)(const char *fname) = NULL;

bool (*dag_on_read_beyond_eof_cb)(void *file_handle, int ofs, int len, int read) = NULL;
bool (*dag_on_read_error_cb)(void *file_handle, int ofs, int len) = NULL;
bool (*dag_on_write_error_cb)(void *file_handle, int ofs, int len) = NULL;

bool (*dag_on_file_pre_open)(const char *fname) = NULL;

void (*dag_on_zlib_error_cb)(const char *fname, int error) = NULL;

void (*dag_on_assets_fatal_cb)(const char *assets_name) = NULL;

#define EXPORT_PULL dll_pull_osapiwrappers_fileIoErr
#include <supp/exportPull.h>
