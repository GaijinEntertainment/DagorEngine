// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <breakpad/binder.h>

namespace breakpad
{

Product::Product(bool) {}

void init(Product &&, Configuration &&) {}

void shutdown() {}                       // VOID
void dump(const CrashInfo & /*info*/) {} // VOID

Configuration *get_configuration() { return NULL; }
void configure(const Product & /*p*/) {}                // VOID
void add_file_to_report(const char * /*path*/) {}       // VOID
void remove_file_from_report(const char * /*path*/) {}  // VOID
void set_user_id(uint64_t) {}                           // VOID
void set_locale(const char *) {}                        // VOID
void set_product_title(const char *) {}                 // VOID
void set_environment(const char *) {}                   // VOID
void set_d3d_driver_data(const char *, const char *) {} // VOID

bool is_enabled() { return false; }

eastl::string Configuration::getSenderPath() { return {}; }

} // namespace breakpad
