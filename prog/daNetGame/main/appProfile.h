// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>
#include <rapidjson/document.h>

class DataBlock;

namespace app_profile
{
struct ProfileSettings
{
  eastl::string serverSessionId;
  eastl::string cmdAuthKey;
  eastl::string userId;
  int appId = 0;
  rapidjson::Document matchingInviteData; // not null on dedicated only or in offline mode

  bool inited = false;
  bool haveGraphicsWindow = false;
  bool devMode = false;
  bool disableRemoteNetServices = false;
  struct
  {
    eastl::string playFile;
    eastl::optional<eastl::string> record;
    float startTime = 0.0;
  } replay;
};

ProfileSettings const &get();
ProfileSettings &getRW();

void apply_settings_blk(const DataBlock &blk);
} // namespace app_profile

inline const rapidjson::Document &get_matching_invite_data() { return app_profile::get().matchingInviteData; }
void replay_reset();
