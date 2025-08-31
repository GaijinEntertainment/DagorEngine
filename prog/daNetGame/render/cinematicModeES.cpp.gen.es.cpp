// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cinematicModeES.cpp.inl"
ECS_DEF_PULL_VAR(cinematicMode);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc cinematic_mode_weather_changed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cinematic_mode__weatherPreset"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void cinematic_mode_weather_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_weather_changed_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_weather_changed_es_event_handler_comps, "cinematic_mode__weatherPreset", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_weather_changed_es_event_handler_es_desc
(
  "cinematic_mode_weather_changed_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_weather_changed_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_weather_changed_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"cinematic_mode__weatherPreset");
static constexpr ecs::ComponentDesc cinematic_mode_toggle_rain_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("cinematic_mode__rain"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasRain"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_toggle_rain_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_toggle_rain_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_toggle_rain_es_event_handler_comps, "cinematic_mode__rain", bool)
    , ECS_RO_COMP(cinematic_mode_toggle_rain_es_event_handler_comps, "cinematic_mode__hasRain", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_toggle_rain_es_event_handler_es_desc
(
  "cinematic_mode_toggle_rain_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_toggle_rain_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_toggle_rain_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"cinematic_mode__rain");
static constexpr ecs::ComponentDesc cinematic_mode_toggle_snow_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("cinematic_mode__snow"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasSnow"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_toggle_snow_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_toggle_snow_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_toggle_snow_es_event_handler_comps, "cinematic_mode__snow", bool)
    , ECS_RO_COMP(cinematic_mode_toggle_snow_es_event_handler_comps, "cinematic_mode__hasSnow", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_toggle_snow_es_event_handler_es_desc
(
  "cinematic_mode_toggle_snow_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_toggle_snow_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_toggle_snow_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"cinematic_mode__snow");
static constexpr ecs::ComponentDesc cinematic_mode_toggle_lightning_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("cinematic_mode__lightning"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasLightning"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_toggle_lightning_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_toggle_lightning_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_toggle_lightning_es_event_handler_comps, "cinematic_mode__lightning", bool)
    , ECS_RO_COMP(cinematic_mode_toggle_lightning_es_event_handler_comps, "cinematic_mode__hasLightning", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_toggle_lightning_es_event_handler_es_desc
(
  "cinematic_mode_toggle_lightning_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_toggle_lightning_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_toggle_lightning_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"cinematic_mode__lightning");
static constexpr ecs::ComponentDesc cinematic_mode_record_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__manager"), ecs::ComponentTypeInfo<CinematicMode>()},
//start of 2 ro components at [1]
  {ECS_HASH("cinematic_mode__recording"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__loadAudio"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_record_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_record_es_event_handler(evt
        , ECS_RW_COMP(cinematic_mode_record_es_event_handler_comps, "cinematic_mode__manager", CinematicMode)
    , ECS_RO_COMP(cinematic_mode_record_es_event_handler_comps, "cinematic_mode__recording", bool)
    , ECS_RO_COMP(cinematic_mode_record_es_event_handler_comps, "cinematic_mode__loadAudio", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_record_es_event_handler_es_desc
(
  "cinematic_mode_record_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_record_es_event_handler_all_events),
  make_span(cinematic_mode_record_es_event_handler_comps+0, 1)/*rw*/,
  make_span(cinematic_mode_record_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"cinematic_mode__recording");
static constexpr ecs::ComponentDesc cinematic_mode_audio_record_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__manager"), ecs::ComponentTypeInfo<CinematicMode>()},
//start of 2 ro components at [1]
  {ECS_HASH("cinematic_mode__audio_recording"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__saveAudio"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_audio_record_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_audio_record_es_event_handler(evt
        , ECS_RW_COMP(cinematic_mode_audio_record_es_event_handler_comps, "cinematic_mode__manager", CinematicMode)
    , ECS_RO_COMP(cinematic_mode_audio_record_es_event_handler_comps, "cinematic_mode__audio_recording", bool)
    , ECS_RO_COMP(cinematic_mode_audio_record_es_event_handler_comps, "cinematic_mode__saveAudio", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_audio_record_es_event_handler_es_desc
(
  "cinematic_mode_audio_record_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_audio_record_es_event_handler_all_events),
  make_span(cinematic_mode_audio_record_es_event_handler_comps+0, 1)/*rw*/,
  make_span(cinematic_mode_audio_record_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"cinematic_mode__audio_recording");
static constexpr ecs::ComponentDesc cinematic_mode_get_weathers_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__weatherPresetList"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void cinematic_mode_get_weathers_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_get_weathers_es_event_handler(evt
        , ECS_RW_COMP(cinematic_mode_get_weathers_es_event_handler_comps, "cinematic_mode__weatherPresetList", ecs::StringList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_get_weathers_es_event_handler_es_desc
(
  "cinematic_mode_get_weathers_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_get_weathers_es_event_handler_all_events),
  make_span(cinematic_mode_get_weathers_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc cinematic_mode_change_time_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cinematic_mode__dayTime"), ecs::ComponentTypeInfo<float>()}
};
static void cinematic_mode_change_time_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_change_time_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_change_time_es_event_handler_comps, "cinematic_mode__dayTime", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_change_time_es_event_handler_es_desc
(
  "cinematic_mode_change_time_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_change_time_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_change_time_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"cinematic_mode__dayTime");
static constexpr ecs::ComponentDesc cinematic_mode_set_weather_es_event_handler_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("cinematic_mode__rain"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__snow"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__lightning"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasRain"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasSnow"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__hasLightning"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__dayTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("cinematic_mode__weatherPreset"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void cinematic_mode_set_weather_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_set_weather_es_event_handler(evt
        , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__rain", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__snow", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__lightning", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__hasRain", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__hasSnow", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__hasLightning", bool)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__dayTime", float)
    , ECS_RW_COMP(cinematic_mode_set_weather_es_event_handler_comps, "cinematic_mode__weatherPreset", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_set_weather_es_event_handler_es_desc
(
  "cinematic_mode_set_weather_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_set_weather_es_event_handler_all_events),
  make_span(cinematic_mode_set_weather_es_event_handler_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventSkiesLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc cinematic_mode_settings_changed_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("cinematic_mode__manager"), ecs::ComponentTypeInfo<CinematicMode>()},
  {ECS_HASH("cinematic_mode__lenseCoveringTex"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("cinematic_mode__lenseRadialTex"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 12 ro components at [3]
  {ECS_HASH("cinematic_mode__vignetteStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("cinematic_mode__lenseFlareIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("cinematic_mode__lenseDust"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("cinematic_mode__chromaticAberration"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("cinematic_mode__filmGrain"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("cinematic_mode__fps"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("cinematic_mode__subPixels"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("cinematic_mode__superPixels"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("cinematic_mode__fname"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("cinematic_mode__bitrate"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("cinematic_mode__audioDataFName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("cinematic_mode__enablePostBloom"), ecs::ComponentTypeInfo<bool>()}
};
static void cinematic_mode_settings_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_settings_changed_es_event_handler(evt
        , ECS_RW_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__manager", CinematicMode)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__vignetteStrength", float)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__lenseFlareIntensity", float)
    , ECS_RW_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__lenseCoveringTex", ecs::string)
    , ECS_RW_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__lenseRadialTex", ecs::string)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__lenseDust", bool)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__chromaticAberration", Point3)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__filmGrain", Point3)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__fps", int)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__subPixels", int)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__superPixels", int)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__fname", ecs::string)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__bitrate", int)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__audioDataFName", ecs::string)
    , ECS_RO_COMP(cinematic_mode_settings_changed_es_event_handler_comps, "cinematic_mode__enablePostBloom", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_settings_changed_es_event_handler_es_desc
(
  "cinematic_mode_settings_changed_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_settings_changed_es_event_handler_all_events),
  make_span(cinematic_mode_settings_changed_es_event_handler_comps+0, 3)/*rw*/,
  make_span(cinematic_mode_settings_changed_es_event_handler_comps+3, 12)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"cinematic_mode__audioDataFName,cinematic_mode__bitrate,cinematic_mode__chromaticAberration,cinematic_mode__enablePostBloom,cinematic_mode__filmGrain,cinematic_mode__fname,cinematic_mode__fps,cinematic_mode__lenseDust,cinematic_mode__lenseFlareIntensity,cinematic_mode__subPixels,cinematic_mode__superPixels,cinematic_mode__vignetteStrength");
static constexpr ecs::ComponentDesc cinematic_mode_save_bloom_threshold_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__saved_bloom_threshold"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [1]
  {ECS_HASH("cinematic_mode__default_bloom_threshold"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void cinematic_mode_save_bloom_threshold_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_save_bloom_threshold_es(evt
        , ECS_RW_COMP(cinematic_mode_save_bloom_threshold_es_comps, "cinematic_mode__saved_bloom_threshold", float)
    , ECS_RO_COMP_OR(cinematic_mode_save_bloom_threshold_es_comps, "cinematic_mode__default_bloom_threshold", float(1.0))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_save_bloom_threshold_es_es_desc
(
  "cinematic_mode_save_bloom_threshold_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_save_bloom_threshold_es_all_events),
  make_span(cinematic_mode_save_bloom_threshold_es_comps+0, 1)/*rw*/,
  make_span(cinematic_mode_save_bloom_threshold_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc cinematic_mode_restore_bloom_threshold_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cinematic_mode__saved_bloom_threshold"), ecs::ComponentTypeInfo<float>()}
};
static void cinematic_mode_restore_bloom_threshold_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_restore_bloom_threshold_es(evt
        , ECS_RO_COMP(cinematic_mode_restore_bloom_threshold_es_comps, "cinematic_mode__saved_bloom_threshold", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_restore_bloom_threshold_es_es_desc
(
  "cinematic_mode_restore_bloom_threshold_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_restore_bloom_threshold_es_all_events),
  empty_span(),
  make_span(cinematic_mode_restore_bloom_threshold_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc flare_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__manager"), ecs::ComponentTypeInfo<CinematicMode>()},
//start of 1 ro components at [1]
  {ECS_HASH("cinematic_mode__lenseFlareIntensity"), ecs::ComponentTypeInfo<float>()}
};
static void flare_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderLateTransEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    flare_render_es(static_cast<const RenderLateTransEvent&>(evt)
        , ECS_RW_COMP(flare_render_es_comps, "cinematic_mode__manager", CinematicMode)
    , ECS_RO_COMP(flare_render_es_comps, "cinematic_mode__lenseFlareIntensity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc flare_render_es_es_desc
(
  "flare_render_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, flare_render_es_all_events),
  make_span(flare_render_es_comps+0, 1)/*rw*/,
  make_span(flare_render_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderLateTransEvent>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc cinematic_mode_color_grading_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("cinematic_mode__colorGradingSelected"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("cinematic_mode__colorGradingOptions"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::StringList>>()}
};
static void cinematic_mode_color_grading_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cinematic_mode_color_grading_es_event_handler(evt
        , ECS_RO_COMP(cinematic_mode_color_grading_es_event_handler_comps, "cinematic_mode__colorGradingSelected", ecs::string)
    , ECS_RO_COMP(cinematic_mode_color_grading_es_event_handler_comps, "cinematic_mode__colorGradingOptions", ecs::SharedComponent<ecs::StringList>)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cinematic_mode_color_grading_es_event_handler_es_desc
(
  "cinematic_mode_color_grading_es",
  "prog/daNetGame/render/cinematicModeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cinematic_mode_color_grading_es_event_handler_all_events),
  empty_span(),
  make_span(cinematic_mode_color_grading_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"cinematic_mode__colorGradingSelected");
static constexpr ecs::ComponentDesc get_cinematic_mode_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__manager"), ecs::ComponentTypeInfo<CinematicMode>()}
};
static ecs::CompileTimeQueryDesc get_cinematic_mode_manager_ecs_query_desc
(
  "get_cinematic_mode_manager_ecs_query",
  make_span(get_cinematic_mode_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_cinematic_mode_manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_cinematic_mode_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_cinematic_mode_manager_ecs_query_comps, "cinematic_mode__manager", CinematicMode)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_video_recording_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cinematic_mode__recording"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc set_video_recording_ecs_query_desc
(
  "set_video_recording_ecs_query",
  make_span(set_video_recording_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_video_recording_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_video_recording_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_video_recording_ecs_query_comps, "cinematic_mode__recording", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_bloom_threshold_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bloom__threshold"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_bloom_threshold_ecs_query_desc
(
  "get_bloom_threshold_ecs_query",
  make_span(get_bloom_threshold_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_bloom_threshold_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_bloom_threshold_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_bloom_threshold_ecs_query_comps, "bloom__threshold", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_bloom_threshold_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bloom__threshold"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc set_bloom_threshold_ecs_query_desc
(
  "set_bloom_threshold_ecs_query",
  make_span(set_bloom_threshold_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_bloom_threshold_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_bloom_threshold_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_bloom_threshold_ecs_query_comps, "bloom__threshold", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_color_grading_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc delete_color_grading_ecs_query_desc
(
  "delete_color_grading_ecs_query",
  empty_span(),
  make_span(delete_color_grading_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_color_grading_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_color_grading_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_color_grading_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_color_grading_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_default_color_grade_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cinematic_mode__colorGradingOptions"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::StringList>>()}
};
static ecs::CompileTimeQueryDesc get_default_color_grade_ecs_query_desc
(
  "get_default_color_grade_ecs_query",
  empty_span(),
  make_span(get_default_color_grade_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_default_color_grade_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_default_color_grade_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_default_color_grade_ecs_query_comps, "cinematic_mode__colorGradingOptions", ecs::SharedComponent<ecs::StringList>)
            );

        }while (++comp != compE);
    }
  );
}
