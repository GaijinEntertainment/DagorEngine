// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameLauncher.h"

#include <statsd/statsd.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/updateStage.h>

#include "game/gameEvents.h"
#include "game/dasEvents.h"


namespace gamelauncher
{

static int game_played_cnt = 0;
static bool game_session_started = false;
static bool was_in_queue = false;


static void game_launcher_metrics_es_event_handler(EventGameSessionStarted const &) { game_session_started = true; }

static void game_launcher_metrics_es_event_handler(EventGameSessionFinished const &)
{
  if (game_session_started)
  {
    game_played_cnt++;
    game_session_started = false;
  }
}

static void game_launcher_metrics_es_event_handler(EventUserMMQueueJoined const &) { was_in_queue = true; }

void send_exit_metrics()
{
  if (game_played_cnt == 0 && was_in_queue)
    statsd::counter("app.exit_non_played");
  if (game_session_started)
    statsd::counter("app.exit_midgame");
}

} // namespace gamelauncher
