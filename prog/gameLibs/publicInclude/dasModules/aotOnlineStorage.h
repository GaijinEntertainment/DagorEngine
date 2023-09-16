//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace online_storage
{

void save_and_send_to_server();
void add_bigquery_record(const char *event, const char *params);

} // namespace online_storage
