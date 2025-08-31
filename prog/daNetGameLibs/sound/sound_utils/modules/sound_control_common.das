module sound_control_common shared


enum SoundControlState : int
  SOUND_CONTROL_IS_HEARABLE = 1 << 0
  SOUND_CONTROL_IS_PLAYABLE = 1 << 1
  SOUND_CONTROL_IS_WATCHED = 1 << 2


def is_hearable_sound_state(sound_control__state : int)
  return (sound_control__state & int(SoundControlState.SOUND_CONTROL_IS_HEARABLE)) != 0


def is_watched_sound_state(sound_control__state : int)
  return (sound_control__state & int(SoundControlState.SOUND_CONTROL_IS_WATCHED)) != 0


def default_sound_control_state() : int
  return int(SoundControlState.SOUND_CONTROL_IS_PLAYABLE) | int(SoundControlState.SOUND_CONTROL_IS_HEARABLE)


def have_sound(sound_control__state : int)
  let values = int(SoundControlState.SOUND_CONTROL_IS_PLAYABLE) | int(SoundControlState.SOUND_CONTROL_IS_HEARABLE)
  return (sound_control__state & values) == values
