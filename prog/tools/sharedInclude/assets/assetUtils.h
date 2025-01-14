//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once
#include <util/dag_string.h>

class DagorAsset;
class DagorAssetFolder;

void dag_reveal_in_explorer(const DagorAsset *asset);
void dag_reveal_in_explorer(const DagorAssetFolder *folder);
void dag_reveal_in_explorer(String file_path);
