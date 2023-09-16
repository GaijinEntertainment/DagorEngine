//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>


class SqModules;

namespace bindquirrel
{

void bind_http_client(SqModules *module_mgr);
void http_client_wait_active_requests(int timeout_ms = 0); // if 0 - wait infinite
void http_client_on_vm_shutdown(HSQUIRRELVM vm);
void http_set_domains_whitelist(HSQUIRRELVM vm, const eastl::vector<eastl::string> &domains);
void http_set_blocking_wait_mode(bool on); // for use in console interpreter

} // namespace bindquirrel
