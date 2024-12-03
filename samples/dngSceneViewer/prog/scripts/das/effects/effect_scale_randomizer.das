require ecs
require DagorRandom

[es(tag=server, on_appear)]
def effect_scale_randomizer(evt : Event; effect_scale_randomizer__range : float2; var effect_scale_randomizer__scale : float&)
  effect_scale_randomizer__scale = rnd_float(effect_scale_randomizer__range.x, effect_scale_randomizer__range.y)

[es(tag=render, track=effect_scale_randomizer__scale, on_appear)]
def apply_effect_scale_randomizer(evt : Event; effect_scale_randomizer__scale : float; var effect__scale : float&)
  effect__scale = effect_scale_randomizer__scale
