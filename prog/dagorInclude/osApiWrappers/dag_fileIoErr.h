//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

//! if installed, is called on each successful file open operation
//! (can be used to bind fname and file_handle to resolve filename in other callbacks)
extern KRNLIMP void (*dag_on_file_open)(const char *fname, void *file_handle, int flags);
//! if installed, is called on each file close operation
extern KRNLIMP void (*dag_on_file_close)(void *file_handle);

//! if installed, is called on each file after it was deleted
extern KRNLIMP void (*dag_on_file_was_erased)(const char *fname);

//! if installed, is called on each failure to open file
extern KRNLIMP void (*dag_on_file_not_found)(const char *fname);

//! dagor cricital error callcack called on read file error;
//! if installed, must return false to report IO fail or true to re-try reading
extern KRNLIMP bool (*dag_on_read_beyond_eof_cb)(void *file_handle, int ofs, int len, int read);

//! dagor cricital error callcack called on read file error;
//! if installed, must return false to report IO fail or true to re-try reading
extern KRNLIMP bool (*dag_on_read_error_cb)(void *file_handle, int ofs, int len);

//! dagor cricital error callcack called on write file error;
//! if installed, must return false to report IO fail or true to re-try writing
extern KRNLIMP bool (*dag_on_write_error_cb)(void *file_handle, int ofs, int len);

//! if installed, is called before attempt to open file
//! if it returns false, file is handled as non-existing
extern KRNLIMP bool (*dag_on_file_pre_open)(const char *fname);

//! called on unpacking errors in zlib
extern KRNLIMP void (*dag_on_zlib_error_cb)(const char *fname, int error);

//! can be called when assets failed to load and the game can't run after it
//! when installed should show system message box on android and ios and quit app after
extern KRNLIMP void (*dag_on_assets_fatal_cb)(const char *asset_name);

#include <supp/dag_undef_KRNLIMP.h>
