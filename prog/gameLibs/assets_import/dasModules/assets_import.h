// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasDataBlock.h>

#include <osApiWrappers/dag_shellExecute.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include "../main/assetManager.h"
#include "../main/assetStatus.h"
#include "assets_import_bindings.h"

MAKE_TYPE_FACTORY(DagorAsset, DagorAsset);
MAKE_TYPE_FACTORY(DagorAssetMgr, DagorAssetMgr);

DAS_BIND_ENUM_CAST(AssetLoadingStatus);
