//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes
class DagorGameScene;


class IDagorGameSceneRenderer
{
public:
  virtual void render(DagorGameScene &scene) = 0;
};


//! selects game scene renderer (NULL sets default renderer)
void dagor_set_game_scene_renderer(IDagorGameSceneRenderer *renderer);

//! returns currently selected game scene renderer
IDagorGameSceneRenderer *dagor_get_current_game_scene_renderer();
