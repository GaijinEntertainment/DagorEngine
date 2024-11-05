//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class String;
class RenderableInstanceLodsResource;
class DynamicRenderableSceneLodsResource;

// This is a linear search, should only be used for debugging purposes
bool resolve_game_resource_name(String &name, const RenderableInstanceLodsResource *resource);
bool resolve_game_resource_name(String &name, const DynamicRenderableSceneLodsResource *resource);
