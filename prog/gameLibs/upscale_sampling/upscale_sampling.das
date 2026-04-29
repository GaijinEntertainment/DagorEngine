module upscale_sampling

require DagorDriver3D
require DagorResPtr

//same as in c++ code
let UPSCALE_WEIGHTS_COUNT = 16
let UPSCALE_DATA_SIZE = typeinfo sizeof(type<float4>)

[generic]
def upload_weights(upscale_weights_buffer : UniqueBufHolder)
  let UPSCALE_WEIGHTS_DATA <-  array<float>(
    1f, 1f, 1f, 1f,
    0f, 0f, 0f, 1f,
    0f, 0f, 1f, 0f,
    0f, 0f, 1f, 1f,
    0f, 1f, 0f, 0f,
    0f, 1f, 0f, 1f,
    0f, 1f, 1f, 0f,
    0f, 1f, 1f, 1f,

    1f, 0f, 0f, 0f,
    1f, 0f, 0f, 1f,
    1f, 0f, 1f, 0f,
    1f, 0f, 1f, 1f,
    1f, 1f, 0f, 0f,
    1f, 1f, 0f, 1f,
    1f, 1f, 1f, 0f,
    1f, 1f, 1f, 1f
  )
  *getBuf(upscale_weights_buffer) |> updateData(UPSCALE_WEIGHTS_DATA, VBLOCK_WRITEONLY | VBLOCK_DISCARD)

[generic]
def init_upscale(var upscale_weights_buffer : UniqueBufHolder)
  create_sbuffer(upscale_weights_buffer, UPSCALE_DATA_SIZE, UPSCALE_WEIGHTS_COUNT, SBCF_CB_PERSISTENT, uint(0), "upscale_sampling_weights")
