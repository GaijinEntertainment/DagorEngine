#pragma once

#include <squirrel.h>

class SqModules;

namespace gamelib
{

namespace sound
{
struct PcmSound;
typedef SQInteger PlayingSoundHandle;

void initialize();
void finalize();
void bind_script(SqModules *module_mgr);
void on_script_reload(bool hard);
} // namespace sound

} // namespace gamelib
