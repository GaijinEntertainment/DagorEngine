// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>

ecs::string get_current_weather_preset_name();

void select_weather_preset(const char *preset_name);
void select_weather_preset_delayed(const char *preset_name); // can be called from ecs query

const ecs::Object::value_type *get_preset_by_name(ecs::EntityManager &mgr, ecs::EntityId level_eid, const char *preset_name);
const ecs::Object::value_type *get_preset_by_seed(ecs::EntityManager &mgr, ecs::EntityId level_eid);
void create_weather_choice_entity(ecs::EntityManager &mgr, const ecs::ChildComponent &effect, ecs::EntityId eid);

void get_weather_preset_list(ecs::StringList &);

bool has_any_rain_entity();
bool has_any_snow_entity();
bool has_any_lightning_entity();

void delete_rain_entities();
void delete_snow_entities();
void delete_lightning_entities();

void create_rain_entities();
void create_snow_entities();
void create_lightning_entities();