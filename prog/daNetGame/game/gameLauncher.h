// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace Sqrat
{
class Table;
}
namespace sceneload
{
struct UserGameModeContext;
}

namespace gamelauncher
{

void bind_game_launcher(Sqrat::Table &ns);
void send_exit_metrics();
const char *fill_ugm_context_from_table(const Sqrat::Table &params, sceneload::UserGameModeContext &ugm_context);

} // namespace gamelauncher
