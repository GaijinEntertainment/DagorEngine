options no_aot
require ecs
require ecs.ecs_template
require math.color
require math.base
require DagorMath
require DagorDebug3D
require soundSystem
require soundHash
require DagorConsole
require DagorSystem
require environment_sounds.modules.game_object_sound_common
require gameObject


let BOX_FACES = fixed_array(float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(0.5, -0.5, 0.5), float3(0.5, -0.5, 0.5), float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, -0.5, 0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(0.5, -0.5, -0.5), float3(0.5, -0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5))
let SPHERE_FACES = fixed_array(float3(0., -0., 0.5), float3(0.15, -0., 0.48), float3(0., -0.15, 0.48), float3(0.15, -0., 0.48), float3(0.29, -0., 0.4), float3(0.16, -0.16, 0.44), float3(0.15, -0., 0.48), float3(0.16, -0.16, 0.44), float3(0., -0.15, 0.48), float3(0., -0.15, 0.48), float3(0.16, -0.16, 0.44), float3(0., -0.29, 0.4), float3(0.29, -0., 0.4), float3(0.4, -0., 0.29), float3(0.31, -0.17, 0.35), float3(0.29, -0., 0.4), float3(0.31, -0.17, 0.35), float3(0.16, -0.16, 0.44), float3(0.16, -0.16, 0.44), float3(0.31, -0.17, 0.35), float3(0.17, -0.31, 0.35), float3(0.16, -0.16, 0.44), float3(0.17, -0.31, 0.35), float3(0., -0.29, 0.4), float3(0., -0.29, 0.4), float3(0.17, -0.31, 0.35), float3(0., -0.4, 0.29), float3(0.4, -0., 0.29), float3(0.48, -0., 0.15), float3(0.43, -0.17, 0.19), float3(0.4, -0., 0.29), float3(0.43, -0.17, 0.19), float3(0.31, -0.17, 0.35), float3(0.31, -0.17, 0.35), float3(0.43, -0.17, 0.19), float3(0.32, -0.32, 0.21), float3(0.31, -0.17, 0.35), float3(0.32, -0.32, 0.21), float3(0.17, -0.31, 0.35), float3(0.17, -0.31, 0.35), float3(0.32, -0.32, 0.21), float3(0.17, -0.43, 0.19), float3(0.17, -0.31, 0.35), float3(0.17, -0.43, 0.19), float3(0., -0.4, 0.29), float3(0., -0.4, 0.29), float3(0.17, -0.43, 0.19), float3(0., -0.48, 0.15), float3(0.48, -0., 0.15), float3(0.5, -0., 0.), float3(0.48, -0.15, 0.), float3(0.48, -0., 0.15), float3(0.48, -0.15, 0.), float3(0.43, -0.17, 0.19), float3(0.43, -0.17, 0.19), float3(0.48, -0.15, 0.), float3(0.4, -0.29, 0.), float3(0.43, -0.17, 0.19), float3(0.4, -0.29, 0.), float3(0.32, -0.32, 0.21), float3(0.32, -0.32, 0.21), float3(0.4, -0.29, 0.), float3(0.29, -0.4, 0.), float3(0.32, -0.32, 0.21), float3(0.29, -0.4, 0.), float3(0.17, -0.43, 0.19), float3(0.17, -0.43, 0.19), float3(0.29, -0.4, 0.), float3(0.15, -0.48, 0.), float3(0.17, -0.43, 0.19), float3(0.15, -0.48, 0.), float3(0., -0.48, 0.15), float3(0., -0.48, 0.15), float3(0.15, -0.48, 0.), float3(0., -0.5, 0.), float3(0., -0., 0.5), float3(0., -0.15, 0.48), float3(-0.15, -0., 0.48), float3(0., -0.15, 0.48), float3(0., -0.29, 0.4), float3(-0.16, -0.16, 0.44), float3(0., -0.15, 0.48), float3(-0.16, -0.16, 0.44), float3(-0.15, -0., 0.48), float3(-0.15, -0., 0.48), float3(-0.16, -0.16, 0.44), float3(-0.29, -0., 0.4), float3(0., -0.29, 0.4), float3(0., -0.4, 0.29), float3(-0.17, -0.31, 0.35), float3(0., -0.29, 0.4), float3(-0.17, -0.31, 0.35), float3(-0.16, -0.16, 0.44), float3(-0.16, -0.16, 0.44), float3(-0.17, -0.31, 0.35), float3(-0.31, -0.17, 0.35), float3(-0.16, -0.16, 0.44), float3(-0.31, -0.17, 0.35), float3(-0.29, -0., 0.4), float3(-0.29, -0., 0.4), float3(-0.31, -0.17, 0.35), float3(-0.4, -0., 0.29), float3(0., -0.4, 0.29), float3(0., -0.48, 0.15), float3(-0.17, -0.43, 0.19), float3(0., -0.4, 0.29), float3(-0.17, -0.43, 0.19), float3(-0.17, -0.31, 0.35), float3(-0.17, -0.31, 0.35), float3(-0.17, -0.43, 0.19), float3(-0.32, -0.32, 0.21), float3(-0.17, -0.31, 0.35), float3(-0.32, -0.32, 0.21), float3(-0.31, -0.17, 0.35), float3(-0.31, -0.17, 0.35), float3(-0.32, -0.32, 0.21), float3(-0.43, -0.17, 0.19), float3(-0.31, -0.17, 0.35), float3(-0.43, -0.17, 0.19), float3(-0.4, -0., 0.29), float3(-0.4, -0., 0.29), float3(-0.43, -0.17, 0.19), float3(-0.48, -0., 0.15), float3(0., -0.48, 0.15), float3(0., -0.5, 0.), float3(-0.15, -0.48, 0.), float3(0., -0.48, 0.15), float3(-0.15, -0.48, 0.), float3(-0.17, -0.43, 0.19), float3(-0.17, -0.43, 0.19), float3(-0.15, -0.48, 0.), float3(-0.29, -0.4, 0.), float3(-0.17, -0.43, 0.19), float3(-0.29, -0.4, 0.), float3(-0.32, -0.32, 0.21), float3(-0.32, -0.32, 0.21), float3(-0.29, -0.4, 0.), float3(-0.4, -0.29, 0.), float3(-0.32, -0.32, 0.21), float3(-0.4, -0.29, 0.), float3(-0.43, -0.17, 0.19), float3(-0.43, -0.17, 0.19), float3(-0.4, -0.29, 0.), float3(-0.48, -0.15, 0.), float3(-0.43, -0.17, 0.19), float3(-0.48, -0.15, 0.), float3(-0.48, -0., 0.15), float3(-0.48, -0., 0.15), float3(-0.48, -0.15, 0.), float3(-0.5, -0., 0.), float3(0., -0., 0.5), float3(-0.15, -0., 0.48), float3(0., 0.15, 0.48), float3(-0.15, -0., 0.48), float3(-0.29, -0., 0.4), float3(-0.16, 0.16, 0.44), float3(-0.15, -0., 0.48), float3(-0.16, 0.16, 0.44), float3(0., 0.15, 0.48), float3(0., 0.15, 0.48), float3(-0.16, 0.16, 0.44), float3(0., 0.29, 0.4), float3(-0.29, -0., 0.4), float3(-0.4, -0., 0.29), float3(-0.31, 0.17, 0.35), float3(-0.29, -0., 0.4), float3(-0.31, 0.17, 0.35), float3(-0.16, 0.16, 0.44), float3(-0.16, 0.16, 0.44), float3(-0.31, 0.17, 0.35), float3(-0.17, 0.31, 0.35), float3(-0.16, 0.16, 0.44), float3(-0.17, 0.31, 0.35), float3(0., 0.29, 0.4), float3(0., 0.29, 0.4), float3(-0.17, 0.31, 0.35), float3(0., 0.4, 0.29), float3(-0.4, -0., 0.29), float3(-0.48, -0., 0.15), float3(-0.43, 0.17, 0.19), float3(-0.4, -0., 0.29), float3(-0.43, 0.17, 0.19), float3(-0.31, 0.17, 0.35), float3(-0.31, 0.17, 0.35), float3(-0.43, 0.17, 0.19), float3(-0.32, 0.32, 0.21), float3(-0.31, 0.17, 0.35), float3(-0.32, 0.32, 0.21), float3(-0.17, 0.31, 0.35), float3(-0.17, 0.31, 0.35), float3(-0.32, 0.32, 0.21), float3(-0.17, 0.43, 0.19), float3(-0.17, 0.31, 0.35), float3(-0.17, 0.43, 0.19), float3(0., 0.4, 0.29), float3(0., 0.4, 0.29), float3(-0.17, 0.43, 0.19), float3(0., 0.48, 0.15), float3(-0.48, -0., 0.15), float3(-0.5, -0., 0.), float3(-0.48, 0.15, 0.), float3(-0.48, -0., 0.15), float3(-0.48, 0.15, 0.), float3(-0.43, 0.17, 0.19), float3(-0.43, 0.17, 0.19), float3(-0.48, 0.15, 0.), float3(-0.4, 0.29, 0.), float3(-0.43, 0.17, 0.19), float3(-0.4, 0.29, 0.), float3(-0.32, 0.32, 0.21), float3(-0.32, 0.32, 0.21), float3(-0.4, 0.29, 0.), float3(-0.29, 0.4, 0.), float3(-0.32, 0.32, 0.21), float3(-0.29, 0.4, 0.), float3(-0.17, 0.43, 0.19), float3(-0.17, 0.43, 0.19), float3(-0.29, 0.4, 0.), float3(-0.15, 0.48, 0.), float3(-0.17, 0.43, 0.19), float3(-0.15, 0.48, 0.), float3(0., 0.48, 0.15), float3(0., 0.48, 0.15), float3(-0.15, 0.48, 0.), float3(0., 0.5, 0.), float3(0., -0., 0.5), float3(0., 0.15, 0.48), float3(0.15, -0., 0.48), float3(0., 0.15, 0.48), float3(0., 0.29, 0.4), float3(0.16, 0.16, 0.44), float3(0., 0.15, 0.48), float3(0.16, 0.16, 0.44), float3(0.15, -0., 0.48), float3(0.15, -0., 0.48), float3(0.16, 0.16, 0.44), float3(0.29, -0., 0.4), float3(0., 0.29, 0.4), float3(0., 0.4, 0.29), float3(0.17, 0.31, 0.35), float3(0., 0.29, 0.4), float3(0.17, 0.31, 0.35), float3(0.16, 0.16, 0.44), float3(0.16, 0.16, 0.44), float3(0.17, 0.31, 0.35), float3(0.31, 0.17, 0.35), float3(0.16, 0.16, 0.44), float3(0.31, 0.17, 0.35), float3(0.29, -0., 0.4), float3(0.29, -0., 0.4), float3(0.31, 0.17, 0.35), float3(0.4, -0., 0.29), float3(0., 0.4, 0.29), float3(0., 0.48, 0.15), float3(0.17, 0.43, 0.19), float3(0., 0.4, 0.29), float3(0.17, 0.43, 0.19), float3(0.17, 0.31, 0.35), float3(0.17, 0.31, 0.35), float3(0.17, 0.43, 0.19), float3(0.32, 0.32, 0.21), float3(0.17, 0.31, 0.35), float3(0.32, 0.32, 0.21), float3(0.31, 0.17, 0.35), float3(0.31, 0.17, 0.35), float3(0.32, 0.32, 0.21), float3(0.43, 0.17, 0.19), float3(0.31, 0.17, 0.35), float3(0.43, 0.17, 0.19), float3(0.4, -0., 0.29), float3(0.4, -0., 0.29), float3(0.43, 0.17, 0.19), float3(0.48, -0., 0.15), float3(0., 0.48, 0.15), float3(0., 0.5, 0.), float3(0.15, 0.48, 0.), float3(0., 0.48, 0.15), float3(0.15, 0.48, 0.), float3(0.17, 0.43, 0.19), float3(0.17, 0.43, 0.19), float3(0.15, 0.48, 0.), float3(0.29, 0.4, 0.), float3(0.17, 0.43, 0.19), float3(0.29, 0.4, 0.), float3(0.32, 0.32, 0.21), float3(0.32, 0.32, 0.21), float3(0.29, 0.4, 0.), float3(0.4, 0.29, 0.), float3(0.32, 0.32, 0.21), float3(0.4, 0.29, 0.), float3(0.43, 0.17, 0.19), float3(0.43, 0.17, 0.19), float3(0.4, 0.29, 0.), float3(0.48, 0.15, 0.), float3(0.43, 0.17, 0.19), float3(0.48, 0.15, 0.), float3(0.48, -0., 0.15), float3(0.48, -0., 0.15), float3(0.48, 0.15, 0.), float3(0.5, -0., 0.), float3(0., -0., -0.5), float3(0., -0.15, -0.48), float3(0.15, -0., -0.48), float3(0., -0.15, -0.48), float3(0., -0.29, -0.4), float3(0.16, -0.16, -0.44), float3(0., -0.15, -0.48), float3(0.16, -0.16, -0.44), float3(0.15, -0., -0.48), float3(0.15, -0., -0.48), float3(0.16, -0.16, -0.44), float3(0.29, -0., -0.4), float3(0., -0.29, -0.4), float3(0., -0.4, -0.29), float3(0.17, -0.31, -0.35), float3(0., -0.29, -0.4), float3(0.17, -0.31, -0.35), float3(0.16, -0.16, -0.44), float3(0.16, -0.16, -0.44), float3(0.17, -0.31, -0.35), float3(0.31, -0.17, -0.35), float3(0.16, -0.16, -0.44), float3(0.31, -0.17, -0.35), float3(0.29, -0., -0.4), float3(0.29, -0., -0.4), float3(0.31, -0.17, -0.35), float3(0.4, -0., -0.29), float3(0., -0.4, -0.29), float3(0., -0.48, -0.15), float3(0.17, -0.43, -0.19), float3(0., -0.4, -0.29), float3(0.17, -0.43, -0.19), float3(0.17, -0.31, -0.35), float3(0.17, -0.31, -0.35), float3(0.17, -0.43, -0.19), float3(0.32, -0.32, -0.21), float3(0.17, -0.31, -0.35), float3(0.32, -0.32, -0.21), float3(0.31, -0.17, -0.35), float3(0.31, -0.17, -0.35), float3(0.32, -0.32, -0.21), float3(0.43, -0.17, -0.19), float3(0.31, -0.17, -0.35), float3(0.43, -0.17, -0.19), float3(0.4, -0., -0.29), float3(0.4, -0., -0.29), float3(0.43, -0.17, -0.19), float3(0.48, -0., -0.15), float3(0., -0.48, -0.15), float3(0., -0.5, 0.), float3(0.15, -0.48, 0.), float3(0., -0.48, -0.15), float3(0.15, -0.48, 0.), float3(0.17, -0.43, -0.19), float3(0.17, -0.43, -0.19), float3(0.15, -0.48, 0.), float3(0.29, -0.4, 0.), float3(0.17, -0.43, -0.19), float3(0.29, -0.4, 0.), float3(0.32, -0.32, -0.21), float3(0.32, -0.32, -0.21), float3(0.29, -0.4, 0.), float3(0.4, -0.29, 0.), float3(0.32, -0.32, -0.21), float3(0.4, -0.29, 0.), float3(0.43, -0.17, -0.19), float3(0.43, -0.17, -0.19), float3(0.4, -0.29, 0.), float3(0.48, -0.15, 0.), float3(0.43, -0.17, -0.19), float3(0.48, -0.15, 0.), float3(0.48, -0., -0.15), float3(0.48, -0., -0.15), float3(0.48, -0.15, 0.), float3(0.5, -0., 0.), float3(0., -0., -0.5), float3(-0.15, -0., -0.48), float3(0., -0.15, -0.48), float3(-0.15, -0., -0.48), float3(-0.29, -0., -0.4), float3(-0.16, -0.16, -0.44), float3(-0.15, -0., -0.48), float3(-0.16, -0.16, -0.44), float3(0., -0.15, -0.48), float3(0., -0.15, -0.48), float3(-0.16, -0.16, -0.44), float3(0., -0.29, -0.4), float3(-0.29, -0., -0.4), float3(-0.4, -0., -0.29), float3(-0.31, -0.17, -0.35), float3(-0.29, -0., -0.4), float3(-0.31, -0.17, -0.35), float3(-0.16, -0.16, -0.44), float3(-0.16, -0.16, -0.44), float3(-0.31, -0.17, -0.35), float3(-0.17, -0.31, -0.35), float3(-0.16, -0.16, -0.44), float3(-0.17, -0.31, -0.35), float3(0., -0.29, -0.4), float3(0., -0.29, -0.4), float3(-0.17, -0.31, -0.35), float3(0., -0.4, -0.29), float3(-0.4, -0., -0.29), float3(-0.48, -0., -0.15), float3(-0.43, -0.17, -0.19), float3(-0.4, -0., -0.29), float3(-0.43, -0.17, -0.19), float3(-0.31, -0.17, -0.35), float3(-0.31, -0.17, -0.35), float3(-0.43, -0.17, -0.19), float3(-0.32, -0.32, -0.21), float3(-0.31, -0.17, -0.35), float3(-0.32, -0.32, -0.21), float3(-0.17, -0.31, -0.35), float3(-0.17, -0.31, -0.35), float3(-0.32, -0.32, -0.21), float3(-0.17, -0.43, -0.19), float3(-0.17, -0.31, -0.35), float3(-0.17, -0.43, -0.19), float3(0., -0.4, -0.29), float3(0., -0.4, -0.29), float3(-0.17, -0.43, -0.19), float3(0., -0.48, -0.15), float3(-0.48, -0., -0.15), float3(-0.5, -0., 0.), float3(-0.48, -0.15, 0.), float3(-0.48, -0., -0.15), float3(-0.48, -0.15, 0.), float3(-0.43, -0.17, -0.19), float3(-0.43, -0.17, -0.19), float3(-0.48, -0.15, 0.), float3(-0.4, -0.29, 0.), float3(-0.43, -0.17, -0.19), float3(-0.4, -0.29, 0.), float3(-0.32, -0.32, -0.21), float3(-0.32, -0.32, -0.21), float3(-0.4, -0.29, 0.), float3(-0.29, -0.4, 0.), float3(-0.32, -0.32, -0.21), float3(-0.29, -0.4, 0.), float3(-0.17, -0.43, -0.19), float3(-0.17, -0.43, -0.19), float3(-0.29, -0.4, 0.), float3(-0.15, -0.48, 0.), float3(-0.17, -0.43, -0.19), float3(-0.15, -0.48, 0.), float3(0., -0.48, -0.15), float3(0., -0.48, -0.15), float3(-0.15, -0.48, 0.), float3(0., -0.5, 0.), float3(0., -0., -0.5), float3(0., 0.15, -0.48), float3(-0.15, -0., -0.48), float3(0., 0.15, -0.48), float3(0., 0.29, -0.4), float3(-0.16, 0.16, -0.44), float3(0., 0.15, -0.48), float3(-0.16, 0.16, -0.44), float3(-0.15, -0., -0.48), float3(-0.15, -0., -0.48), float3(-0.16, 0.16, -0.44), float3(-0.29, -0., -0.4), float3(0., 0.29, -0.4), float3(0., 0.4, -0.29), float3(-0.17, 0.31, -0.35), float3(0., 0.29, -0.4), float3(-0.17, 0.31, -0.35), float3(-0.16, 0.16, -0.44), float3(-0.16, 0.16, -0.44), float3(-0.17, 0.31, -0.35), float3(-0.31, 0.17, -0.35), float3(-0.16, 0.16, -0.44), float3(-0.31, 0.17, -0.35), float3(-0.29, -0., -0.4), float3(-0.29, -0., -0.4), float3(-0.31, 0.17, -0.35), float3(-0.4, -0., -0.29), float3(0., 0.4, -0.29), float3(0., 0.48, -0.15), float3(-0.17, 0.43, -0.19), float3(0., 0.4, -0.29), float3(-0.17, 0.43, -0.19), float3(-0.17, 0.31, -0.35), float3(-0.17, 0.31, -0.35), float3(-0.17, 0.43, -0.19), float3(-0.32, 0.32, -0.21), float3(-0.17, 0.31, -0.35), float3(-0.32, 0.32, -0.21), float3(-0.31, 0.17, -0.35), float3(-0.31, 0.17, -0.35), float3(-0.32, 0.32, -0.21), float3(-0.43, 0.17, -0.19), float3(-0.31, 0.17, -0.35), float3(-0.43, 0.17, -0.19), float3(-0.4, -0., -0.29), float3(-0.4, -0., -0.29), float3(-0.43, 0.17, -0.19), float3(-0.48, -0., -0.15), float3(0., 0.48, -0.15), float3(0., 0.5, 0.), float3(-0.15, 0.48, 0.), float3(0., 0.48, -0.15), float3(-0.15, 0.48, 0.), float3(-0.17, 0.43, -0.19), float3(-0.17, 0.43, -0.19), float3(-0.15, 0.48, 0.), float3(-0.29, 0.4, 0.), float3(-0.17, 0.43, -0.19), float3(-0.29, 0.4, 0.), float3(-0.32, 0.32, -0.21), float3(-0.32, 0.32, -0.21), float3(-0.29, 0.4, 0.), float3(-0.4, 0.29, 0.), float3(-0.32, 0.32, -0.21), float3(-0.4, 0.29, 0.), float3(-0.43, 0.17, -0.19), float3(-0.43, 0.17, -0.19), float3(-0.4, 0.29, 0.), float3(-0.48, 0.15, 0.), float3(-0.43, 0.17, -0.19), float3(-0.48, 0.15, 0.), float3(-0.48, -0., -0.15), float3(-0.48, -0., -0.15), float3(-0.48, 0.15, 0.), float3(-0.5, -0., 0.), float3(0., -0., -0.5), float3(0.15, -0., -0.48), float3(0., 0.15, -0.48), float3(0.15, -0., -0.48), float3(0.29, -0., -0.4), float3(0.16, 0.16, -0.44), float3(0.15, -0., -0.48), float3(0.16, 0.16, -0.44), float3(0., 0.15, -0.48), float3(0., 0.15, -0.48), float3(0.16, 0.16, -0.44), float3(0., 0.29, -0.4), float3(0.29, -0., -0.4), float3(0.4, -0., -0.29), float3(0.31, 0.17, -0.35), float3(0.29, -0., -0.4), float3(0.31, 0.17, -0.35), float3(0.16, 0.16, -0.44), float3(0.16, 0.16, -0.44), float3(0.31, 0.17, -0.35), float3(0.17, 0.31, -0.35), float3(0.16, 0.16, -0.44), float3(0.17, 0.31, -0.35), float3(0., 0.29, -0.4), float3(0., 0.29, -0.4), float3(0.17, 0.31, -0.35), float3(0., 0.4, -0.29), float3(0.4, -0., -0.29), float3(0.48, -0., -0.15), float3(0.43, 0.17, -0.19), float3(0.4, -0., -0.29), float3(0.43, 0.17, -0.19), float3(0.31, 0.17, -0.35), float3(0.31, 0.17, -0.35), float3(0.43, 0.17, -0.19), float3(0.32, 0.32, -0.21), float3(0.31, 0.17, -0.35), float3(0.32, 0.32, -0.21), float3(0.17, 0.31, -0.35), float3(0.17, 0.31, -0.35), float3(0.32, 0.32, -0.21), float3(0.17, 0.43, -0.19), float3(0.17, 0.31, -0.35), float3(0.17, 0.43, -0.19), float3(0., 0.4, -0.29), float3(0., 0.4, -0.29), float3(0.17, 0.43, -0.19), float3(0., 0.48, -0.15), float3(0.48, -0., -0.15), float3(0.5, -0., 0.), float3(0.48, 0.15, 0.), float3(0.48, -0., -0.15), float3(0.48, 0.15, 0.), float3(0.43, 0.17, -0.19), float3(0.43, 0.17, -0.19), float3(0.48, 0.15, 0.), float3(0.4, 0.29, 0.), float3(0.43, 0.17, -0.19), float3(0.4, 0.29, 0.), float3(0.32, 0.32, -0.21), float3(0.32, 0.32, -0.21), float3(0.4, 0.29, 0.), float3(0.29, 0.4, 0.), float3(0.32, 0.32, -0.21), float3(0.29, 0.4, 0.), float3(0.17, 0.43, -0.19), float3(0.17, 0.43, -0.19), float3(0.29, 0.4, 0.), float3(0.15, 0.48, 0.), float3(0.17, 0.43, -0.19), float3(0.15, 0.48, 0.), float3(0., 0.48, -0.15), float3(0., 0.48, -0.15), float3(0.15, 0.48, 0.), float3(0., 0.5, 0.))
let HEMISPHERE_FACES = fixed_array(float3(0., 0.5, -0.), float3(0.15, 0.48, -0.), float3(0., 0.48, -0.15), float3(0.15, 0.48, -0.), float3(0.29, 0.4, -0.), float3(0.16, 0.44, -0.16), float3(0.15, 0.48, -0.), float3(0.16, 0.44, -0.16), float3(0., 0.48, -0.15), float3(0., 0.48, -0.15), float3(0.16, 0.44, -0.16), float3(0., 0.4, -0.29), float3(0.29, 0.4, -0.), float3(0.4, 0.29, -0.), float3(0.31, 0.35, -0.17), float3(0.29, 0.4, -0.), float3(0.31, 0.35, -0.17), float3(0.16, 0.44, -0.16), float3(0.16, 0.44, -0.16), float3(0.31, 0.35, -0.17), float3(0.17, 0.35, -0.31), float3(0.16, 0.44, -0.16), float3(0.17, 0.35, -0.31), float3(0., 0.4, -0.29), float3(0., 0.4, -0.29), float3(0.17, 0.35, -0.31), float3(0., 0.29, -0.4), float3(0.4, 0.29, -0.), float3(0.48, 0.15, -0.), float3(0.43, 0.19, -0.17), float3(0.4, 0.29, -0.), float3(0.43, 0.19, -0.17), float3(0.31, 0.35, -0.17), float3(0.31, 0.35, -0.17), float3(0.43, 0.19, -0.17), float3(0.32, 0.21, -0.32), float3(0.31, 0.35, -0.17), float3(0.32, 0.21, -0.32), float3(0.17, 0.35, -0.31), float3(0.17, 0.35, -0.31), float3(0.32, 0.21, -0.32), float3(0.17, 0.19, -0.43), float3(0.17, 0.35, -0.31), float3(0.17, 0.19, -0.43), float3(0., 0.29, -0.4), float3(0., 0.29, -0.4), float3(0.17, 0.19, -0.43), float3(0., 0.15, -0.48), float3(0.48, 0.15, -0.), float3(0.5, 0., -0.), float3(0.48, 0., -0.15), float3(0.48, 0.15, -0.), float3(0.48, 0., -0.15), float3(0.43, 0.19, -0.17), float3(0.43, 0.19, -0.17), float3(0.48, 0., -0.15), float3(0.4, 0., -0.29), float3(0.43, 0.19, -0.17), float3(0.4, 0., -0.29), float3(0.32, 0.21, -0.32), float3(0.32, 0.21, -0.32), float3(0.4, 0., -0.29), float3(0.29, 0., -0.4), float3(0.32, 0.21, -0.32), float3(0.29, 0., -0.4), float3(0.17, 0.19, -0.43), float3(0.17, 0.19, -0.43), float3(0.29, 0., -0.4), float3(0.15, 0., -0.48), float3(0.17, 0.19, -0.43), float3(0.15, 0., -0.48), float3(0., 0.15, -0.48), float3(0., 0.15, -0.48), float3(0.15, 0., -0.48), float3(0., 0., -0.5), float3(0., 0.5, -0.), float3(0., 0.48, -0.15), float3(-0.15, 0.48, -0.), float3(0., 0.48, -0.15), float3(0., 0.4, -0.29), float3(-0.16, 0.44, -0.16), float3(0., 0.48, -0.15), float3(-0.16, 0.44, -0.16), float3(-0.15, 0.48, -0.), float3(-0.15, 0.48, -0.), float3(-0.16, 0.44, -0.16), float3(-0.29, 0.4, -0.), float3(0., 0.4, -0.29), float3(0., 0.29, -0.4), float3(-0.17, 0.35, -0.31), float3(0., 0.4, -0.29), float3(-0.17, 0.35, -0.31), float3(-0.16, 0.44, -0.16), float3(-0.16, 0.44, -0.16), float3(-0.17, 0.35, -0.31), float3(-0.31, 0.35, -0.17), float3(-0.16, 0.44, -0.16), float3(-0.31, 0.35, -0.17), float3(-0.29, 0.4, -0.), float3(-0.29, 0.4, -0.), float3(-0.31, 0.35, -0.17), float3(-0.4, 0.29, -0.), float3(0., 0.29, -0.4), float3(0., 0.15, -0.48), float3(-0.17, 0.19, -0.43), float3(0., 0.29, -0.4), float3(-0.17, 0.19, -0.43), float3(-0.17, 0.35, -0.31), float3(-0.17, 0.35, -0.31), float3(-0.17, 0.19, -0.43), float3(-0.32, 0.21, -0.32), float3(-0.17, 0.35, -0.31), float3(-0.32, 0.21, -0.32), float3(-0.31, 0.35, -0.17), float3(-0.31, 0.35, -0.17), float3(-0.32, 0.21, -0.32), float3(-0.43, 0.19, -0.17), float3(-0.31, 0.35, -0.17), float3(-0.43, 0.19, -0.17), float3(-0.4, 0.29, -0.), float3(-0.4, 0.29, -0.), float3(-0.43, 0.19, -0.17), float3(-0.48, 0.15, -0.), float3(0., 0.15, -0.48), float3(0., 0., -0.5), float3(-0.15, 0., -0.48), float3(0., 0.15, -0.48), float3(-0.15, 0., -0.48), float3(-0.17, 0.19, -0.43), float3(-0.17, 0.19, -0.43), float3(-0.15, 0., -0.48), float3(-0.29, 0., -0.4), float3(-0.17, 0.19, -0.43), float3(-0.29, 0., -0.4), float3(-0.32, 0.21, -0.32), float3(-0.32, 0.21, -0.32), float3(-0.29, 0., -0.4), float3(-0.4, 0., -0.29), float3(-0.32, 0.21, -0.32), float3(-0.4, 0., -0.29), float3(-0.43, 0.19, -0.17), float3(-0.43, 0.19, -0.17), float3(-0.4, 0., -0.29), float3(-0.48, 0., -0.15), float3(-0.43, 0.19, -0.17), float3(-0.48, 0., -0.15), float3(-0.48, 0.15, -0.), float3(-0.48, 0.15, -0.), float3(-0.48, 0., -0.15), float3(-0.5, 0., -0.), float3(0., 0.5, -0.), float3(-0.15, 0.48, -0.), float3(0., 0.48, 0.15), float3(-0.15, 0.48, -0.), float3(-0.29, 0.4, -0.), float3(-0.16, 0.44, 0.16), float3(-0.15, 0.48, -0.), float3(-0.16, 0.44, 0.16), float3(0., 0.48, 0.15), float3(0., 0.48, 0.15), float3(-0.16, 0.44, 0.16), float3(0., 0.4, 0.29), float3(-0.29, 0.4, -0.), float3(-0.4, 0.29, -0.), float3(-0.31, 0.35, 0.17), float3(-0.29, 0.4, -0.), float3(-0.31, 0.35, 0.17), float3(-0.16, 0.44, 0.16), float3(-0.16, 0.44, 0.16), float3(-0.31, 0.35, 0.17), float3(-0.17, 0.35, 0.31), float3(-0.16, 0.44, 0.16), float3(-0.17, 0.35, 0.31), float3(0., 0.4, 0.29), float3(0., 0.4, 0.29), float3(-0.17, 0.35, 0.31), float3(0., 0.29, 0.4), float3(-0.4, 0.29, -0.), float3(-0.48, 0.15, -0.), float3(-0.43, 0.19, 0.17), float3(-0.4, 0.29, -0.), float3(-0.43, 0.19, 0.17), float3(-0.31, 0.35, 0.17), float3(-0.31, 0.35, 0.17), float3(-0.43, 0.19, 0.17), float3(-0.32, 0.21, 0.32), float3(-0.31, 0.35, 0.17), float3(-0.32, 0.21, 0.32), float3(-0.17, 0.35, 0.31), float3(-0.17, 0.35, 0.31), float3(-0.32, 0.21, 0.32), float3(-0.17, 0.19, 0.43), float3(-0.17, 0.35, 0.31), float3(-0.17, 0.19, 0.43), float3(0., 0.29, 0.4), float3(0., 0.29, 0.4), float3(-0.17, 0.19, 0.43), float3(0., 0.15, 0.48), float3(-0.48, 0.15, -0.), float3(-0.5, 0., -0.), float3(-0.48, 0., 0.15), float3(-0.48, 0.15, -0.), float3(-0.48, 0., 0.15), float3(-0.43, 0.19, 0.17), float3(-0.43, 0.19, 0.17), float3(-0.48, 0., 0.15), float3(-0.4, 0., 0.29), float3(-0.43, 0.19, 0.17), float3(-0.4, 0., 0.29), float3(-0.32, 0.21, 0.32), float3(-0.32, 0.21, 0.32), float3(-0.4, 0., 0.29), float3(-0.29, 0., 0.4), float3(-0.32, 0.21, 0.32), float3(-0.29, 0., 0.4), float3(-0.17, 0.19, 0.43), float3(-0.17, 0.19, 0.43), float3(-0.29, 0., 0.4), float3(-0.15, 0., 0.48), float3(-0.17, 0.19, 0.43), float3(-0.15, 0., 0.48), float3(0., 0.15, 0.48), float3(0., 0.15, 0.48), float3(-0.15, 0., 0.48), float3(0., 0., 0.5), float3(0., 0.5, -0.), float3(0., 0.48, 0.15), float3(0.15, 0.48, -0.), float3(0., 0.48, 0.15), float3(0., 0.4, 0.29), float3(0.16, 0.44, 0.16), float3(0., 0.48, 0.15), float3(0.16, 0.44, 0.16), float3(0.15, 0.48, -0.), float3(0.15, 0.48, -0.), float3(0.16, 0.44, 0.16), float3(0.29, 0.4, -0.), float3(0., 0.4, 0.29), float3(0., 0.29, 0.4), float3(0.17, 0.35, 0.31), float3(0., 0.4, 0.29), float3(0.17, 0.35, 0.31), float3(0.16, 0.44, 0.16), float3(0.16, 0.44, 0.16), float3(0.17, 0.35, 0.31), float3(0.31, 0.35, 0.17), float3(0.16, 0.44, 0.16), float3(0.31, 0.35, 0.17), float3(0.29, 0.4, -0.), float3(0.29, 0.4, -0.), float3(0.31, 0.35, 0.17), float3(0.4, 0.29, -0.), float3(0., 0.29, 0.4), float3(0., 0.15, 0.48), float3(0.17, 0.19, 0.43), float3(0., 0.29, 0.4), float3(0.17, 0.19, 0.43), float3(0.17, 0.35, 0.31), float3(0.17, 0.35, 0.31), float3(0.17, 0.19, 0.43), float3(0.32, 0.21, 0.32), float3(0.17, 0.35, 0.31), float3(0.32, 0.21, 0.32), float3(0.31, 0.35, 0.17), float3(0.31, 0.35, 0.17), float3(0.32, 0.21, 0.32), float3(0.43, 0.19, 0.17), float3(0.31, 0.35, 0.17), float3(0.43, 0.19, 0.17), float3(0.4, 0.29, -0.), float3(0.4, 0.29, -0.), float3(0.43, 0.19, 0.17), float3(0.48, 0.15, -0.), float3(0., 0.15, 0.48), float3(0., 0., 0.5), float3(0.15, 0., 0.48), float3(0., 0.15, 0.48), float3(0.15, 0., 0.48), float3(0.17, 0.19, 0.43), float3(0.17, 0.19, 0.43), float3(0.15, 0., 0.48), float3(0.29, 0., 0.4), float3(0.17, 0.19, 0.43), float3(0.29, 0., 0.4), float3(0.32, 0.21, 0.32), float3(0.32, 0.21, 0.32), float3(0.29, 0., 0.4), float3(0.4, 0., 0.29), float3(0.32, 0.21, 0.32), float3(0.4, 0., 0.29), float3(0.43, 0.19, 0.17), float3(0.43, 0.19, 0.17), float3(0.4, 0., 0.29), float3(0.48, 0., 0.15), float3(0.43, 0.19, 0.17), float3(0.48, 0., 0.15), float3(0.48, 0.15, -0.), float3(0.48, 0.15, -0.), float3(0.48, 0., 0.15), float3(0.5, 0., -0.))
let TUBE_FACES = fixed_array(float3(0.48, 0.5, -0.15), float3(0.5, 0.5, -0.), float3(0.5, -0.5, -0.), float3(0.5, -0.5, -0.), float3(0.48, -0.5, -0.15), float3(0.48, 0.5, -0.15), float3(0.4, 0.5, -0.29), float3(0.48, 0.5, -0.15), float3(0.48, -0.5, -0.15), float3(0.48, -0.5, -0.15), float3(0.4, -0.5, -0.29), float3(0.4, 0.5, -0.29), float3(0.29, 0.5, -0.4), float3(0.4, 0.5, -0.29), float3(0.4, -0.5, -0.29), float3(0.4, -0.5, -0.29), float3(0.29, -0.5, -0.4), float3(0.29, 0.5, -0.4), float3(0.15, 0.5, -0.48), float3(0.29, 0.5, -0.4), float3(0.29, -0.5, -0.4), float3(0.29, -0.5, -0.4), float3(0.15, -0.5, -0.48), float3(0.15, 0.5, -0.48), float3(0., 0.5, -0.5), float3(0.15, 0.5, -0.48), float3(0.15, -0.5, -0.48), float3(0.15, -0.5, -0.48), float3(0., -0.5, -0.5), float3(0., 0.5, -0.5), float3(-0.15, 0.5, -0.48), float3(0., 0.5, -0.5), float3(0., -0.5, -0.5), float3(0., -0.5, -0.5), float3(-0.15, -0.5, -0.48), float3(-0.15, 0.5, -0.48), float3(-0.29, 0.5, -0.4), float3(-0.15, 0.5, -0.48), float3(-0.15, -0.5, -0.48), float3(-0.15, -0.5, -0.48), float3(-0.29, -0.5, -0.4), float3(-0.29, 0.5, -0.4), float3(-0.4, 0.5, -0.29), float3(-0.29, 0.5, -0.4), float3(-0.29, -0.5, -0.4), float3(-0.29, -0.5, -0.4), float3(-0.4, -0.5, -0.29), float3(-0.4, 0.5, -0.29), float3(-0.48, 0.5, -0.15), float3(-0.4, 0.5, -0.29), float3(-0.4, -0.5, -0.29), float3(-0.4, -0.5, -0.29), float3(-0.48, -0.5, -0.15), float3(-0.48, 0.5, -0.15), float3(-0.5, 0.5, -0.), float3(-0.48, 0.5, -0.15), float3(-0.48, -0.5, -0.15), float3(-0.48, -0.5, -0.15), float3(-0.5, -0.5, -0.), float3(-0.5, 0.5, -0.), float3(-0.48, 0.5, 0.15), float3(-0.5, 0.5, -0.), float3(-0.5, -0.5, -0.), float3(-0.5, -0.5, -0.), float3(-0.48, -0.5, 0.15), float3(-0.48, 0.5, 0.15), float3(-0.4, 0.5, 0.29), float3(-0.48, 0.5, 0.15), float3(-0.48, -0.5, 0.15), float3(-0.48, -0.5, 0.15), float3(-0.4, -0.5, 0.29), float3(-0.4, 0.5, 0.29), float3(-0.29, 0.5, 0.4), float3(-0.4, 0.5, 0.29), float3(-0.4, -0.5, 0.29), float3(-0.4, -0.5, 0.29), float3(-0.29, -0.5, 0.4), float3(-0.29, 0.5, 0.4), float3(-0.15, 0.5, 0.48), float3(-0.29, 0.5, 0.4), float3(-0.29, -0.5, 0.4), float3(-0.29, -0.5, 0.4), float3(-0.15, -0.5, 0.48), float3(-0.15, 0.5, 0.48), float3(0., 0.5, 0.5), float3(-0.15, 0.5, 0.48), float3(-0.15, -0.5, 0.48), float3(-0.15, -0.5, 0.48), float3(0., -0.5, 0.5), float3(0., 0.5, 0.5), float3(0.15, 0.5, 0.48), float3(0., 0.5, 0.5), float3(0., -0.5, 0.5), float3(0., -0.5, 0.5), float3(0.15, -0.5, 0.48), float3(0.15, 0.5, 0.48), float3(0.29, 0.5, 0.4), float3(0.15, 0.5, 0.48), float3(0.15, -0.5, 0.48), float3(0.15, -0.5, 0.48), float3(0.29, -0.5, 0.4), float3(0.29, 0.5, 0.4), float3(0.4, 0.5, 0.29), float3(0.29, 0.5, 0.4), float3(0.29, -0.5, 0.4), float3(0.29, -0.5, 0.4), float3(0.4, -0.5, 0.29), float3(0.4, 0.5, 0.29), float3(0.48, 0.5, 0.15), float3(0.4, 0.5, 0.29), float3(0.4, -0.5, 0.29), float3(0.4, -0.5, 0.29), float3(0.48, -0.5, 0.15), float3(0.48, 0.5, 0.15), float3(0.5, 0.5, -0.), float3(0.48, 0.5, 0.15), float3(0.48, -0.5, 0.15), float3(0.48, -0.5, 0.15), float3(0.5, -0.5, -0.), float3(0.5, 0.5, -0.))


let MARK_COLOR = E3DCOLOR(0xFFFFFF00)
let WIRE_COLOR = E3DCOLOR(0xFFFFFF00)
let SOLID_COLOR = E3DCOLOR(0x60FFFF00)
let WIRE_UNDERCOLOR = E3DCOLOR(0xFFFF00FF)
let POINT_COLOR = E3DCOLOR(0xFFFFFF00)


def debug_draw_shape(shape : uint; norm_tm : float3x4; extent : float3; wire_color : E3DCOLOR; solid : bool)

  if shape == sound_hash("sphere")
    var tm = norm_tm
    let bounds = extent * 2.
    tm[0] *= bounds.x
    tm[1] *= bounds.x
    tm[2] *= bounds.x
    set_cached_debug_lines_wtm(tm)
    let center = float3(0., 0., 0.)
    draw_cached_debug_sphere(center, 0.5, wire_color, 16)
    if solid
      draw_cached_debug_trilist(SPHERE_FACES, SOLID_COLOR)

  elif shape == sound_hash("capsule")
    let radius = extent.x
    let diameter = extent.x * 2.
    let halfHeight = max(extent.y - radius, 0.)
    let height = halfHeight * 2.

    var tm = norm_tm
    tm[0] *= diameter
    tm[1] *= height
    tm[2] *= diameter
    set_cached_debug_lines_wtm(tm)
    draw_cached_debug_cylinder(IDENT_TM, 0.5, 1., wire_color)
    if solid
      draw_cached_debug_trilist(TUBE_FACES, SOLID_COLOR)

    tm = norm_tm
    tm[0] *= diameter
    tm[1] *= diameter
    tm[2] *= diameter
    tm[3] += norm_tm[1] * halfHeight
    set_cached_debug_lines_wtm(tm)
    let center = float3(0., 0., 0.)
    draw_cached_debug_sphere(center, 0.5, wire_color, 16)
    if solid
      draw_cached_debug_trilist(HEMISPHERE_FACES, SOLID_COLOR)

    tm[3] = norm_tm[3] - norm_tm[1] * halfHeight
    var mirrorTm = IDENT_TM
    mirrorTm[1] = -mirrorTm[1]
    tm = tm * mirrorTm
    set_cached_debug_lines_wtm(tm)
    draw_cached_debug_sphere(center, 0.5, wire_color, 16)
    if solid
      draw_cached_debug_trilist(HEMISPHERE_FACES, SOLID_COLOR)

  else//if shape == sound_hash("box")
    var tm = norm_tm
    let bounds = extent * 2.
    tm[0] *= bounds.x
    tm[1] *= bounds.y
    tm[2] *= bounds.z
    set_cached_debug_lines_wtm(tm)
    draw_cached_debug_box(BBox3(-float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5)), wire_color)
    if solid
      draw_cached_debug_trilist(BOX_FACES, SOLID_COLOR)


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawVolumes)]
def volume_ambient_sound_debug(evt : Event)

  query() <| $ [es] (eid : EntityId;
                     game_object_sound__name : string;
                     sound_shape__type : string;
                     var volume_ambient_sound__transform : float3x4&;
                     var volume_ambient_sound__extent_maxRadiusSq : float4&)

    query(eid) <| $ [es(REQUIRE=soundShapeDebug)] (transform : float3x4)
      volume_ambient_sound__transform = transform
      volume_ambient_sound__extent_maxRadiusSq.xyz = make_sound_shape(volume_ambient_sound__transform, sound_hash(sound_shape__type))

    add_debug_text_mark(volume_ambient_sound__transform[3], "{game_object_sound__name} {getEntityTemplateName(eid)}", -1, 0., MARK_COLOR)

    begin_draw_cached_debug_lines(false, false, false)
    debug_draw_shape(sound_hash(sound_shape__type), volume_ambient_sound__transform, volume_ambient_sound__extent_maxRadiusSq.xyz, WIRE_UNDERCOLOR, false)
    end_draw_cached_debug_lines()

    begin_draw_cached_debug_lines(true, false, false)
    debug_draw_shape(sound_hash(sound_shape__type), volume_ambient_sound__transform, volume_ambient_sound__extent_maxRadiusSq.xyz, WIRE_COLOR, true)
    end_draw_cached_debug_lines()


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawVolumes)]
def compound_ambient_sound_type_debug(evt : Event)
/*
  let debugObjectName = "sfx_point_transformator_a"
  begin_draw_cached_debug_lines(false, false, false)
  query() <| $ [es] (game_objects : GameObjects)
    let scene = game_objects |> get_scene_game_objects_by_name(debugObjectName)
    if scene != null
      var count = 0
      scene |> find_scene_game_objects() <| $ [unused_argument(ni, tm)] (ni : uint; tm : float3x4#)
        ++count
        return false
      var idx = 0
      scene |> find_scene_game_objects() <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
        let _tm = tm
        set_cached_debug_lines_wtm(_tm)
        draw_cached_debug_box(BBox3(-float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5)), WIRE_COLOR)
        add_debug_text_mark(_tm[3], "{debugObjectName} #{idx}/{count}", -1, 0., MARK_COLOR)
        ++idx
        return false
  end_draw_cached_debug_lines()
*/

  let radius = 400.
  let listener = get_listener_pos()
  let aa = listener - float3(radius, radius, radius)
  let bb = listener + float3(radius, radius, radius)
  let box = BBox3(aa, bb)

  begin_draw_cached_debug_lines(false, false, false)

  query() <| $ [es] (eid : EntityId; compound_ambient_sound_type__soundType : string)
    enum_sounds_in_box(box, sound_hash(compound_ambient_sound_type__soundType)) <| $(sound_shape : uint; extents : float3; tm : float3x4#)
      set_cached_debug_lines_wtm(tm)
      debug_draw_shape(sound_shape, get_norm_tm(tm, extents), extents, WIRE_UNDERCOLOR, false)
      let shape = sound_shape == sound_hash("box") ? "box" : sound_shape == sound_hash("capsule") ? "capsule" : sound_shape == sound_hash("sphere") ? "sphere" : "???"
      add_debug_text_mark(tm[3], "\"{compound_ambient_sound_type__soundType}\" ({shape}) {getEntityTemplateName(eid)}", -1, 0., MARK_COLOR)

  end_draw_cached_debug_lines()

  begin_draw_cached_debug_lines(true, false, false)

  query() <| $ [es] (eid : EntityId; compound_ambient_sound_type__soundType : string)
    enum_sounds_in_box(box, sound_hash(compound_ambient_sound_type__soundType)) <| $(sound_shape : uint; extents : float3; tm : float3x4#)
      set_cached_debug_lines_wtm(tm)
      debug_draw_shape(sound_shape, get_norm_tm(tm, extents), extents, SOLID_COLOR, true)
      let shape = sound_shape == sound_hash("box") ? "box" : sound_shape == sound_hash("capsule") ? "capsule" : sound_shape == sound_hash("sphere") ? "sphere" : "???"
      add_debug_text_mark(tm[3], "\"{compound_ambient_sound_type__soundType}\" ({shape}) {getEntityTemplateName(eid)}", -1, 0., MARK_COLOR)

  end_draw_cached_debug_lines()


[console_cmd(name="snd.print_sound_volumes")]
def sound_debug_print_sound_volumes()
  var text = "snd.print_sound_volumes\n"
  query() <| $ [es(REQUIRE=soundShapeDebug)] (eid : EntityId;
                                              game_object_sound__name : string;
                                              sound_shape__type : string;
                                              transform : float3x4)
    text = "{text}{eid} '{game_object_sound__name}':{sound_shape__type} {transform}\n"
  set_clipboard_ansi_text(text)
  logerr(text)


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawVolumes)]
def generated_sound_effect_debug(evt : Event)
  begin_draw_cached_debug_lines(false, false, false)
  var tm = IDENT_TM
  tm[0] *= 0.15
  tm[1] *= 0.15
  tm[2] *= 0.15
  query() <| $ [es(REQUIRE=generatedSoundEffect)] (transform : float3x4)
    tm[3] = transform[3]
    set_cached_debug_lines_wtm(tm)
    draw_cached_debug_trilist(BOX_FACES, POINT_COLOR)
  end_draw_cached_debug_lines()


[ecs_template]
struct sound_debug_draw_volumes
  soundDebugDrawVolumes : bool

[console_cmd(name="snd.debug_draw_volumes")]
def sound_debug_draw_volumes_console_cmd()
  var entEid : EntityId
  find_query() <| $ [es(REQUIRE=soundDebugDrawVolumes)] (eid : EntityId)
    entEid = eid
    return true
  if !entEid
    createEntity("sound_debug_draw_volumes")
    sound_debug("snd.debug_draw_volumes=1")
  else
    destroyEntity(entEid)
    sound_debug("snd.debug_draw_volumes=0")
