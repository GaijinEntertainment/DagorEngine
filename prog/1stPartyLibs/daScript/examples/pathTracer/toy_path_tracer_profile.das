options gen2
// options log
// options logCpp
// options log_nodes

options solid_context

require path_tracer

require strings
require stbimage
require daslib/jobque_boost
require daslib/random
require daslib/math_boost

[export]
def main {
    let width = 256
    let height = 256
    let kFrameCount = 1
    var backbuffer : array<float3>
    resize(backbuffer, width * height)
    print("\ntracing...\n")
    var i = 0

    init_path_tracer()
    // trace_ray(width, height, 132, 0)

    var t0 = ref_time_ticks()
    var totalRays = 0
    draw(i, width, height, 0, height, backbuffer, totalRays)
    var dt = get_time_usec(t0)
    var sec = double(dt) / 1000000.0lf
    print("AOT: {totalRays} in {sec} sec, {fmt(":.3f",double(totalRays)/double(dt))} mrays/sec\n")

    var pixels : array<uint>
    pixels |> reserve(width * height)
    for (y in range(height)) {
        for (x in range(width)) {
            let srgb = linear_to_SRGB(float4(backbuffer[x + (height - 1 - y) * width].xyz, 1.0f))
            pixels |> push <| RGBA_TO_UCOLOR(srgb)
        }
    }
    let path = "{get_das_root()}/examples/pathTracer/path_tracer.png"
    unsafe {
        stbi_write_png(path, width, height, 4, addr(pixels[0]), width * 4)
        print("image saved to {path}\n")
    }
}
