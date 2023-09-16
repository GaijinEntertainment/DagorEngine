// options log
// options logCpp
// options log_nodes

require path_tracer

require strings
require stbimage
require daslib/jobque_boost
require daslib/random
require daslib/math_boost

enum Mode
    SingleThread
    JobQue
    Threads
    SimulateGpu

let USE_MODE = Mode JobQue

[export]
def main
    let width = 1024
    let height = 1024
    let kFrameCount = 1
    var backbuffer : array<float3>
    resize(backbuffer,width*height)
    print("\ntracing...\n")
    var i = 0
    if USE_MODE == Mode JobQue
        with_job_que <|
            var t0 = ref_time_ticks()
            var totalRays = 0
            profile(kFrameCount,"ray-trace frame (with JOB-QUE)") <|
                var pbb : array<float3>?
                var ptr : int?
                unsafe
                    pbb = addr(backbuffer)                  // NOTE: we actually share data here
                    ptr = addr(totalRays)
                with_job_status(height) <| $ ( status )
                    for y in range(height)
                        new_job <| @
                            init_path_tracer()
                            var rayCount = 0
                            draw(i, width, height, y, y+1, *pbb, rayCount)
                            *ptr += rayCount
                            status |> notify_and_release
                    status |> join
                    i ++
            var dt = get_time_usec(t0)
            var sec = double(dt)/1000000.0lf
            print("{totalRays} in {sec} sec, {format("%.3f",double(totalRays)/double(dt))} mrays/sec\n")
    elif USE_MODE == Mode Threads
        var t0 = ref_time_ticks()
        var totalRays = 0
        let hwThreads = get_total_hw_threads()
        profile(kFrameCount,"ray-trace frame (with {hwThreads} THREADS)") <|
            var pbb : array<float3>?
            var ptr : int?
            unsafe
                pbb = addr(backbuffer)                  // NOTE: we actually share data here
                ptr = addr(totalRays)
            with_job_status(hwThreads) <| $ ( status )
                let chunk = (height+hwThreads-1) / hwThreads
                for c in range(hwThreads)
                    let yMin = c * chunk
                    let yMax = min((c + 1)*chunk, height)
                    new_thread <| @
                        init_path_tracer()
                        var rayCount = 0
                        draw(i, width, height, yMin, yMax, *pbb, rayCount)
                        *ptr += rayCount
                        status |> notify_and_release
                status |> join
                i ++
        var dt = get_time_usec(t0)
        var sec = double(dt)/1000000.0lf
        print("{totalRays} in {sec} sec, {format("%.3f",double(totalRays)/double(dt))} mrays/sec\n")
    elif USE_MODE == Mode SimulateGpu
        profile(kFrameCount,"ray-trace frame via per ray prep") <|
            init_path_tracer()
            for y in range(height)
                for x in range(width)
                    random_seed2D(RAND_SEED,int2(x,y))
                    backbuffer[y*width+x] = draw_ray(x,y,width,height)
    else
        profile(kFrameCount,"ray-trace frame") <|
            init_path_tracer()
            var rayCount = 0
            draw(i, width, height, 0, height, backbuffer, rayCount)
            i ++

    var pixels : array<uint>
    pixels |> reserve(width*height)
    for y in range(height)
        for x in range(width)
            let srgb = linear_to_SRGB(float4(backbuffer[x + (height-1-y)*width].xyz,1.0f))
            pixels |> push <| RGBA_TO_UCOLOR(srgb)
    unsafe
        stbi_write_png( "{get_das_root()}/examples/pathTracer/path_tracer.png", width, height, 4, addr(pixels[0]), width*4  )
