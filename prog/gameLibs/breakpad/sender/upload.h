// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>

namespace breakpad
{

struct Configuration;

bool upload(const Configuration &cfg, const std::string &url, std::string &response);

} // namespace breakpad
