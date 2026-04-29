options gen2

module smooth shared public

require fastmath

// Smooth interpolation utilities built on top of fastmath C module

def smooth_step(edge0, edge1, x : float) : float {
    let t = fast_clamp_int(int((x - edge0) / (edge1 - edge0) * 1000.0), 0, 1000)
    let f = float(t) / 1000.0
    return f * f * (3.0 - 2.0 * f)
}

def smooth_lerp(a, b, t, smoothness : float) : float {
    let st = smooth_step(0.0, 1.0, t)
    return fast_lerp(a, b, fast_lerp(t, st, smoothness))
}
