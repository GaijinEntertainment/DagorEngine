// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <matching/types.h>
#include <json/json.h>

namespace dedicated_matching
{
void init();
void update();
void shutdown();

int get_player_team(matching::UserId uid);
int get_player_req_teams_num(matching::UserId uid);
int64_t get_player_group(matching::UserId uid);
int64_t get_player_original_group(matching::UserId uid);
int get_player_app_id(matching::UserId uid);
eastl::string get_player_name(matching::UserId uid);
const Json::Value &get_mode_info();
const eastl::string &get_room_secret();
void on_player_team_changed(matching::UserId user_id, int team);
void player_kick_from_room(matching::UserId user_id);
void ban_player_in_room(matching::UserId user_id);
void on_level_loaded();
int get_room_members_count();
const char *get_player_custom_info(matching::UserId uid);

void apply_room_info_on_join(const Json::Value &params);

void apply_room_invite(Json::Value const &params, Json::Value &resp);
void apply_room_member_leaved(Json::Value const &params);
void apply_room_member_joined(Json::Value const &params);
void apply_room_member_attr_changed(Json::Value const &params);
void apply_room_destroyed(Json::Value const &params);
void apply_room_member_kicked(Json::Value const &params);
void apply_room_attr_changed(Json::Value const &params);
} // namespace dedicated_matching
