#ifndef DAFX_LOADERS_HLSL
#define DAFX_LOADERS_HLSL

#ifdef __cplusplus

// getters
DAFX_INLINE
uint dafx_get_1ui( BufferData_cref buf, uint offset )
{
  return buf[offset];
}

DAFX_INLINE
int dafx_get_1i( BufferData_cref buf, uint offset )
{
  return *(int*)( buf + offset );
}

DAFX_INLINE
uint2 dafx_get_2ui( BufferData_cref buf, uint offset )
{
  return *(uint2*)( buf + offset );
}

DAFX_INLINE
uint4 dafx_get_4ui( BufferData_cref buf, uint offset )
{
  return *(uint4*)( buf + offset );
}

DAFX_INLINE
float dafx_get_1f( BufferData_cref buf, uint offset )
{
  return *(float*)(buf + offset );
}

DAFX_INLINE
float2 dafx_get_2f( BufferData_cref buf, uint offset )
{
  return *(float2*)(buf + offset );
}

DAFX_INLINE
float3 dafx_get_3f( BufferData_cref buf, uint offset )
{
  return *(float3*)(buf + offset );
}

DAFX_INLINE
float4 dafx_get_4f( BufferData_cref buf, uint offset )
{
  return *(float4*)(buf + offset );
}

DAFX_INLINE
float3x3 dafx_get_33mat( BufferData_cref buf, uint offset )
{
  return *(Matrix3*)(buf + offset );
}

DAFX_INLINE
float4x4 dafx_get_44mat( BufferData_cref buf, uint offset )
{
  return *(TMatrix4*)(buf + offset );
}

// scale is stored in the last element
DAFX_INLINE
float4x4 dafx_get_44mat_scale( BufferData_cref buf, uint offset, float_ref scale )
{
  float4x4 mat = dafx_get_44mat( buf, offset );
  scale = mat[3][3];
  mat[3][3] = 1;
  return mat;
}

// setters
DAFX_INLINE
void dafx_set_1ui( uint v, BufferData_ref buf, uint offset )
{
  *( buf + offset ) = v;
}

DAFX_INLINE
void dafx_set_4ui( uint4_cref v, BufferData_ref buf, uint offset )
{
  *((uint4*)( buf + offset)) = v;
}

DAFX_INLINE
void dafx_set_1f( float v, BufferData_ref buf, uint offset )
{
  *((float*)( buf + offset)) = v;
}

DAFX_INLINE
void dafx_set_2f( float2_cref v, BufferData_ref buf, uint offset )
{
  *((float2*)( buf + offset)) = v;
}

DAFX_INLINE
void dafx_set_3f( float3_cref v, BufferData_ref buf, uint offset )
{
  *((float3*)( buf + offset)) = v;
}

DAFX_INLINE
void dafx_set_4f( float4_cref v, BufferData_ref buf, uint offset )
{
  *((float4*)( buf + offset)) = v;
}

// loaders
DAFX_INLINE
uint dafx_load_1ui( BufferData_cref buf, uint1_ref offset )
{
  uint v = dafx_get_1ui( buf, offset );
  offset += 1u;
  return v;
}

DAFX_INLINE
int dafx_load_1i( BufferData_cref buf, uint1_ref offset )
{
  int v = dafx_get_1i( buf, offset );
  offset += 1u;
  return v;
}

DAFX_INLINE
uint2 dafx_load_2ui( BufferData_cref buf, uint1_ref offset )
{
  uint2 v = dafx_get_2ui( buf, offset );
  offset += 2u;
  return v;
}

DAFX_INLINE
uint4 dafx_load_4ui( BufferData_cref buf, uint1_ref offset )
{
  uint4 v = dafx_get_4ui( buf, offset );
  offset += 4u;
  return v;
}

DAFX_INLINE
float dafx_load_1f( BufferData_cref buf, uint1_ref offset )
{
  float v = dafx_get_1f( buf, offset );
  offset += 1u;
  return v;
}

DAFX_INLINE
float2 dafx_load_2f( BufferData_cref buf, uint1_ref offset )
{
  float2 v = dafx_get_2f( buf, offset );
  offset += 2u;
  return v;
}

DAFX_INLINE
float3 dafx_load_3f( BufferData_cref buf, uint_ref offset )
{
  float3 v = dafx_get_3f( buf, offset );
  offset += 3u;
  return v;
}

DAFX_INLINE
float4 dafx_load_4f( BufferData_cref buf, uint1_ref offset )
{
  float4 v = dafx_get_4f( buf, offset );
  offset += 4u;
  return v;
}

DAFX_INLINE
float4x4 dafx_load_44mat( BufferData_cref buf, uint1_ref offset )
{
  float4x4 v = dafx_get_44mat( buf, offset );
  offset += 16u;
  return v;
}

DAFX_INLINE
float3x3 dafx_load_33mat( BufferData_cref buf, uint1_ref offset )
{
  float3x3 v = dafx_get_33mat( buf, offset );
  offset += 9u;
  return v;
}

// savers
DAFX_INLINE
void dafx_store_1ui( uint v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_1ui( v, buf, offset );
  offset += 1u;
}

DAFX_INLINE
void dafx_store_4ui( uint4_cref v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_4ui( v, buf, offset );
  offset += 4u;
}

DAFX_INLINE
void dafx_store_1f( float v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_1f( v, buf, offset );
  offset += 1u;
}

DAFX_INLINE
void dafx_store_2f( float2_cref v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_2f( v, buf, offset );
  offset += 2u;
}

DAFX_INLINE
void dafx_store_3f( float3_cref v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_3f( v, buf, offset );
  offset += 3u;
}

DAFX_INLINE
void dafx_store_4f( float4_cref v, BufferData_ref buf, uint1_ref offset )
{
  dafx_set_4f( v, buf, offset);
  offset += 4u;
}

#else // hlsl

// getters
#define dafx_get_1ui(_buf, _offset) _dafx_get_1ui(_offset, _FILE_, __LINE__)
uint _dafx_get_1ui(uint offset, int file, int ln)
{
  return loadBufferBase(dafx_system_data, offset, file, ln, -1);
}

#define dafx_get_1i(_buf, _offset) _dafx_get_1i(_offset, _FILE_, __LINE__)
int _dafx_get_1i(uint offset, int file, int ln)
{
  return asint( loadBufferBase(dafx_system_data, offset, file, ln, -1) );
}

#define dafx_get_2ui(_buf, _offset) _dafx_get_2ui(_offset, _FILE_, __LINE__)
uint2 _dafx_get_2ui(uint offset, int file, int ln)
{
  return uint2(
    loadBufferBase(dafx_system_data, offset + 0, file, ln, -1),
    loadBufferBase(dafx_system_data, offset + 1, file, ln, -1) );
}

#define dafx_get_4ui(_buf, _offset) _dafx_get_4ui(_offset, _FILE_, __LINE__)
uint4 _dafx_get_4ui(uint offset, int file, int ln)
{
  return uint4(
    loadBufferBase(dafx_system_data, offset + 0, file, ln, -1),
    loadBufferBase(dafx_system_data, offset + 1, file, ln, -1),
    loadBufferBase(dafx_system_data, offset + 2, file, ln, -1),
    loadBufferBase(dafx_system_data, offset + 3, file, ln, -1) );
}

#define dafx_get_1f(_buf, _offset) _dafx_get_1f(_offset, _FILE_, __LINE__)
float _dafx_get_1f(uint offset, int file, int ln)
{
  return asfloat( loadBufferBase(dafx_system_data, offset, file, ln, -1) );
}

#define dafx_get_2f(_buf, _offset) _dafx_get_2f(_offset, _FILE_, __LINE__)
float2 _dafx_get_2f(uint offset, int file, int ln )
{
  return float2(
    asfloat( loadBufferBase(dafx_system_data, offset + 0, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 1, file, ln, -1) ) );
}

#define dafx_get_3f(_buf, _offset) _dafx_get_3f(_offset, _FILE_, __LINE__)
float3 _dafx_get_3f(uint offset, int file, int ln )
{
  return float3(
    asfloat( loadBufferBase(dafx_system_data, offset + 0, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 1, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 2, file, ln, -1) ) );
}

#define dafx_get_4f(_buf, _offset) _dafx_get_4f(_offset, _FILE_, __LINE__)
float4 _dafx_get_4f(uint offset, int file, int ln )
{
  return float4(
    asfloat( loadBufferBase(dafx_system_data, offset + 0, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 1, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 2, file, ln, -1) ),
    asfloat( loadBufferBase(dafx_system_data, offset + 3, file, ln, -1) ) );
}

#define dafx_get_33mat(_buf, _offset) _dafx_get_33mat(_offset, _FILE_, __LINE__)
float3x3 _dafx_get_33mat(uint offset, int file, int ln )
{
  float3x3 r;
  r[0] = _dafx_get_3f( offset + 0, file, ln );
  r[1] = _dafx_get_3f( offset + 3, file, ln );
  r[2] = _dafx_get_3f( offset + 6, file, ln );
  return r;
}

#define dafx_get_44mat(_buf, _offset) _dafx_get_44mat(_offset, _FILE_, __LINE__)
float4x4 _dafx_get_44mat(uint offset, int file, int ln )
{
  float4x4 r;
  r[0] = _dafx_get_4f( offset + 0, file, ln );
  r[1] = _dafx_get_4f( offset + 4, file, ln );
  r[2] = _dafx_get_4f( offset + 8, file, ln );
  r[3] = _dafx_get_4f( offset + 12, file, ln );
  return r;
}

// scale is stored in the last element
// TODO: return a float3x4 instead
#define dafx_get_44mat_scale(_buf, _offset, _scale) _dafx_get_44mat_scale(_offset, _scale, _FILE_, __LINE__)
float4x4 _dafx_get_44mat_scale(uint offset, out float scale, int file, int ln )
{
  float4x4 r = _dafx_get_44mat( offset, file, ln );
  scale = r[3][3];
  r[3][3] = 1;
  return r;
}

#define dafx_get_44mat_scale_noret(_buf, _offset, _wtm, _scale) _dafx_get_44mat_scale_noret(_offset, _wtm, _scale, _FILE_, __LINE__)
void _dafx_get_44mat_scale_noret(uint offset, out float4x4 wtm, out float scale, int file, int ln )
{
  wtm[0] = _dafx_get_4f( offset + 0, file, ln );
  wtm[1] = _dafx_get_4f( offset + 4, file, ln );
  wtm[2] = _dafx_get_4f( offset + 8, file, ln );
  wtm[3] = _dafx_get_4f( offset + 12, file, ln );
  scale = wtm[3][3];
  wtm[3][3] = 1;
}

// setters
#define dafx_set_1ui(_v, _buf, _offset) _dafx_set_1ui(_v, _offset, _FILE_, __LINE__)
void _dafx_set_1ui( uint v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset, dim, file, ln, -1);
    #endif
    dafx_system_data[offset] = v;
  #endif
}

#define dafx_set_4ui(_v, _buf, _offset) _dafx_set_4ui(_v, _offset, _FILE_, __LINE__)
void _dafx_set_4ui( uint4_cref v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset + 3, dim, file, ln, -1);
    #endif
    dafx_system_data[offset + 0] = v.x;
    dafx_system_data[offset + 1] = v.y;
    dafx_system_data[offset + 2] = v.z;
    dafx_system_data[offset + 3] = v.w;
  #endif
}

#define dafx_set_1f(_v, _buf, _offset) _dafx_set_1f(_v, _offset, _FILE_, __LINE__)
void _dafx_set_1f( float v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset, dim, file, ln, -1);
    #endif
    dafx_system_data[offset] = asuint( v );
  #endif
}

#define dafx_set_2f(_v, _buf, _offset) _dafx_set_2f(_v, _offset, _FILE_, __LINE__)
void _dafx_set_2f( float2_cref v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset + 1, dim, file, ln, -1);
    #endif
    dafx_system_data[offset + 0] = asuint( v.x );
    dafx_system_data[offset + 1] = asuint( v.y );
  #endif
}

#define dafx_set_3f(_v, _buf, _offset) _dafx_set_3f(_v, _offset, _FILE_, __LINE__)
void _dafx_set_3f( float3_cref v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset + 2, dim, file, ln, -1);
    #endif
    dafx_system_data[offset + 0] = asuint( v.x );
    dafx_system_data[offset + 1] = asuint( v.y );
    dafx_system_data[offset + 2] = asuint( v.z );
  #endif
}

#define dafx_set_4f(_v, _buf, _offset) _dafx_set_4f(_v, _offset, _FILE_, __LINE__)
void _dafx_set_4f( float4_cref v, uint offset, int file, int ln )
{
  #ifdef DAFX_RW_ENABLED
    #if DEBUG_ENABLE_BOUNDS_CHECKS && !_HARDWARE_METAL
      uint dim, stride;
      dafx_system_data.GetDimensions(dim, stride);
      checkBufferBounds(offset + 3, dim, file, ln, -1);
    #endif
    dafx_system_data[offset] = asuint( v.x );
    dafx_system_data[offset + 1] = asuint( v.y );
    dafx_system_data[offset + 2] = asuint( v.z );
    dafx_system_data[offset + 3] = asuint( v.w );
  #endif
}

// loaders
#define dafx_load_1ui(_buf, _offset) _dafx_load_1ui(_offset, _FILE_, __LINE__)
uint _dafx_load_1ui( uint1_ref offset, int file, int ln )
{
  uint v = _dafx_get_1ui( offset, file, ln );
  offset += 1u;
  return v;
}

#define dafx_load_1i(_buf, _offset) _dafx_load_1i(_offset, _FILE_, __LINE__)
int _dafx_load_1i( uint1_ref offset, int file, int ln )
{
  int v = _dafx_get_1i( offset, file, ln );
  offset += 1u;
  return v;
}

#define dafx_load_2ui(_buf, _offset) _dafx_load_2ui(_offset, _FILE_, __LINE__)
uint2 _dafx_load_2ui( uint1_ref offset, int file, int ln )
{
  uint2 v = _dafx_get_2ui( offset, file, ln );
  offset += 2u;
  return v;
}

#define dafx_load_4ui(_buf, _offset) _dafx_load_4ui(_offset, _FILE_, __LINE__)
uint4 _dafx_load_4ui( uint1_ref offset, int file, int ln )
{
  uint4 v = _dafx_get_4ui( offset, file, ln );
  offset += 4u;
  return v;
}

#define dafx_load_1f(_buf, _offset) _dafx_load_1f(_offset, _FILE_, __LINE__)
float _dafx_load_1f( uint1_ref offset, int file, int ln )
{
  float v = _dafx_get_1f( offset, file, ln );
  offset += 1u;
  return v;
}

#define dafx_load_2f(_buf, _offset) _dafx_load_2f(_offset, _FILE_, __LINE__)
float2 _dafx_load_2f( uint1_ref offset, int file, int ln )
{
  float2 v = _dafx_get_2f( offset, file, ln );
  offset += 2u;
  return v;
}

#define dafx_load_3f(_buf, _offset) _dafx_load_3f(_offset, _FILE_, __LINE__)
float3 _dafx_load_3f( uint_ref offset, int file, int ln )
{
  float3 v = _dafx_get_3f( offset, file, ln );
  offset += 3u;
  return v;
}

#define dafx_load_4f(_buf, _offset) _dafx_load_4f(_offset, _FILE_, __LINE__)
float4 _dafx_load_4f( uint1_ref offset, int file, int ln )
{
  float4 v = _dafx_get_4f( offset, file, ln );
  offset += 4u;
  return v;
}

#define dafx_load_44mat(_buf, _offset) _dafx_load_44mat(_offset, _FILE_, __LINE__)
float4x4 _dafx_load_44mat( uint1_ref offset, int file, int ln )
{
  float4x4 v = _dafx_get_44mat( offset, file, ln );
  offset += 16u;
  return v;
}

#define dafx_load_33mat(_buf, _offset) _dafx_load_33mat(_offset, _FILE_, __LINE__)
float3x3 _dafx_load_33mat( uint1_ref offset, int file, int ln )
{
  float3x3 v = _dafx_get_33mat( offset, file, ln );
  offset += 9u;
  return v;
}

// savers
#define dafx_store_1ui(_v, _buf, _offset) _dafx_store_1ui(_v, _offset, _FILE_, __LINE__)
void _dafx_store_1ui( uint v, uint1_ref offset, int file, int ln )
{
  _dafx_set_1ui( v, offset, file, ln );
  offset += 1u;
}

#define dafx_store_4ui(_v, _buf, _offset) _dafx_store_4ui(_v, _offset, _FILE_, __LINE__)
void _dafx_store_4ui( uint4_cref v, uint1_ref offset, int file, int ln )
{
  _dafx_set_4ui( v, offset, file, ln );
  offset += 4u;
}

#define dafx_store_1f(_v, _buf, _offset) _dafx_store_1f(_v, _offset, _FILE_, __LINE__)
void _dafx_store_1f( float v, uint1_ref offset, int file, int ln )
{
  _dafx_set_1f( v, offset, file, ln );
  offset += 1u;
}

#define dafx_store_2f(_v, _buf, _offset) _dafx_store_2f(_v, _offset, _FILE_, __LINE__)
void _dafx_store_2f( float2_cref v, uint1_ref offset, int file, int ln )
{
  _dafx_set_2f( v, offset, file, ln );
  offset += 2u;
}

#define dafx_store_3f(_v, _buf, _offset) _dafx_store_3f(_v, _offset, _FILE_, __LINE__)
void _dafx_store_3f( float3_cref v, uint1_ref offset, int file, int ln )
{
  _dafx_set_3f( v, offset, file, ln );
  offset += 3u;
}

#define dafx_store_4f(_v, _buf, _offset) _dafx_store_4f(_v, _offset, _FILE_, __LINE__)
void _dafx_store_4f( float4_cref v, uint1_ref offset, int file, int ln )
{
  _dafx_set_4f( v, offset, file, ln );
  offset += 4u;
}

#endif
#endif
