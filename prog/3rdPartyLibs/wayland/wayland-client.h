/*
 * Copyright © 2008 Kristian Høgsberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 *
 *  \brief Include the client API and protocol C API.
 *
 *  \warning Use of this header file is discouraged. Prefer including
 *  wayland-client-core.h instead, which does not include the
 *  client protocol header and as such only defines the library
 *  API.
 */

#ifndef WAYLAND_CLIENT_H
#define WAYLAND_CLIENT_H

// patch out variadic defenition to properly route it to dynamic loaded symbol
#define wl_proxy_marshal_flags stub_wl_proxy_marshal_flags
#define wl_proxy_get_version   stub_wl_proxy_get_version
#define wl_proxy_add_listener  stub_wl_proxy_add_listener
#define wl_proxy_destroy       stub_wl_proxy_destroy
#define wl_display_connect     stub_wl_display_connect
#define wl_display_disconnect  stub_wl_display_disconnect
#define wl_display_roundtrip   stub_wl_display_roundtrip
#include "wayland-client-core.h"
#undef wl_proxy_marshal_flags
#undef wl_proxy_get_version
#undef wl_proxy_add_listener
#undef wl_proxy_destroy
#undef wl_display_connect
#undef wl_display_disconnect
#undef wl_display_roundtrip

extern struct wl_proxy *(*pwl_proxy_marshal_flags)(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface,
  uint32_t version, uint32_t flags, ...);
extern uint32_t (*pwl_proxy_get_version)(struct wl_proxy *proxy);
extern int (*pwl_proxy_add_listener)(struct wl_proxy *proxy, void (**implementation)(void), void *data);
extern void (*pwl_proxy_destroy)(struct wl_proxy *proxy);
extern struct wl_display *(*pwl_display_connect)(const char *name);
extern void (*pwl_display_disconnect)(struct wl_display *display);
extern int (*pwl_display_roundtrip)(struct wl_display *display);

#define wl_proxy_marshal_flags pwl_proxy_marshal_flags
#define wl_proxy_get_version   pwl_proxy_get_version
#define wl_proxy_add_listener  pwl_proxy_add_listener
#define wl_proxy_destroy       pwl_proxy_destroy
#define wl_display_connect     pwl_display_connect
#define wl_display_disconnect  pwl_display_disconnect
#define wl_display_roundtrip   pwl_display_roundtrip

#include "wayland-client-protocol.h"

#endif
