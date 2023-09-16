#include <movie/fullScreenMovie.h>
#include "playerCreators.h"
#include <videoPlayer/dag_videoPlayer.h>

static IGenVideoPlayer *create_theora(const char *fname) { return IGenVideoPlayer::create_ogg_video_player(fname); }

void init_player_for_ogg_theora() { create_theora_movie_player = &create_theora; }
