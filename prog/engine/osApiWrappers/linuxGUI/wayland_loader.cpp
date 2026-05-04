// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_dynLib.h>
#include "wayland.h"
#include <wayland/wayland-client.h>

uint32_t (*pwl_proxy_get_version)(struct wl_proxy *proxy);
int (*pwl_proxy_add_listener)(struct wl_proxy *proxy, void (**implementation)(void), void *data);
void (*pwl_proxy_destroy)(struct wl_proxy *proxy);
struct wl_display *(*pwl_display_connect)(const char *name);
void (*pwl_display_disconnect)(struct wl_display *display);
int (*pwl_display_roundtrip)(struct wl_display *display);
struct wl_proxy *(*pwl_proxy_marshal_flags)(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface,
  uint32_t version, uint32_t flags, ...);

bool Wayland::loadLib()
{
  libHandle = os_dll_load("libwayland-client.so");
  if (!libHandle)
    return false;

#define LOAD(var)                                                \
  p##var = (decltype(p##var))os_dll_get_symbol(libHandle, #var); \
  if (!(p##var))                                                 \
    return false;

  LOAD(wl_proxy_get_version);
  LOAD(wl_proxy_marshal_flags);
  LOAD(wl_proxy_add_listener);
  LOAD(wl_proxy_destroy);
  LOAD(wl_display_connect);
  LOAD(wl_display_disconnect);
  LOAD(wl_display_roundtrip);
#undef LOAD

  return true;
}

void Wayland::unloadLib()
{
  os_dll_close(libHandle);
  libHandle = nullptr;
}
