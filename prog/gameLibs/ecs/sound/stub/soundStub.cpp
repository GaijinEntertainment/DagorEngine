// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/sound/soundGroup.h>

ECS_DEF_PULL_VAR(soundComponent);

using namespace sndsys;

EventHandle get_sound(const SoundEventGroup &, event_id_t) { return {}; }
bool has_sound(const SoundEventGroup &, event_id_t) { return false; }
void reject_sound(SoundEventGroup &, event_id_t, bool) {}

void abandon_sound(SoundEventGroup &, event_id_t) {}

void release_sound(SoundEventGroup &, event_id_t) {}

void abandon_all_sounds(SoundEventGroup &) {}

void release_all_sounds(SoundEventGroup &) {}

void update_sounds(SoundEventGroup &, const Animchar &) {}

void update_sounds(SoundEventGroup &, const TMatrix &) {}

void update_sounds(SoundEventGroup &) {}

bool has_sound(const SoundEventGroup &, const char *) { return false; }

bool has_sound(const SoundEventGroup &, const char *, const char *) { return false; }

void add_sound(SoundEventGroup &, event_id_t, const Point3 &, EventHandle, int) {}

void remove_sound(SoundEventGroup &, const sndsys::EventHandle &) {}

int get_num_sounds(const SoundEventGroup &, sndsys::event_id_t) { return 0; }

int get_num_sounds(const SoundEventGroup &) { return 0; }

SoundEventGroup::~SoundEventGroup() {}
