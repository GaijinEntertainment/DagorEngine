module game_object_sound_common shared

require soundHash
require math.base
require math
require DagorMath


def private make_listener_in_shape_space(listener : float3; norm_tm : float3x4) : float3
  let dir = listener - norm_tm[3]
  return float3(dot(dir, norm_tm[0]), dot(dir, norm_tm[1]), dot(dir, norm_tm[2]))

def private is_listener_faraway(listener : float3; tm : float3x4; max_radius_sq : float) : bool
  return distance_sq(listener, tm[3]) >= max_radius_sq


def private place_sphere_game_object_sound_impl(listener : float3; extents : float3; tm : float3x4) : float3
  let local = make_listener_in_shape_space(listener, tm)
  return length_sq(local) < square(extents.x) ? listener : tm * (normalize(local) * extents.x)

def place_sphere_game_object_sound(listener : float3; extent_maxRadiusSq : float4; tm : float3x4) : float3
  return is_listener_faraway(listener, tm, extent_maxRadiusSq.w) ? tm[3] : place_sphere_game_object_sound_impl(listener, extent_maxRadiusSq.xyz, tm)


def private place_capsule_game_object_sound_impl(listener : float3; extents : float3; tm : float3x4) : float3
  let local = make_listener_in_shape_space(listener, tm)

  assume radius = extents.x
  let halfHeight = max(extents.y - radius, 0.)

  let pos = float3(0., clamp(local.y, -halfHeight, halfHeight), 0.)
  return distance_sq(pos, local) < square(radius) ? listener : tm * (pos + normalize(local - pos) * radius)

def place_capsule_game_object_sound(listener : float3; extent_maxRadiusSq : float4; tm : float3x4) : float3
  return is_listener_faraway(listener, tm, extent_maxRadiusSq.w) ? tm[3] : place_capsule_game_object_sound_impl(listener, extent_maxRadiusSq.xyz, tm)


def private place_box_game_object_sound_impl(listener : float3; extents : float3; tm : float3x4) : float3
  let local = make_listener_in_shape_space(listener, tm)
  let pos = clamp(local, -extents, extents)
  if pos == local
    return listener // dist_sq(listener, pos) should be zero if pos is inside volume
  return tm * pos

def place_box_game_object_sound(listener : float3; extent_maxRadiusSq : float4; tm : float3x4) : float3
  return is_listener_faraway(listener, tm, extent_maxRadiusSq.w) ? tm[3] : place_box_game_object_sound_impl(listener, extent_maxRadiusSq.xyz, tm)


def place_game_object_sound(listener : float3; extent_maxRadiusSq : float4; shape : uint; tm : float3x4) : float3
  if is_listener_faraway(listener, tm, extent_maxRadiusSq.w)
    return tm[3] // expecting that sound event will be released
  if shape == sound_hash("box")
    return place_box_game_object_sound(listener, extent_maxRadiusSq, tm)
  elif shape == sound_hash("sphere")
    return place_sphere_game_object_sound(listener, extent_maxRadiusSq, tm)
  elif shape == sound_hash("capsule")
    return place_capsule_game_object_sound(listener, extent_maxRadiusSq, tm)
  return tm[3]


def place_game_object_sound(listener : float3; extents : float3; shape : uint; tm : float3x4) : float3
  if shape == sound_hash("box")
    return place_box_game_object_sound_impl(listener, extents, tm)
  elif shape == sound_hash("sphere")
    return place_sphere_game_object_sound_impl(listener, extents, tm)
  elif shape == sound_hash("capsule")
    return place_capsule_game_object_sound_impl(listener, extents, tm)
  return tm[3]


def make_sphere_sound_shape_extents(bounds : float3) : float3
  var extents : float3
  extents.x = max(max(bounds.x, bounds.y), bounds.z) * 0.5
  extents.y = extents.x
  extents.z = extents.x
  return extents

def make_capsule_sound_shape_extents(bounds : float3) : float3
  var extents : float3
  extents.x = max(bounds.x, bounds.z) * 0.5
  extents.z = extents.x
  extents.y = max(0., bounds.y * 0.5 - extents.x)
  return extents

def make_box_sound_shape_extents(bounds : float3) : float3
  return bounds * 0.5


def make_sound_shape_extents(bounds : float3; shape : uint) : float3
  if shape == sound_hash("sphere")
    return make_sphere_sound_shape_extents(bounds)
  elif shape == sound_hash("capsule")
    return make_capsule_sound_shape_extents(bounds)
  elif shape == sound_hash("box")
    return make_box_sound_shape_extents(bounds)
  return float3(0., 0., 0.)


def make_sound_shape(var transform : float3x4&; shape : uint) : float3
  let bounds = float3(length(transform[0]), length(transform[1]), length(transform[2]))
  transform[0] *= safeinv(bounds.x)
  transform[1] *= safeinv(bounds.y)
  transform[2] *= safeinv(bounds.z)
  return make_sound_shape_extents(bounds, shape)


def make_sphere_sound_shape_radius(extents : float3) : float
  return extents.x

def make_capsule_sound_shape_radius(extents : float3) : float
  return extents.y + extents.x

def make_box_sound_shape_radius(extents : float3) : float
  return length(extents)


def find_sound_shape_radius(extents : float3; shape : uint) : float
  if shape == sound_hash("sphere")
    return make_sphere_sound_shape_radius(extents)
  elif shape == sound_hash("capsule")
    return make_capsule_sound_shape_radius(extents)
  elif shape == sound_hash("box")
    return make_box_sound_shape_radius(extents)
  return 0.


def get_norm_tm(tm : float3x4#; extents : float3)
  var normTm : float3x4 = tm
  normTm[0] = tm[0] * safeinv(extents.x * 2.)
  normTm[1] = tm[1] * safeinv(extents.y * 2.)
  normTm[2] = tm[2] * safeinv(extents.z * 2.)
  return normTm
