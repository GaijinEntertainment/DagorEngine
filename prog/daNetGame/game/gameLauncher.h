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
namespace net
{
struct ConnectParams;
}

namespace gamelauncher
{

void bind_game_launcher(Sqrat::Table &ns);
void send_exit_metrics();
const char *fill_ugm_context_from_table(const Sqrat::Table &params, sceneload::UserGameModeContext &ugm_context);
// Appends params.host_urls (and optional host_url_kinds) to connectParams. Returns URL count
// (0 when host_urls is missing or empty). Kinds apply only when lengths match; else Unknown.
int append_host_urls_from_params(const Sqrat::Table &params, net::ConnectParams &connectParams);

} // namespace gamelauncher
