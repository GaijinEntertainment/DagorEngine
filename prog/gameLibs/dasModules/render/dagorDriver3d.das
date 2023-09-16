options indenting = 2
require DagorDriver3D

enum ZCompareFunc : uint8
  NEVER               = CMPF_NEVER
  LESS                = CMPF_LESS
  EQUAL               = CMPF_EQUAL
  LESSEQUAL           = CMPF_LESSEQUAL
  GREATER             = CMPF_GREATER
  NOTEQUAL            = CMPF_NOTEQUAL
  GREATEREQUAL        = CMPF_GREATEREQUAL
  ALWAYS              = CMPF_ALWAYS
  INVALID             = 0

enum BlendOperation : uint8
  ADD              = BLENDOP_ADD
  SUBTRACT         = BLENDOP_SUBTRACT
  REVSUBTRACT      = BLENDOP_REVSUBTRACT
  MIN              = BLENDOP_MIN
  MAX              = BLENDOP_MAX
  INVALID          = 0

enum BlendType : uint
  ZERO              = BLEND_ZERO
  ONE               = BLEND_ONE
  SRCCOLOR          = BLEND_SRCCOLOR
  INVSRCCOLOR       = BLEND_INVSRCCOLOR
  SRCALPHA          = BLEND_SRCALPHA
  INVSRCALPHA       = BLEND_INVSRCALPHA
  DESTALPHA         = BLEND_DESTALPHA
  INVDESTALPHA      = BLEND_INVDESTALPHA
  DESTCOLOR         = BLEND_DESTCOLOR
  INVDESTCOLOR      = BLEND_INVDESTCOLOR
  SRCALPHASAT       = BLEND_SRCALPHASAT
  BOTHINVSRCALPHA   = BLEND_BOTHINVSRCALPHA
  BLENDFACTOR       = BLEND_BLENDFACTOR
  INVBLENDFACTOR    = BLEND_INVBLENDFACTOR
  INVALID           = 0


enum WriteMask : uint
  RED     = WRITEMASK_RED
  GREEN   = WRITEMASK_GREEN
  BLUE    = WRITEMASK_BLUE
  ALPHA   = WRITEMASK_ALPHA
  RGB     = WRITEMASK_RGB
  ALL     = WRITEMASK_ALL

bitfield RenderState
  Z_TEST_DISABLE
  Z_WRITE_DISABLE
  Z_BOUNDS_ENABLED
  Z_CLAMP_ENABLED
  Z_FUNC
  Z_BIAS             //both biases
  STENCIL            //all stencil
  OP
  OP_A
  SRC_DEST
  SRC_DEST_A
  CULL_NONE          //superceeds flipcull
  FLIP_CULL
  FORCED_SAMPLE_COUNT
  CONSERVATIVE
  SCISSOR_ENABLED
  ALPHA_TO_COVERAGE

struct OverrideRenderState
  bits : RenderState = bitfield(0)
  zFunc : ZCompareFunc = ZCompareFunc INVALID
  forcedSampleCount : uint8 = uint8(0)
  blendOp : BlendOperation = BlendOperation INVALID
  blendOpA : BlendOperation = BlendOperation INVALID
  sblend : BlendType = BlendType INVALID
  dblend : BlendType = BlendType INVALID
  sblenda : BlendType = BlendType INVALID
  dblenda : BlendType = BlendType INVALID
  colorWr : WriteMask = WriteMask ALL
  zBias : float = 0f
  slopeZBias : float = 0f