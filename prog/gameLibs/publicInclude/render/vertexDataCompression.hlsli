#ifndef VERTEX_DATA_COMPRESSION_HLSL_INCLUDED
#define VERTEX_DATA_COMPRESSION_HLSL_INCLUDED

// useful for packing normals, tangents or even vertex position in single uint
// _valmax - maximum length of vector
// 11, 10, 11 bits in components

#define PACK_VECTOR_TO_UINT(_invec, _valmax, _outuint)             \
  {float3 _pvec = 0.5f*_invec / _valmax + float3(0.5f, 0.5f, 0.5f); \
 _outuint = (uint)(_pvec.x * 2047.0f) |                            \
            ((uint)(_pvec.y * 1023.0f) << 11) |                    \
            ((uint)(_pvec.z * 2047.0f) << 21);}

#define UNPACKPACK_VECTOR_FROM_UINT(_inuint, _valmax, _outvec)                \
 _outvec = _valmax*float3((_inuint&2047) * (2.0f/2047.0f) - 1.0f,             \
                         ((_inuint >> 11) & 1023) * (2.0f / 1023.0f) - 1.0f,  \
                         ((_inuint >> 21) & 2047) * (2.0f / 2047.0f) - 1.0f);

// useful for packing texcoords and small unsigned integer (usually id) value in single uint
// _bits0, _bits1 - bit size for tc channels
// _inid packed into remaining bits
// _intc must have range 0..1

#define PACK_TC2_ID_TO_UINT(_intc, _inid, _bits0, _bits1, _outuint)          \
 _outuint = (uint)(_intc.x * (float)((2<<(_bits0 - 1)) - 1)) |               \
            ((uint)(_intc.y * (float)((2<<(_bits1 - 1)) - 1)) << (_bits0)) | \
            ((uint)(_inid << (_bits0 + _bits1)));

#define UNPACKPACK_TC2_ID_FROM_UINT(_inuint, _bits0, _bits1, _outtc, _outid)                    \
 _outid = _inuint >> (_bits0 + _bits1);                                                         \
 _outtc = float2((_inuint & ((2<<(_bits0-1)) - 1)) * (1.0f / ((float)((2<<(_bits0-1)) - 1))),   \
                ((_inuint >> _bits0) & ((2<<(_bits1-1)) - 1)) * (1.0f / ((float)((2<<(_bits1-1)) - 1))));

#endif