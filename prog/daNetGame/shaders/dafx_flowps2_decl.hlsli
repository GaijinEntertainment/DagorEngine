#ifndef DAFX_FLOWPS2_HLSL
#define DAFX_FLOWPS2_HLSL

#ifdef __cplusplus
  #define SimData_ref SimData&
  #define SimData_cref const SimData&
  #define RenData_ref RenData&
  #define RenData_cref const RenData&
  #define ParentRenData_cref const ParentRenData&
  #define ParentSimData_cref const ParentSimData&
#else
  #define SimData_ref in out SimData
  #define SimData_cref SimData
  #define RenData_ref in out RenData
  #define RenData_cref RenData
  #define ParentRenData_cref ParentRenData
  #define ParentSimData_cref ParentSimData
#endif

//
// STD Cubic Curve
//
struct StdCubicCurve
{
  uint type;
  float4 coef;
  float4 spline_coef[5];
};
#ifdef __cplusplus
  #define StdCubicCurve_cref const StdCubicCurve&
#else
  #define StdCubicCurve_cref StdCubicCurve
#endif

#ifndef __cplusplus
  StdCubicCurve std_cubic_curve_load( BufferData_ref buf, in out uint ofs )
  {
    StdCubicCurve o;
    o.type = dafx_load_1ui( buf, ofs );
    o.coef = dafx_load_4f( buf, ofs );
    UNROLL
    for ( uint i = 0 ; i < 5 ; ++i )
      o.spline_coef[i] = dafx_load_4f( buf, ofs );
    return o;
  }
#endif

//
// STD Emitter
//
struct StdEmitter
{
  float4x4 worldTm;
  float4x4 normalTm;
  float3 emitterVel;
  float3 prevEmitterPos;

  float3 size;
  float radius;
  uint isVolumetric;
  uint isTrail;
  uint geometryType;
};
#ifdef __cplusplus
  #define StdEmitter_cref const StdEmitter&
#else
  #define StdEmitter_cref StdEmitter
#endif

#ifndef __cplusplus
  StdEmitter std_emitter_load( BufferData_ref buf, in out uint ofs )
  {
    StdEmitter o;
    o.worldTm = dafx_load_44mat( buf, ofs );
    o.normalTm = dafx_load_44mat( buf, ofs );
    o.emitterVel = dafx_load_3f( buf, ofs );
    o.prevEmitterPos = dafx_load_3f( buf, ofs );
    o.size = dafx_load_3f( buf, ofs );
    o.radius = dafx_load_1f( buf, ofs );
    o.isVolumetric = dafx_load_1ui( buf, ofs );
    o.isTrail = dafx_load_1ui( buf, ofs );
    o.geometryType = dafx_load_1ui( buf, ofs );
    return o;
  }
#endif

//
// Flowps2 Size
//
struct Flowps2Size
{
  float radius;
  float lengthDt;
  StdCubicCurve sizeFunc;
};
#ifdef __cplusplus
  #define Flowps2Size_cref const Flowps2Size&
#else
  #define Flowps2Size_cref Flowps2Size
#endif

#ifndef __cplusplus
  Flowps2Size flowps2_size_load( BufferData_ref buf, in out uint ofs )
  {
    Flowps2Size o;
    o.radius = dafx_load_1f( buf, ofs );
    o.lengthDt = dafx_load_1f( buf, ofs );
    o.sizeFunc = std_cubic_curve_load( buf, ofs );
    return o;
  }
#endif

//
// Flowps2 Color
//
struct Flowps2Color
{
  float4 color;
  float4 color2;
  float fakeHdrBrightness;
  StdCubicCurve rFunc;
  StdCubicCurve gFunc;
  StdCubicCurve bFunc;
  StdCubicCurve aFunc;
  StdCubicCurve specFunc;
};
#ifdef __cplusplus
  #define Flowps2Color_cref const Flowps2Color&
#else
  #define Flowps2Color_cref Flowps2Color
#endif

#ifndef __cplusplus
  Flowps2Color flowps2_color_load( BufferData_ref buf, in out uint ofs )
  {
    Flowps2Color o;
    o.color = dafx_load_4f( buf, ofs );
    o.color2 = dafx_load_4f( buf, ofs );
    o.fakeHdrBrightness = dafx_load_1f( buf, ofs );
    o.rFunc = std_cubic_curve_load( buf, ofs );
    o.gFunc = std_cubic_curve_load( buf, ofs );
    o.bFunc = std_cubic_curve_load( buf, ofs );
    o.aFunc = std_cubic_curve_load( buf, ofs );
    o.specFunc = std_cubic_curve_load( buf, ofs );
    return o;
  }
#endif

//
// Flowps2 Params (!!! KEEP ALL DYNAMIC DATA AT THE BEGINNING !!!)
//
struct Flowps2RenParams
{
  float4x4 fxTm;
  float4 lightPos;
  float4 lightParam;
  float colorScale;
  float fadeScale;
  float softnessDepthRcp;
  uint shader;

  uint framesX;
  uint framesY;
};
#define ParentRenData Flowps2RenParams

struct Flowps2SimParams
{
  StdEmitter emitter;
  uint seed;
  float fxScale;
  float4 colorMult;
  uint isUserScale;
  float2 alphaHeightRange;
  float3 localWindSpeed;
  uint onMoving;
  float life;

  Flowps2Size size;
  Flowps2Color color;
  float startPhase;
  float randomPhase;
  float randomLife;
  float randomVel;
  float normalVel;
  float3 startVel;
  float gravity;
  float viscosity;
  float randomRot;
  float rotSpeed;
  float rotViscosity;
  float animSpeed;
  float windScale;
  uint animatedFlipbook;
  float collisionBounceFactor;
  float collisionBounceDecay;
};
#define ParentSimData Flowps2SimParams

#ifndef __cplusplus
  #define L44M(a) o.a = dafx_load_44mat( buf, ofs )
  #define L1F(a) o.a = dafx_load_1f( buf, ofs )
  #define L2F(a) o.a = dafx_load_2f( buf, ofs )
  #define L3F(a) o.a = dafx_load_3f( buf, ofs )
  #define L4F(a) o.a = dafx_load_4f( buf, ofs )
  #define L1UI(a) o.a = dafx_load_1ui( buf, ofs )

  void flowps2_ren_params_load( in out Flowps2RenParams o, BufferData_ref buf, uint ofs )
  {
    L44M( fxTm );
    L4F( lightPos );
    L4F( lightParam );
    L1F( colorScale );
    L1F( fadeScale );
    L1F( softnessDepthRcp );
    L1UI( shader );
    L1UI( framesX );
    L1UI( framesY );
  }
  #define unpack_parent_ren_data flowps2_ren_params_load

  void flowps2_sim_params_load( in out Flowps2SimParams o, BufferData_ref buf, uint ofs )
  {
    o.emitter = std_emitter_load( buf, ofs );
    L1UI( seed );
    L1F( fxScale );
    L4F( colorMult );
    L1UI( isUserScale );
    L2F( alphaHeightRange );
    L3F( localWindSpeed );
    L1UI( onMoving );
    L1F( life );

    o.size = flowps2_size_load( buf, ofs );
    o.color = flowps2_color_load( buf, ofs );
    L1F( startPhase );
    L1F( randomPhase );
    L1F( randomLife );
    L1F( randomVel );
    L1F( normalVel );
    L3F( startVel );
    L1F( gravity );
    L1F( viscosity );
    L1F( randomRot );
    L1F( rotSpeed );
    L1F( rotViscosity );
    L1F( animSpeed );
    L1F( windScale );
    L1UI( animatedFlipbook );
    L1F( collisionBounceFactor );
    L1F( collisionBounceDecay );
  }
  #define unpack_parent_sim_data flowps2_sim_params_load

  #undef L44M
  #undef L1F
  #undef L2F
  #undef L3F
  #undef L4F
  #undef L1UI
#endif

#define DAFX_PARENT_REN_DATA_SIZE (30*4)
#define DAFX_PARENT_SIM_DATA_SIZE (238*4)

#ifdef __cplusplus
  G_STATIC_ASSERT( sizeof( Flowps2RenParams ) == DAFX_PARENT_REN_DATA_SIZE );
  G_STATIC_ASSERT( sizeof( Flowps2SimParams ) == DAFX_PARENT_SIM_DATA_SIZE );
#endif

// packed to 32 bytes
struct Flowps2RenPart
{
  float3 pos;          // 12b
  float4 color;        // 4b
  float radius;        // 4b
  float angle;         // 2b
  uint frameNo;        // 1b
  float frameDuration; // 1b
  float3 lengthening;  // 7b
  float rnd;           // 1b
};
#define RenData Flowps2RenPart

// packed to 32 bytes
struct Flowps2SimPart
{
  float3 vel;          // 12b
  float life;          // 4b
  float wvel;          // 4b
  float fxScale;       // 4b
  float4 startColor;   // 4b
  float angle;         // 4b
};
#define SimData Flowps2SimPart

#define DAFX_REN_DATA_SIZE 32
#define DAFX_SIM_DATA_SIZE 32

//
// packing/unpacking parts
//
DAFX_INLINE
void pack_ren_data( RenData_ref p, BufferData_ref buf, uint ofs )
{
  uint packedRnd = pack_n1f_to_byte( saturate( p.rnd ) );
  uint packedColor = pack_n4f_to_uint( saturate( p.color ) );
  uint angleShort = ((uint)(p.angle * (65536.f / dafx_2pi ))) % 65536;
  uint2 packedAngle = uint2( angleShort / 256, angleShort % 256 );
  uint packedMisc = pack_4b(
    p.frameNo % 256,
    pack_n1f_to_byte( p.frameDuration ),
    packedAngle.x,
    packedAngle.y );
  float lengtheningScale;
  uint packedLengtheningAndRnd = pack_s3f_1b_to_uint( p.lengthening, packedRnd, lengtheningScale );

  dafx_store_3f( p.pos, buf, ofs );
  dafx_store_1ui( packedColor, buf, ofs );

  dafx_store_1f( p.radius, buf, ofs );
  dafx_store_1ui( packedMisc, buf, ofs );
  dafx_store_1ui( packedLengtheningAndRnd, buf, ofs );
  dafx_store_1f( lengtheningScale, buf, ofs );
}

DAFX_INLINE
void unpack_ren_data( RenData_ref p, BufferData_ref buf, uint ofs )
{
  p.pos = dafx_load_3f( buf, ofs );
  uint packedColor = dafx_load_1ui( buf, ofs );

  p.radius = dafx_load_1f( buf, ofs );
  uint packedMisc = dafx_load_1ui( buf, ofs );
  uint packedLengtheningAndRnd = dafx_load_1ui( buf, ofs );
  float lengtheningScale = dafx_load_1f( buf, ofs );

  uint packedRnd;
  p.color = unpack_uint_to_n4f( packedColor );
  p.lengthening = unpack_uint_to_s3f_1b( packedLengtheningAndRnd, lengtheningScale, packedRnd );
  p.rnd = unpack_byte_to_n1f( packedRnd );

  uint4 misc4 = unpack_4b( packedMisc );
  p.frameNo = misc4.x;
  p.frameDuration = unpack_byte_to_n1f( misc4.y );
  p.angle = (misc4.w + misc4.z / 256.f) * dafx_2pi - dafx_pi;
}

DAFX_INLINE
void pack_sim_data( SimData_ref p, BufferData_ref buf, uint ofs )
{
  uint packedStartColor = pack_n4f_to_uint( p.startColor );

  dafx_store_3f( p.vel, buf, ofs );
  dafx_store_1f( p.life, buf, ofs );

  dafx_store_1f( p.angle, buf, ofs );
  dafx_store_1f( p.wvel, buf, ofs );
  dafx_store_1f( p.fxScale, buf, ofs );
  dafx_store_1ui( packedStartColor, buf, ofs );
}

DAFX_INLINE
void unpack_sim_data( SimData_ref p, BufferData_ref buf, uint ofs )
{
  p.vel = dafx_load_3f( buf, ofs );
  p.life = dafx_load_1f( buf, ofs );

  p.angle = dafx_load_1f( buf, ofs );
  p.wvel = dafx_load_1f( buf, ofs );
  p.fxScale = dafx_load_1f( buf, ofs );
  uint packedStartColor = dafx_load_1ui( buf, ofs );

  p.startColor = unpack_uint_to_n4f( packedStartColor );
}

#endif