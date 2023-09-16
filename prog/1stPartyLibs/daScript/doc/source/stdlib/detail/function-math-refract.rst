returns vector representing refractoin of vector v from normal n same as ::

    def refract(v, n: float3; nint: float; outRefracted: float3&)
        let dt = dot(v, n)
        let discr = 1. - nint * nint * (1. - dt * dt)
        if discr > 0.
            outRefracted = nint * (v - n * dt) - n * sqrt(discr)
            return true
        return false
