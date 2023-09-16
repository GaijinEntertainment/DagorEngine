#pragma once

#include <supp/_platform.h>

class Driver3dInitCallback;

namespace windows
{
void *create_threaded_window(void *hinst, const char *name, int show, void *icon, const char *title, Driver3dInitCallback *cb);
void shutdown_threaded_window();

bool process_main_thread_messages(bool input_only, bool &out_ret_val);
RAWINPUT *get_rid();
unsigned long get_thread_id();
} // namespace windows
