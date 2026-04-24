// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_lightService.h>
#include <assets/assetMgr.h>

static int animCharEntityClassId = -1;
static ISceneLightService *ltService;
static bool registeredNotifier = false;
static FastNameMapEx usedAssetNames;
static float simDtScale = 1.0f;
static bool simPaused = false;
static DagorAssetMgr *aMgr = NULL;
static bool validateName = false;
static Tab<String> gameDataPath;
