// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>
#include <matching/types.h>
#include <json/json.h>

namespace matching
{
class Client;
}

namespace dedicated_matching
{
void init();
void update();
void shutdown();
void notify_network_is_up();

int get_player_team(matching::UserId uid);
int get_player_req_teams_num(matching::UserId uid);
int64_t get_player_group(matching::UserId uid);
int get_player_app_id(matching::UserId uid);
eastl::string get_player_name(matching::UserId uid);
const Json::Value &get_mode_info();
const eastl::string &get_room_secret();
void on_player_team_changed(matching::UserId user_id, int team);
void player_kick_from_room(matching::UserId user_id);
// Reports a member the server lost as a leave: no blacklist, so it can rejoin to reconnect.
void player_remove_from_room(matching::UserId user_id);
void ban_player_in_room(matching::UserId user_id);
void leave_room();
void on_level_loaded();
int get_room_members_count();
const char *get_player_custom_info(matching::UserId uid);
bool has_player_penalty(matching::UserId uid, const char *penalty);
matching::RoomId get_current_room_id();
eastl::string get_current_cluster();
matching::Client *get_client();
// Matching client owns these RPCs so reply lifetime follows matching, not the relay ES.
void request_start_relay(int gen);
void set_urls(const eastl::vector<eastl::string> &extra_urls, const eastl::string &stun_request, int pub_gen);

const eastl::unordered_map<matching::UserId, Json::Value> &get_session_players();

void apply_room_info_on_join(const Json::Value &params);

void apply_room_invite(Json::Value const &params, Json::Value &resp);
void apply_room_member_leaved(Json::Value const &params);
void apply_room_member_joined(Json::Value const &params);
void apply_room_member_attr_changed(Json::Value const &params);
void apply_room_destroyed(Json::Value const &params);
void apply_room_member_kicked(Json::Value const &params);
void apply_room_attr_changed(Json::Value const &params);
} // namespace dedicated_matching
