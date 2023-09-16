#include <movie/fullScreenMovie.h>
#include "playerCreators.h"
#include <videoPlayer/dag_videoPlayer.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <cstring>

static IGenVideoPlayer *create_native(const char *fname, int v, int a, float vol)
{
  (void)v;
  (void)a;
  (void)vol;

  const char *fext = dd_get_fname_ext(fname);
  if (fext && dd_stricmp(fext, ".ivf") == 0)
  {
    return IGenVideoPlayer::create_av1_video_player(fname);
  }
  else
  {
    return IGenVideoPlayer::create_ogg_video_player(fname);
  }
}

static IGenVideoPlayer *create_theora(const char *fname) { return IGenVideoPlayer::create_ogg_video_player(fname); }

void init_player_for_native_video()
{
  create_native_movie_player = &create_native;
  create_theora_movie_player = &create_theora;
}
