returns vector representing reflection of vector v from normal n same as ::

    def reflect(v, n: float3)
        return v - 2. * dot(v, n) * n
