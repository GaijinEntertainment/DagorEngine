options gen2

require glfw/glfw_boost
require live/glfw_live
require opengl/opengl_boost
require opengl/opengl_gen
require opengl/opengl_cache
require daslib/defer
require daslib/safe_addr
require daslib/decs_boost
require live/decs_live
require live/opengl_live
require live/live_commands
require live/live_api
require daslib/json
require daslib/json_boost
require daslib/random
require daslib/math_boost
require live_host

// --- Constants ---

let LIGHT_DIRECTION = normalize(float3(-1, -4, 3))
let LIGHT_COLOR = UCOLOR_TO_RGB(0xffffff)
let LIGHT_BK_COLOR = UCOLOR_TO_RGB(0x202025)
let SHADOW_COLOR = UCOLOR_TO_RGB(0x101010)

let TANK_VEL = 2.0
let TANK_SHELL_VELOCITY = 4.0
let TANK_SHELL_COLOR = float3(1, 1, 0)
let TANK_TURRET_RELOAD_TIME = 0.3
let TANK_SHELL_SIZE = 0.12
let TANK_HEALTH = 3
let TANK_COLOR = UCOLOR_TO_RGB(0xffff20)

let TOWER_TURRET_RELOAD_TIME = 1.0
let TOWER_ROT_SPEED = 25.0 * PI / 180.0
let TOWER_SHELL_VELOCITY = 3.0
let TOWER_SHELL_COLOR = UCOLOR_TO_RGB(0xff00ff)
let TOWER_SHELL_SIZE = 0.18
let TOWER_HEALTH = 10
let TOWER_COLOR = UCOLOR_TO_RGB(0xff20ff)

let WALL_COLOR = UCOLOR_TO_RGB(0xff2020)
let BUILDING_COLOR = UCOLOR_TO_RGB(0xDABB98)

let KABOOM_TIME = 0.2
let KABOOM_COLOR = UCOLOR_TO_RGB(0xFF4800)

let PULSE_MIN_SCALE = 0.85
let PULSE_MAX_SCALE = 0.95

let FADE_MIN_SCALE = 0.4
let FADE_MAX_SCALE = 0.6
let FADE_TIME = 4.0
let FADE_VEL = 1.0 / FADE_TIME

var seed = random_seed(13)

let MAX_POINT_LIGHTS = 8

struct PointLight {
    xyz_r : float4
    color : float4
}

struct PointLights {
    lights  : PointLight[MAX_POINT_LIGHTS]
    nLights : int
}

// --- Shaders ---

var @in @location = 0 v_position : float3
var @in @location = 1 v_normal   : float3
var @in @location = 2 v_texcoord : float2
var @uniform v_model : float4x4
var @uniform v_view : float4x4
var @uniform v_projection : float4x4
var @inout f_texcoord : float2
var @inout f_normal : float3
var @inout f_position : float3
var @uniform f_Color : float4
var @out f_outputColor : float4
var @ssbo@stage = 1 c_lights : PointLights
var @out f_FragColor : float4

[vertex_program]
def vs_preview {
    f_texcoord = v_texcoord
    f_normal = normalize(float3x3(v_model) * v_normal)
    let tpos = v_model * float4(v_position, 1.0)
    f_position = tpos.xyz
    gl_Position = v_projection * v_view * tpos
}

[fragment_program(version=430)]
def ps_preview {
    let camera_dir = normalize(f_position)
    let normal = normalize(f_normal)
    let k = saturate(-dot(LIGHT_DIRECTION, normal))
    let k2 = saturate(dot(LIGHT_DIRECTION, normal))
    var diffuse = float3(0)
    for (i in range(c_lights.nLights)) {
        let xyz_r = c_lights.lights[i].xyz_r
        let dist = distance(xyz_r.xyz, f_position)
        let attn = 1.0 - saturate(dist / xyz_r.w)
        diffuse += attn * c_lights.lights[i].color.xyz
    }
    f_outputColor.xyz = f_Color.xyz * (LIGHT_COLOR * k + LIGHT_BK_COLOR * k2) + diffuse
    f_outputColor.w = f_Color.w
}

[fragment_program]
def ps_shadow_preview {
    f_FragColor.xyz = SHADOW_COLOR
    f_FragColor.w = f_Color.w
}

// --- GL objects ---

var program, shadow_program : uint
var ssbo : uint
var cube, sphere, cylinder : OpenGLGeometryFragment
var ground : OpenGLGeometryFragment
var tex : uint
var cursor : GLFWcursor?

// --- Obstacle / Primitive enums ---

bitfield Obstacle {
    wall
    tank
    tower
}

enum Primitive {
    cube
    cylinder
}

// --- DECS templates ---

[decs_template]
struct KeyControls {
    key_up, key_down, key_left, key_right : bool
}

[decs_template]
struct MouseControls {
    mouse_pos     : float2
    mouse_buttons : bitfield<left; center; right>
}

[decs_template(prefix)]
struct Object {
    eid      : EntityId
    pos      : float3
    dim      : float3
    scale    : float
    color    : float3
    obstacle : Obstacle
    prim     : Primitive
}

[decs_template]
struct Turret {
    base        : EntityId
    last_target : float3
    dir         : float
    dir_vel     : float
    rot_speed   : float
    reload      : float
    reload_time : float
    health      : int
}

[decs_template]
struct Projectile {
    pos        : float3
    vel        : float3
    color      : float3
    size       : float
    projectile : bool
    targets    : Obstacle
}

[decs_template]
struct Kaboom {
    pos        : float3
    vel        : float3
    radius     : float
    radius_vel : float
    time       : float
    time_max   : float
    color      : float3
    damage     : bool
    light_f    : float
}

[decs_template]
struct PickUpItem {
    eid          : EntityId
    pos          : float3
    color        : float3
    fade         : float
    fade_vel     : float
    rot_t        : float
    rot_total    : float
    health_bonus : int
}

// --- Entity factories ---

def make_tank(pos : float3) {
    var tank_id = create_entity <| @(eid, cmp) {
        cmp |> set("vel", float3(0.0))
        cmp |> set("pulse", -0.25)
        cmp |> set("tank", true)
        cmp |> set("alive", true)
        apply_decs_template(cmp, Object(
            eid = eid,
            pos = pos,
            dim = float3(1.0),
            scale = 0.95,
            color = TANK_COLOR,
            obstacle = Obstacle.tank,
            prim = Primitive.cube
        ))
        apply_decs_template(cmp, KeyControls())
    }
    create_entity <| @(eid, cmp) {
        cmp |> set("eid", eid)
        cmp |> set("pos", float3(0.0))
        cmp |> set("color", float3(0.5, 0.5, 0.1))
        cmp |> set("name", "TANK")
        apply_decs_template(cmp, Turret(
            base = tank_id,
            reload_time = TANK_TURRET_RELOAD_TIME,
            health = TANK_HEALTH))
        apply_decs_template(cmp, MouseControls())
    }
}

def make_tower(pos : float3) {
    var tower_id = create_entity <| @(eid, cmp) {
        cmp |> set("tower", true)
        cmp |> set("alive", true)
        apply_decs_template(cmp, Object(
            eid = eid,
            pos = pos,
            dim = float3(1.0),
            scale = 0.95,
            color = TOWER_COLOR,
            obstacle = Obstacle.tower,
            prim = Primitive.cylinder
        ))
    }
    create_entity <| @(eid, cmp) {
        cmp |> set("eid", eid)
        cmp |> set("pos", float3(0.0))
        cmp |> set("color", float3(0.2, 0.2, 0.5))
        cmp |> set("name", "TOWER")
        apply_decs_template(cmp, Turret(
            base = tower_id,
            reload_time = TOWER_TURRET_RELOAD_TIME,
            rot_speed = TOWER_ROT_SPEED,
            health = TOWER_HEALTH
        ))
        cmp |> set("ai_turret", true)
    }
}

def make_wall(pos : float3; height : float) {
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, Object(
            eid = eid,
            pos = pos,
            dim = float3(1.0, height, 1.0),
            scale = 0.95,
            color = WALL_COLOR,
            obstacle = Obstacle.wall,
            prim = Primitive.cube
        ))
    }
}

def make_bonus(pos : float3) {
    create_entity <| @(eid, cmp) {
        apply_decs_template(cmp, PickUpItem(
            eid = eid,
            pos = pos,
            color = UCOLOR_TO_RGB(0x0000BB),
            fade = FADE_MIN_SCALE,
            fade_vel = FADE_VEL,
            rot_total = 2.0,
            health_bonus = 3
        ))
    }
}

def make_building_congregation(n : int; center : float3; radius : float; minDim, maxDim : float3) {
    for (t in range(n)) {
        var pos = random_in_unit_disk(seed).xzy * radius + center
        let dim = minDim + (maxDim - minDim) * random_float4(seed).xyz
        let color = BUILDING_COLOR * (random_float(seed) * 0.4 + 0.4)
        create_entity <| @(eid, cmp) {
            apply_decs_template(cmp, Object(
                eid = eid,
                pos = pos,
                dim = dim,
                scale = 1.0,
                color = color,
                obstacle = Obstacle.wall,
                prim = Primitive.cube
            ))
        }
    }
}

def make_level_2 {
    for (x in range(-10, 10)) {
        for (z in range(-10, 10)) {
            if (x == -10 || x == 9 || z == -10 || z == 9) {
                make_wall(float3(x, 0, z), 2.0)
            }
        }
    }
    make_building_congregation(
        12,
        float3(0),
        3.5,
        float3(1, 1, 1),
        float3(2, 3, 2)
    )
    make_bonus(float3(-7, 0, -5))
    make_tank(float3(-8, 0, -8))
    make_tower(float3(7, 0, 7))
    make_tower(float3(0, 0, -6))
    commit()
}

// --- Drawing helpers ---

def draw_sphere(pos : float3; scale : float; color : float3; alpha : float = 1.0) {
    v_model = compose(float3(pos), float4(0.0), float3(scale))
    f_Color.xyz = color
    f_Color.w = alpha
    vs_preview_bind_uniform(program)
    ps_preview_bind_uniform(program)
    sphere |> draw_geometry_fragment()
}

def draw_ground_plane(pos : float3; scale : float; color : float3) {
    v_model = compose(float3(pos), float4(0.0), float3(scale))
    f_Color.xyz = color
    f_Color.w = 1.0
    vs_preview_bind_uniform(program)
    ps_preview_bind_uniform(program)
    ground |> draw_geometry_fragment()
}

def draw_cylinder(tm : float4x4; color : float3) {
    v_model = tm
    f_Color.xyz = color
    f_Color.w = 1.0
    vs_preview_bind_uniform(program)
    ps_preview_bind_uniform(program)
    cylinder |> draw_geometry_fragment()
}

def draw_cylinder(pos : float3; scale : float3; color : float3) {
    let tm = compose(float3(pos), float4(0), float3(scale))
    draw_cylinder(tm, color)
}

// --- Game state ---

var tick_dt = 1.0
var display_w = 0
var display_h = 0
var key_up = false
var key_down = false
var key_left = false
var key_right = false
var mouse_x = 0.0
var mouse_y = 0.0
var mouse_buttons : uint

// --- DECS update systems ---

[decs(stage=update)]
def update_pickup_items(var p : PickUpItem) {
    p.rot_t += tick_dt
    if (p.rot_t > p.rot_total) {
        p.rot_t -= p.rot_total
    }
    p.fade += p.fade_vel * tick_dt
    if (p.fade < FADE_MIN_SCALE) {
        p.fade = FADE_MIN_SCALE
        p.fade_vel = -p.fade_vel
    } elif (p.fade > FADE_MAX_SCALE) {
        p.fade = FADE_MAX_SCALE
        p.fade_vel = -p.fade_vel
    }
    var trect = AABR(
        min = float2(p.pos.x - 0.25, p.pos.z - 0.25),
        max = float2(p.pos.x + 0.25, p.pos.z + 0.25)
    )
    var any = false
    query <| $(pos : float3; var Turret_health : int&) {
        let wrect = AABR(
            min = float2(pos.x - 0.5, pos.z - 0.5),
            max = float2(pos.x + 0.5, pos.z + 0.5)
        )
        if (is_intersecting(trect, wrect)) {
            Turret_health += p.health_bonus
            any = true
        }
    }
    if (any) {
        for (i in range(5)) {
            make_sparks(p.pos, p.color)
        }
        delete_entity(p.eid)
    }
}

[decs(stage=update_controls)]
def update_turret_direction(pos : float3; var turret : Turret; var ctrl : MouseControls) {
    if (turret.health == 0) {
        return
    }
    var pos4d = v_projection * v_view * float4(pos.x, pos.y + 1.25, pos.z, 1.0)
    pos4d = pos4d / pos4d.w
    let pos2d = (pos4d.xy * float2(0.5, -0.5) + float2(0.5)) * float2(display_w, display_h)
    let dxy = ctrl.mouse_pos - pos2d
    let angle = atan2(-dxy.y, dxy.x)
    turret.dir = angle
}

def fire_turret(var turret_reload : float& implicit; turret_reload_time, turret_dir : float; pos : float3;
                velocity : float; color : float3; size : float; targets : Obstacle) {
    if (turret_reload != 0.0) {
        return
    }
    turret_reload = turret_reload_time
    create_entity <| @(eid, cmp) {
        cmp |> set("eid", eid)
        let qr = quat_from_unit_vec_ang(float3(0, 1, 0), turret_dir)
        let vel = quat_mul_vec(qr, float3(1.0, 0.0, 0.0))
        apply_decs_template(cmp, Projectile(
            pos = pos + float3(0.0, 0.625, 0.0) + vel * 0.5,
            vel = vel * velocity,
            color = color,
            size = size,
            targets = targets
        ))
    }
}

[decs(stage=update_controls)]
def update_turret_cannon(pos : float3; var turret : Turret; var ctrl : MouseControls) {
    if (turret.health == 0) {
        return
    }
    if (ctrl.mouse_buttons.left) {
        fire_turret(turret.reload, turret.reload_time, turret.dir, pos,
            TANK_SHELL_VELOCITY, TANK_SHELL_COLOR, TANK_SHELL_SIZE,
                Obstacle.wall | Obstacle.tower)
    }
}

def can_see(src_pos, tgt_pos : float3) {
    let dist = distance(tgt_pos, src_pos)
    let ray = Ray(origin = src_pos, dir = normalize(tgt_pos - src_pos))
    let found = find_query <| $(pos, dim : float3; obstacle : Obstacle) {
        if (!obstacle.wall) {
            return false
        }
        let aabb = AABB(min = pos - dim * 0.5, max = pos + dim * 0.5)
        if (is_intersecting(ray, aabb, 0.1, dist)) {
            return true
        }
    }
    return !found
}

def set_pulse(base : EntityId implicit; on : bool) {
    if (on) {
        base |> update_entity <| @(eidd, cmp) {
            if (!cmp |> has("pulse")) {
                cmp |> set("pulse", 0.25)
            }
        }
    } else {
        base |> update_entity <| @(eidd, cmp) {
            cmp |> remove("pulse")
        }
    }
}

[decs(stage=update_ai, REQUIRE=ai_turret)]
def update_ai(eid : EntityId; var turret : Turret; pos : float3) {
    if (turret.health == 0) {
        return
    }
    var target : float3
    let found = find_query <| $ [REQUIRE(tank)] (pos : float3) {
        target = pos
        return true
    }
    if (!found) {
        return
    }
    let seeTarget = can_see(pos, target)
    if (seeTarget) {
        turret.last_target = target
        turret.base |> set_pulse(true)
    } else {
        turret.base |> set_pulse(false)
    }
    let dxy = normalize((turret.last_target - pos).xz)
    var angle = atan2(-dxy.y, dxy.x)
    var A = angle - turret.dir
    if (A > PI) {
        A -= 2.0 * PI
    }
    if (A < -PI) {
        A += 2.0 * PI
    }
    if (A > 0.0) {
        turret.dir_vel = turret.rot_speed
    } else {
        turret.dir_vel = -turret.rot_speed
    }
    if (seeTarget && turret.reload == 0.0 && abs(A) < PI / 180.0) {
        fire_turret(turret.reload, turret.reload_time, turret.dir, pos,
            TOWER_SHELL_VELOCITY, TOWER_SHELL_COLOR, TOWER_SHELL_SIZE,
                Obstacle.wall | Obstacle.tank)
    }
}

[decs(stage=update)]
def update_turret_reload(var turret : Turret) {
    turret.reload = max(turret.reload - tick_dt, 0.0)
}

def make_smoke(pos : float3) {
    let smkt = random_float(seed) * 1.5 + 1.5
    let smkr = 0.1 + random_float(seed) * 0.1
    let smkrv = 0.2 + random_float(seed) * 0.1
    create_entity <| @(eidd, cmp) {
        cmp |> set("eid", eidd)
        apply_decs_template(cmp, Kaboom(
            pos = pos + random_in_unit_disk(seed) * 0.3,
            vel = float3(0.0, 1.0 + random_float(seed), 0.0),
            radius = smkr,
            radius_vel = smkrv,
            time = smkt,
            time_max = smkt,
            color = float3(0.2, 0.2, 0.2),
            damage = false
        ))
    }
}

def make_sparks(pos : float3; bonus_color : float3) {
    let smkt = random_float(seed) * 1.5 + 1.5
    let smkr = 0.2 + random_float(seed) * 0.1
    let smkrv = -0.3
    create_entity <| @(eidd, cmp) {
        cmp |> set("eid", eidd)
        var vel = random_in_unit_sphere(seed) * 6.0
        vel.y = abs(vel.y)
        apply_decs_template(cmp, Kaboom(
            pos = pos + random_in_unit_disk(seed) * 0.3,
            vel = vel,
            radius = smkr,
            radius_vel = smkrv,
            time = smkt,
            time_max = smkt,
            color = bonus_color,
            damage = false,
            light_f = 10.0
        ))
    }
}

[decs(stage=update)]
def update_turret_smoke(pos : float3; var turret : Turret) {
    if (turret.health == 0) {
        let smoke_rate = 5.0
        let smoke_chance = tick_dt * smoke_rate
        if (random_float(seed) < smoke_chance) {
            make_smoke(pos)
        }
    }
}

[decs(stage=update)]
def update_turret_rotation(var turret : Turret) {
    turret.dir += turret.dir_vel * tick_dt
    if (turret.dir < 0.0) {
        turret.dir += 2.0 * PI
    }
    if (turret.dir > 2.0 * PI) {
        turret.dir -= 2.0 * PI
    }
}

[decs(stage=update_controls)]
def update_tank_controls(eid : EntityId; var pos, vel : float3&; var ctrl : KeyControls; alive : bool) {
    if (!alive) {
        return
    }
    if (ctrl.key_up) {
        vel = vel.z > 0.0 ? float3(0.0) : float3(0.0, 0.0, -TANK_VEL)
        ctrl.key_up = false
    }
    if (ctrl.key_down) {
        vel = vel.z < 0.0 ? float3(0.0) : float3(0.0, 0.0, +TANK_VEL)
        ctrl.key_down = false
    }
    if (ctrl.key_left) {
        vel = vel.x > 0.0 ? float3(0.0) : float3(-TANK_VEL, 0.0, 0.0)
        ctrl.key_left = false
    }
    if (ctrl.key_right) {
        vel = vel.x < 0.0 ? float3(0.0) : float3(TANK_VEL, 0.0, 0.0)
        ctrl.key_right = false
    }
}

[decs(stage=update)]
def update_tank(eid : EntityId; var pos, dim, vel : float3&) {
    var new_pos = pos + vel * tick_dt
    var trect = AABR(
        min = new_pos.xz - dim.xz * 0.45,
        max = new_pos.xz + dim.xz * 0.45
    )
    let collide = find_query <| $(pos, dim : float3; obstacle : Obstacle) {
        if (obstacle.tank) {
            return false
        }
        let wrect = AABR(
            min = pos.xz - dim.xz * 0.5,
            max = pos.xz + dim.xz * 0.5
        )
        if (is_intersecting(trect, wrect)) {
            return true
        }
    }
    if (!collide) {
        pos = new_pos
    }
    pos.x = clamp(pos.x, -10.0, 9.0)
    pos.z = clamp(pos.z, -10.0, 9.0)
}

[decs(stage=update)]
def update_pulses(var scale, pulse : float&) {
    scale += pulse * tick_dt
    if (scale < PULSE_MIN_SCALE) {
        scale = PULSE_MIN_SCALE + (PULSE_MIN_SCALE - scale)
        pulse = -pulse
    } elif (scale > PULSE_MAX_SCALE) {
        scale = PULSE_MAX_SCALE - (scale - PULSE_MAX_SCALE)
        pulse = -pulse
    }
}

[decs(stage=update)]
def update_projectile(eid : EntityId; var proj : Projectile) {
    proj.pos += proj.vel * tick_dt
    var trect = AABR(
        min = float2(proj.pos.x - proj.size, proj.pos.z - proj.size),
        max = float2(proj.pos.x + proj.size, proj.pos.z + proj.size)
    )
    let collide = find_query <| $(pos : float3; obstacle : Obstacle) {
        if ((proj.targets & obstacle) == bitfield(0)) {
            return false
        }
        let wrect = AABR(
            min = float2(pos.x - 0.5, pos.z - 0.5),
            max = float2(pos.x + 0.5, pos.z + 0.5)
        )
        if (is_intersecting(trect, wrect)) {
            return true
        }
    }
    if (collide) {
        create_entity <| @(eidd, cmp) {
            cmp |> set("eid", eidd)
            apply_decs_template(cmp, Kaboom(
                pos = proj.pos,
                radius = 0.0,
                radius_vel = (proj.size * proj.size) * 150.0,
                time = KABOOM_TIME,
                time_max = KABOOM_TIME,
                color = KABOOM_COLOR,
                damage = true,
                light_f = 6.0
            ))
        }
        for (t in range(5)) {
            make_smoke(proj.pos)
        }
    }
    if (collide || proj.pos.x < -12.0 || proj.pos.x > 12.0 || proj.pos.z < -12.0 || proj.pos.z > 12.0) {
        delete_entity(eid)
    }
}

[decs(stage=update)]
def update_kaboom(eid : EntityId; var kaboom : Kaboom) {
    kaboom.pos += kaboom.vel * tick_dt
    kaboom.radius = max(0.0, kaboom.radius + kaboom.radius_vel * tick_dt)
    kaboom.time -= tick_dt
    if (kaboom.time < 0.0) {
        if (kaboom.damage) {
            var trect = AABR(
                min = float2(kaboom.pos.x - kaboom.radius, kaboom.pos.z - kaboom.radius),
                max = float2(kaboom.pos.x + kaboom.radius, kaboom.pos.z + kaboom.radius)
            )
            query <| $(pos : float3; var Turret_health : int&; Turret_base : EntityId) {
                let wrect = AABR(
                    min = float2(pos.x - 0.5, pos.z - 0.5),
                    max = float2(pos.x + 0.5, pos.z + 0.5)
                )
                if (is_intersecting(trect, wrect)) {
                    Turret_health = max(Turret_health - 1, 0)
                    if (Turret_health == 0) {
                        Turret_base |> set_pulse(false)
                        query(Turret_base) <| $(var alive : bool&) {
                            alive = false
                        }
                    }
                }
            }
        }
        delete_entity(eid)
    }
}

[decs(stage=update_attachments)]
def update_turrets(eid : EntityId; var pos : float3&; turret : Turret) {
    var turret_pos & = pos
    let upd = query(turret.base) <| $(pos, dim : float3) {
        turret_pos = pos + float3(0.0, dim.y * 0.5, 0.0)
    }
    if (!upd) {
        delete_entity(eid)
    }
}

// --- DECS draw systems ---

[decs(stage=draw)]
def draw_objects(obj : Object) {
    let ofs = obj.pos + float3(0.0, obj.dim.y * 0.5, 0.0)
    let scl = obj.dim * (0.5 * obj.scale)
    v_model = compose(ofs.xyz, float4(0), scl.xyz)
    f_Color.xyz = obj.color
    f_Color.w = 1.0
    vs_preview_bind_uniform(program)
    ps_preview_bind_uniform(program)
    if (obj.prim == Primitive.cube) {
        cube |> draw_geometry_fragment()
    } elif (obj.prim == Primitive.cylinder) {
        cylinder |> draw_geometry_fragment()
    }
}

[decs(stage=draw_pickup_items)]
def draw_pickup_items(var p : PickUpItem) {
    let rot_a = quat_from_unit_vec_ang(float3(1, 0, 0), radians(45.0))
    let rot_b = quat_from_unit_vec_ang(float3(0, 1, 0), radians(45.0))
    let rot_t = quat_from_unit_vec_ang(float3(0, 1, 0), 2.0 * PI * (p.rot_t / p.rot_total))
    let rot_q = quat_mul(rot_t, quat_mul(rot_a, rot_b))
    let tm_scale = compose(float3(0), float4(0), float3(0.15, 0.5, 0.15))
    let tm_rot = compose(float3(0), rot_q, float3(1))
    let tm_pos = compose(float3(p.pos.x, p.pos.y + 0.25, p.pos.z), float4(0), float3(1))
    v_model = tm_pos * tm_scale * tm_rot
    f_Color.xyz = p.color
    f_Color.w = p.fade
    vs_preview_bind_uniform(program)
    ps_preview_bind_uniform(program)
    cube |> draw_geometry_fragment()
}

[decs(stage=draw)]
def draw_projectiles(var proj : Projectile) {
    draw_sphere(proj.pos, proj.size, proj.color)
}

[decs(stage=draw_transparent)]
def draw_kaboom(var kaboom : Kaboom) {
    draw_sphere(kaboom.pos, kaboom.radius, kaboom.color, kaboom.time / kaboom.time_max)
}

[decs(stage=update_lights)]
def update_kaboom_light(var kaboom : Kaboom) {
    if (c_lights.nLights < MAX_POINT_LIGHTS && kaboom.light_f > 0.0) {
        c_lights.lights[c_lights.nLights].xyz_r = float4(kaboom.pos, kaboom.radius * kaboom.light_f)
        c_lights.lights[c_lights.nLights].color = float4(kaboom.color, 1.0)
        c_lights.nLights ++
    }
}

[decs(stage=update_lights)]
def update_pickup_light(var p : PickUpItem) {
    if (c_lights.nLights < MAX_POINT_LIGHTS) {
        c_lights.lights[c_lights.nLights].xyz_r = float4(p.pos, 1.5 + p.fade)
        c_lights.lights[c_lights.nLights].color = float4(p.color, 1.0)
        c_lights.nLights ++
    }
}

[decs(stage=draw)]
def draw_turrets(pos : float3; color : float3; turret : Turret) {
    draw_cylinder(pos + float3(0.0, 0.5, 0.0), float3(0.25, 0.25, 0.25), color)
    let cannon_recoil = 1.0 - turret.reload / turret.reload_time
    let rot_a = quat_from_unit_vec_ang(float3(0, 1, 0), turret.dir)
    let m_rota = compose(float3(0), rot_a, float3(1))
    let m_scale = compose(float3(0), float4(0), float3(0.125, 0.3, 0.125))
    let m_ofsa = compose(float3(0.3 + 0.2 * cannon_recoil, 0.6, 0.0), float4(0), float3(1))
    let rotSide = quat_from_unit_vec_ang(float3(0, 0, 1), PI / 2.0)
    let m_rot = compose(float3(0), rotSide, float3(1))
    let m_pos = compose(pos, float4(0), float3(1))
    draw_cylinder(m_pos * m_rota * m_ofsa * m_rot * m_scale, color)
}

// --- Input DECS stages ---

[decs(stage=key_controls_changed)]
def update_key_controls(var ctr : KeyControls) {
    ctr.key_up = key_up
    ctr.key_down = key_down
    ctr.key_left = key_left
    ctr.key_right = key_right
}

[decs(stage=mouse_controls_changed)]
def update_mouse_controls(var ctr : MouseControls) {
    ctr.mouse_pos = float2(mouse_x, mouse_y)
    ctr.mouse_buttons = bitfield(mouse_buttons)
}

// --- Live commands ---

[live_command]
def game_status(input : JsonValue?) : JsonValue? {
    var result = "\{"
    var first = true
    query() <| $(name : string; Turret_health : int) {
        if (!first) {
            result += ", "
        }
        result += "\"" + name + "\": " + string(Turret_health)
        first = false
    }
    result += "}"
    return JV(result)
}

// --- Rendering ---

def update_lights {
    c_lights.nLights = 0
    decs_stage("update_lights")
}

def draw_ground {
    glUseProgram(program)
    glDisable(GL_CULL_FACE)
    glDisable(GL_DEPTH_TEST)
    glDepthMask(false)
    write_ssbo(ssbo, c_lights)
    draw_ground_plane(float3(0.0, -0.5, 0.0), 12.0, float3(0.3, 0.2, 0.0))
    glEnable(GL_CULL_FACE)
    glEnable(GL_DEPTH_TEST)
    glDepthMask(true)
    glUseProgram(0u)
}

def draw_scene {
    glUseProgram(program)
    write_ssbo(ssbo, c_lights)
    decs_stage("draw")
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glDepthMask(false)
    glEnable(GL_DEPTH_TEST)
    decs_stage("draw_pickup_items")
    glDisable(GL_DEPTH_TEST)
    decs_stage("draw_transparent")
    glDepthMask(true)
    glEnable(GL_DEPTH_TEST)
    glDisable(GL_BLEND)
    glUseProgram(0u)
}

def draw_shadow {
    glUseProgram(shadow_program)
    glDisable(GL_DEPTH_TEST)
    decs_stage("draw")
    glEnable(GL_DEPTH_TEST)
    glUseProgram(0u)
}

// --- GL init/cleanup ---

def create_gl_objects() {
    cursor = glfwCreateStandardCursor(int(GLFW_CROSSHAIR_CURSOR))
    if (live_window != null) {
        glfwSetCursor(live_window, cursor)
    }
    cube <- create_geometry_fragment <| gen_cube()
    sphere <- create_geometry_fragment <| gen_sphere(32, 16, false)
    ground <- create_geometry_fragment <| gen_plane(GenDirection.xz)
    cylinder <- create_geometry_fragment <| gen_cylinder(GenDirection.xz, 32)
    tex = create_texture <| gen_image_checkerboard(16, 16, 0xff404040, 0xff808080)
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
    glBindTexture(GL_TEXTURE_2D, 0u)
    program = cache_shader_program(vs_preview`shader_text, ps_preview`shader_text)
    shadow_program = cache_shader_program(vs_preview`shader_text, ps_shadow_preview`shader_text)
    ssbo = create_ssbo(c_lights)
}

def delete_gl_objects() {
    if (cursor != null) {
        glfwDestroyCursor(cursor)
        cursor = null
    }
    glDeleteTextures(1, safe_addr(tex))
    delete cube
    delete sphere
    delete ground
    delete cylinder
}

// --- Input callbacks ---

def setup_input_callbacks() {
    if (live_window == null) {
        return
    }
    glfwSetKeyCallback(live_window) <| @(w, kk, scancode, action, mods) {
        if (action == 1) {
            key_up    = scancode == 328 || scancode == 17
            key_down  = scancode == 336 || scancode == 31
            key_left  = scancode == 331 || scancode == 30
            key_right = scancode == 333 || scancode == 32
            decs_stage("key_controls_changed")
        }
    }
    glfwSetCursorPosCallback(live_window) <| @(w, x, y) {
        mouse_x = float(x)
        mouse_y = float(y)
        decs_stage("mouse_controls_changed")
    }
    glfwSetMouseButtonCallback(live_window) <| @(w, button, action, mods) {
        if (action != 0) {
            mouse_buttons |= uint(1 << button)
        } else {
            mouse_buttons &= ~uint(1 << button)
        }
        decs_stage("mouse_controls_changed")
    }
}

// --- Lifecycle ---

[export]
def init() {
    live_create_window("Tank Game", 1024, 768)
    create_gl_objects()
    setup_input_callbacks()
    if (!is_reload()) {
        decs::restart()
        make_level_2()
    }
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    let dt = get_dt()
    tick_dt = min(dt, 1.0 / 30.0)
    decs_stage("update_controls")
    decs_stage("update_ai")
    decs_stage("update")
    decs_stage("update_attachments")
    // Display
    live_get_framebuffer_size(display_w, display_h)
    glViewport(0, 0, display_w, display_h)
    glClearColor(0.1, 0.1, 0.1, 1.0)
    glClearDepth(1.0lf)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)
    glEnable(GL_CULL_FACE)
    glCullFace(GL_BACK)
    let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.0
    let tm_camera = look_at_rh(
        float3(0.0, 25.0, 25.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0)
    )
    let tm_shadow = planar_shadow(
        float4(LIGHT_DIRECTION, 0.0),
        plane_from_point_normal(float3(0.0), float3(0.0, 1.0, 0.0))
    )
    v_projection = perspective_rh_opengl(45.0f * PI / 180.0, aspect, 0.1f, 200.0f)
    update_lights()
    v_view = tm_camera
    draw_ground()
    v_view = tm_camera * tm_shadow
    draw_shadow()
    v_view = tm_camera
    draw_scene()
    glDisable(GL_CULL_FACE)
    bind_ffp()
    live_end_frame()
}

[export]
def shutdown() {
    delete_gl_objects()
    live_destroy_window()
}

// Dual-mode: also works with regular daslang.exe
[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
