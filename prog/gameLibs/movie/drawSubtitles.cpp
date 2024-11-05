// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <movie/fullScreenMovie.h>
#include <movie/subtitleplayer.h>

void draw_subtitle_string(const char *text, float lower_scr_bound, int max_lines)
{
  MovieSubPlayer::renderMultilineTex(text, lower_scr_bound, max_lines);
}
