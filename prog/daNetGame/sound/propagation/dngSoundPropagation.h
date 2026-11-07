// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;

namespace dngsound
{
// main thread; uninstall_propagation() when the sndsys::init after install_propagation() fails
void install_propagation(const DataBlock &sndblk);
void uninstall_propagation();
} // namespace dngsound
