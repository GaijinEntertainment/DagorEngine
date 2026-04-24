// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;
namespace Sqrat
{
class Object;
}

class TMatrix;
class TMatrix4;

namespace dngsound
{
void init();
void close();
void sync_update(float dt);
void gpu_update();
void apply_config_volumes();
void debug_draw();
void reset_listener();
void flush_commands();
} // namespace dngsound
