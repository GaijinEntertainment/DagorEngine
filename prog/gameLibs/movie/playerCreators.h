// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class IGenVideoPlayer;

extern IGenVideoPlayer *(*create_theora_movie_player)(const char *fn);
extern IGenVideoPlayer *(*create_native_movie_player)(const char *fn, int v, int a, float vol);
extern IGenVideoPlayer *(*create_dagui_movie_player)(const char *fn);
