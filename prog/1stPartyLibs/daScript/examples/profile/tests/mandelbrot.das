// options log=true, print_var_access=true, print_ref=true

require testProfile

require math

include ../config.das

def level ( c : float2 ) : int
    var l = 0
    var z = c
    while length(z)<2.0 && l<255
        z = float2(z.x*z.x-z.y*z.y,z.x*z.y+z.y*z.x)+c
        l ++
    return l-1

[sideeffects]
def test
    let xmin = -2.0
    let xmax = 2.0
    let ymin = -2.0
    let ymax = 2.0
    let N = 64
    let dx = (xmax-xmin)/float(N)
    let dy = (ymax-ymin)/float(N)
    var S = 0
    var xy = float2(xmin,ymin)
    for i in range(N)
        xy.y = ymin
        for j in range(N)
            S += level(xy)
            xy.y += dy
        xy.x += dx
    return S


[export,no_jit,no_aot]
def main
    profile(20,"mandelbrot") <|
        test()
    if is_cpp_time()
        cpp_label()
        profile(20,"mandelbrot") <|
            testMandelbrot()
        run_files([[TestFile
            luauFile = "mandelbrot.luau",
            luaFile = "mandelbrot.lua",
            luaJitFile = "mandelbrot.lua",
            monoFile = "mandelbrot.cs",
            dotNetFile = "mandelbrot.cs",
        ]])

