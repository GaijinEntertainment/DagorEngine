// options log
// options logCpp
// options log_nodes

module path_tracer public

require math
require strings

require daslib/random

let
    DO_SAMPLES_PER_PIXEL = 32
    LIGHT_SPHERE_ID = 8

let
    kMinT = 0.001
    kMaxT = 1.0e7f
    kMaxDepth = 10

var RAND_SEED : int4

def schlick(cosine,ri:float)
    var r0 = (1. - ri) / (1. + ri)
    return lerp ( r0*r0, 1., pow(1. - cosine,5.))

struct Sphere
    center : float3
    radius : float
    radius2 : float
    iRadius : float
    pad1,pad2 : float

struct Ray
    orig : float3
    pad1 : float
    dir  : float3
    pad2 : float

def pointAt(ray:Ray const;t:float)
    return ray.orig + ray.dir*t

struct Hit
    pos    : float3
    pad    : float
    normal : float3
    t      : float

struct Camera
    origin          : float3
    lowerLeftCorner : float3
    horizontal      : float3
    vertical        : float3
    u, v, w         : float3
    lensRadius      : float

def Camera(lookFrom,lookAt,vup:float3;vfov,aspect,aperture,focusDist:float)
    var that : Camera
    let theta = vfov * PI / 180.
    let halfHeight = tan(theta / 2.)
    let halfWidth = aspect * halfHeight
    with that
        lensRadius = aperture / 2.
        origin = lookFrom
        w = fast_normalize(lookFrom - lookAt)
        u = fast_normalize(cross(vup, w))
        v = cross(w, u)
        lowerLeftCorner = origin - halfWidth*focusDist*u - halfHeight*focusDist*v - focusDist*w
        horizontal = 2. * halfWidth * focusDist * u
        vertical = 2. * halfHeight * focusDist * v
    return that

def getRay(that:Camera;s,t:float)
    with that
        let rd = lensRadius * random_in_unit_disk(RAND_SEED)
        let offset = mad(u,rd.x,v*rd.y)
        return [[Ray orig = origin + offset,
            dir = fast_normalize(mad(vertical,t,mad(horizontal,s,lowerLeftCorner)) - origin - offset) ]]

enum Type
    Lambert
    Metal
    Dielectric

struct Material
    mtype : Type
    albedo : float3
    emissive : float3
    roughness : float
    ri : float

let TOTAL_SPHERES = 9

var [[groupshared]] s_Spheres : Sphere[TOTAL_SPHERES]
var [[groupshared]] s_SphereMats : Material[TOTAL_SPHERES]

def init_path_tracer
    s_Spheres[0] = [[Sphere center = float3(0.,-100.5,-1.), radius = 100.,  radius2=100.0*100., iRadius = 1. / 100.]]
    s_Spheres[1] = [[Sphere center = float3(2.,0.,-1.),     radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[2] = [[Sphere center = float3(0.,0.,-1.),     radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[3] = [[Sphere center = float3(-2.,0.,-1.),    radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[4] = [[Sphere center = float3(2.,0.,1.),      radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[5] = [[Sphere center = float3(0.,0.,1.),      radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[6] = [[Sphere center = float3(-2.,0.,1.),     radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[7] = [[Sphere center = float3(0.5,1.,0.5),    radius = 0.5,   radius2=0.5*0.5,    iRadius = 1. / .5]]
    s_Spheres[8] = [[Sphere center = float3(-1.5,1.5,0.),   radius = 0.3,   radius2=0.3*0.3,    iRadius = 1. / .3]]
    s_SphereMats[0] = [[Material mtype = Type Lambert,    albedo = float3(0.8, 0.8, 0.8), emissive = float3(0,0,0),     roughness = 0.,  ri = 0.]]
    s_SphereMats[1] = [[Material mtype = Type Lambert,    albedo = float3(0.8, 0.4, 0.4), emissive = float3(0,0,0),     roughness = 0.,  ri = 0.]]
    s_SphereMats[2] = [[Material mtype = Type Lambert,    albedo = float3(0.4, 0.8, 0.4), emissive = float3(0,0,0),     roughness = 0.,  ri = 0.]]
    s_SphereMats[3] = [[Material mtype = Type Metal,      albedo = float3(0.4, 0.4, 0.8), emissive = float3(0,0,0),     roughness = 0.,  ri = 0.]]
    s_SphereMats[4] = [[Material mtype = Type Metal,      albedo = float3(0.4, 0.8, 0.4), emissive = float3(0,0,0),     roughness = 0.,  ri = 0.]]
    s_SphereMats[5] = [[Material mtype = Type Metal,      albedo = float3(0.4, 0.8, 0.4), emissive = float3(0,0,0),     roughness = 0.2, ri = 0.]]
    s_SphereMats[6] = [[Material mtype = Type Metal,      albedo = float3(0.4, 0.8, 0.4), emissive = float3(0,0,0),     roughness = 0.6, ri = 0.]]
    s_SphereMats[7] = [[Material mtype = Type Dielectric, albedo = float3(0.4, 0.4, 0.4), emissive = float3(0,0,0),     roughness = 0.,  ri = 1.5]]
    s_SphereMats[8] = [[Material mtype = Type Lambert,    albedo = float3(0.8, 0.6, 0.2), emissive = float3(30,25,15),  roughness = 0.,  ri = 0.]]

def hitSpheres(r:Ray const;tMin,tMax:float;var outHit:Hit)
    var hitT = tMax
    var id = -1
    var i = 0
    for sph in s_Spheres
        let co = sph.center - r.orig
        let nb = dot(co,r.dir)
        let discr = mad(nb,nb, sph.radius2 - length_sq(co))
        if discr > 0.
            let discrSq = sqrt(discr)
            var t = nb - discrSq
            if t<=tMin
                t = nb + discrSq
            if t>tMin && t<hitT
                id = i
                hitT = t
        ++ i
    if id != -1
        let atPos = pointAt(r,hitT)
        outHit = [[Hit pos = atPos,
            normal = (atPos - s_Spheres[id].center) * s_Spheres[id].iRadius,
        t = hitT]]
    return id

def hitWorld(r:Ray const;tMin,tMax:float;var outHit:Hit;var outID:int&)
    outID = r|>hitSpheres(tMin,tMax,outHit)
    return outID != -1

def scatter(mat:Material const; r_in:Ray const; rec:Hit; var attenuation:float3&; var scattered:Ray; var outLightE:float3&; var inoutRayCount:int&)
    outLightE = float3(0)
    if mat.mtype==Type Lambert
        let target = rec.pos + rec.normal + random_unit_vector(RAND_SEED)
        scattered = [[Ray orig=rec.pos, dir=fast_normalize(target - rec.pos)]]
        attenuation = mat.albedo
        let sc = s_Spheres[LIGHT_SPHERE_ID].center
        let sw = fast_normalize(sc-rec.pos)
        let su = fast_normalize(cross(abs(sw.x)>0.01 ? float3(0,1,0) : float3(1,0,0),sw))
        let sv = cross(sw,su)
        let sr = s_Spheres[LIGHT_SPHERE_ID].radius
        let cosAMax = sqrt(saturate(1.-sr*sr*inv_distance_sq(rec.pos,sc)))
        let eps = random_float4(RAND_SEED)
        let cosA = mad(eps.x,cosAMax,1.) - eps.x
        let sinA = sqrt ( 1. - cosA*cosA )
        var sinPhi, cosPhi : float
        sincos(2. * PI * eps.y, sinPhi, cosPhi)
        let l = su*(cosPhi*sinA) + sv*(sinPhi*sinA) + sw*cosA
        var lightHit : Hit
        var hitID = 0
        ++inoutRayCount
        let ray = [[Ray orig=rec.pos, dir=l]]
        if ray|>hitWorld(kMinT,kMaxT,lightHit,hitID) && hitID==LIGHT_SPHERE_ID
            let nl = dot(rec.normal,r_in.dir) < 0. ? rec.normal : -rec.normal
            outLightE += (mat.albedo*s_SphereMats[LIGHT_SPHERE_ID].emissive) * ( saturate(dot(l,nl))*mad(-2.,cosAMax,2.) )
        return true
    elif mat.mtype == Type Metal
        let refl = reflect(r_in.dir,rec.normal)
        scattered = [[Ray orig=rec.pos, dir=fast_normalize(mad(random_in_unit_sphere(RAND_SEED),mat.roughness,refl))]]
        attenuation = mat.albedo
        return dot(scattered.dir,rec.normal) > 0.
    elif mat.mtype == Type Dielectric
        attenuation = float3(1)
        var refr : float3
        var reflProb : float
        if dot(r_in.dir,rec.normal)>0.
            refr = refract(r_in.dir,-rec.normal,mat.ri)
            if length_sq(refr) > 0.
                reflProb = schlick(mat.ri * dot(r_in.dir, rec.normal),mat.ri)
            else
                reflProb = 1.
        else
            refr = refract(r_in.dir,rec.normal,1.0/mat.ri)
            if length_sq(refr) > 0.
                reflProb = schlick(-dot(r_in.dir, rec.normal),mat.ri)
            else
                reflProb = 1.
        if random_float(RAND_SEED) < reflProb
            scattered = [[Ray orig=rec.pos, dir=fast_normalize(reflect(r_in.dir, rec.normal))]]
        else
            scattered = [[Ray orig=rec.pos, dir=fast_normalize(refr)]]
    else
        attenuation = float3(1,0,1)
        scattered = [[Ray orig=float3(0.), dir=float3(0.)]]
        return false
    return true

def trace(r:Ray;var inoutRayCount:int&)
    var ray = r
    var col = float3(0.)
    var curAtten = float3(1.)
    var doMaterialE = true
    for depth in range(kMaxDepth)
        var rec : Hit
        var id = 0
        ++inoutRayCount
        if ray|>hitWorld(kMinT,kMaxT,rec,id)
            var scattered : Ray
            var attenuation : float3
            var lightE : float3
            var matE = s_SphereMats[id].emissive
            if s_SphereMats[id] |>scatter(ray,rec,attenuation,scattered,lightE,inoutRayCount)
                if !doMaterialE
                    matE = float3(0)
                doMaterialE = s_SphereMats[id].mtype != Type Lambert
                col += curAtten * (matE + lightE)
                curAtten *= attenuation
                ray = scattered
            else
                col += curAtten * matE
                break
        else
            let t = mad(ray.dir,0.5,float3(1.))
            let skyCol = lerp(0.3*float3(1.), 0.3*float3(0.5, 0.7, 1.0), t)
            col += curAtten * skyCol
            break
    return col

def trace(screenWidth,screenHeight,frameCount,ymin,ymax:int;var backbuffer:array<float3>;cam:Camera)
    let invWH = float(1) / float2(screenWidth, screenHeight)
    let lerpFac = float3(float(frameCount) / float(frameCount + 1))
    var rayCount = 0
    var backbufferIdx = ymin * screenWidth
    for y in range(ymin,ymax)
        RAND_SEED = random_seed(y*117 + frameCount*13)
        for x in range(screenWidth)
            var col = float3(0)
            for s in range(DO_SAMPLES_PER_PIXEL)
                let uv = (float2(x,y) + random_float4(RAND_SEED).xy) * invWH
                col += cam |> getRay(uv.x,uv.y) |> trace(rayCount)
            col *= 1. / float(DO_SAMPLES_PER_PIXEL)
            backbuffer[backbufferIdx] = lerp(col, backbuffer[backbufferIdx], lerpFac)
            backbufferIdx ++
    return rayCount

def draw_ray(x,y,screenWidth,screenHeight:int)
    var rayCount = 0
    let lookfrom = float3(0,2,3)
    let lookat = float3(0)
    let distToFocus = 3.
    let aperture = 0.1
    let cam = Camera(lookfrom,lookat,float3(0,1,0),60.,float(screenWidth)/float(screenHeight),aperture,distToFocus)
    let invWH = float(1) / float2(screenWidth, screenHeight)
    var col = float3(0)
    for s in range(DO_SAMPLES_PER_PIXEL)
        let uv = (float2(x,y) + random_float4(RAND_SEED).xy) * invWH
        col += cam |> getRay(uv.x,uv.y) |> trace(rayCount)
    col *= 1. / float(DO_SAMPLES_PER_PIXEL)
    return col

def draw(frameCount,screenWidth,screenHeight,ymin,ymax:int;var backbuffer:array<float3>;var outRayCount:int&)
    var rayCount = 0
    let lookfrom = float3(0,2,3)
    let lookat = float3(0)
    let distToFocus = 3.
    let aperture = 0.1
    let cam = Camera(lookfrom,lookat,float3(0,1,0),60.,float(screenWidth)/float(screenHeight),aperture,distToFocus)
    rayCount += trace(screenWidth,screenHeight,frameCount,ymin,ymax,backbuffer,cam)
    outRayCount = rayCount


