require ecs
require app // get_sync_time
require soundEvent


// the server's navmesh detour; with the SDF backend it only deepens the occlusion of a source that is already hidden
// the age uses the server's time, the only clock both sides share
[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE=soundDetourHintSource)]
def sound_detour_hint_voice_occlusion(evt : Event;
                                      sound_detour_hint__delta : float;
                                      sound_detour_hint__sourceMeasurementSeq : uint64;
                                      sound_detour_hint__sourceMeasurementTime : float;
                                      human_voice_sound_occlusion__blob : SoundOcclusionBlob&)
  set_blob_detour_hint(human_voice_sound_occlusion__blob, sound_detour_hint__delta, true,
    sound_detour_hint__sourceMeasurementSeq, get_sync_time() - sound_detour_hint__sourceMeasurementTime)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE=soundDetourHintSource)]
def sound_detour_hint_human_occlusion(evt : Event;
                                      sound_detour_hint__delta : float;
                                      sound_detour_hint__sourceMeasurementSeq : uint64;
                                      sound_detour_hint__sourceMeasurementTime : float;
                                      human_sound_occlusion__blob : SoundOcclusionBlob&)
  set_blob_detour_hint(human_sound_occlusion__blob, sound_detour_hint__delta, true,
    sound_detour_hint__sourceMeasurementSeq, get_sync_time() - sound_detour_hint__sourceMeasurementTime)
