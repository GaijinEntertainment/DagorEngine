// © 2021 NVIDIA Corporation

#pragma once

//======================================================================================================================
// Other
//======================================================================================================================

ML_INLINE float SplitZ_Logarithmic(uint32_t i, uint32_t splits, float fZnear, float fZfar) {
    float ratio = fZfar / fZnear;
    float k = float(i) / float(splits);
    float z = fZnear * pow(ratio, k);

    return z;
}

ML_INLINE float SplitZ_Uniform(uint32_t i, uint32_t splits, float fZnear, float fZfar) {
    float delta = fZfar - fZnear;
    float k = float(i) / float(splits);
    float z = fZnear + delta * k;

    return z;
}

ML_INLINE float SplitZ_Mixed(uint32_t i, uint32_t splits, float fZnear, float fZfar, float lambda) {
    float z_log = SplitZ_Logarithmic(i, splits, fZnear, fZfar);
    float z_uni = SplitZ_Uniform(i, splits, fZnear, fZfar);
    float z = lerp(z_log, z_uni, lambda);

    return z;
}

ML_INLINE uint32_t GreatestCommonDivisor(uint32_t a, uint32_t b) {
    while (a && b) {
        if (a >= b)
            a = a % b;
        else
            b = b % a;
    }

    return a + b;
}

ML_INLINE uint32_t LeastCommonMultiple(uint32_t a, uint32_t b) {
    return (a * b) / GreatestCommonDivisor(a, b);
}

//======================================================================================================================
// Ray-Triangle/AABB
//======================================================================================================================

// NOTE: overlapping axis-aligned boundary box and triangle (center - aabb center, extents - half size)
// NOTE: AABB-triangle overlap test code by Tomas Akenine-Moller
//       http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/
//       SSE code from http://www.codercorner.com/blog/?p=1118

ML_INLINE uint32_t TestClassIII(const v4f& e0V, const v4f& v0V, const v4f& v1V, const v4f& v2V, const v4f& extents) {
    v4f fe0ZYX_V = v4f_abs(e0V);

    v4f e0XZY_V = v4f_swizzle(e0V, 1, 2, 0, 3);
    v4f v0XZY_V = v4f_swizzle(v0V, 1, 2, 0, 3);
    v4f v1XZY_V = v4f_swizzle(v1V, 1, 2, 0, 3);
    v4f v2XZY_V = v4f_swizzle(v2V, 1, 2, 0, 3);
    v4f fe0XZY_V = v4f_swizzle(fe0ZYX_V, 1, 2, 0, 3);
    v4f extentsXZY_V = v4f_swizzle(extents, 1, 2, 0, 3);

    v4f radV = _mm_add_ps(_mm_mul_ps(extents, fe0XZY_V), _mm_mul_ps(extentsXZY_V, fe0ZYX_V));
    v4f p0V = _mm_sub_ps(_mm_mul_ps(v0V, e0XZY_V), _mm_mul_ps(v0XZY_V, e0V));
    v4f p1V = _mm_sub_ps(_mm_mul_ps(v1V, e0XZY_V), _mm_mul_ps(v1XZY_V, e0V));
    v4f p2V = _mm_sub_ps(_mm_mul_ps(v2V, e0XZY_V), _mm_mul_ps(v2XZY_V, e0V));

    v4f minV = _mm_min_ps(_mm_min_ps(p0V, p1V), p2V);
    v4f maxV = _mm_max_ps(_mm_max_ps(p0V, p1V), p2V);

    uint32_t test = _mm_movemask_ps(_mm_cmpgt_ps(minV, radV));
    radV = _mm_sub_ps(_mm_setzero_ps(), radV);
    test |= _mm_movemask_ps(_mm_cmpgt_ps(radV, maxV));

    return test & 7;
}

ML_INLINE bool IsOverlapBoxTriangle(const float3& boxcenter, const float3& extents, const float3& p0, const float3& p1, const float3& p2) {
    v4f v0V = _mm_sub_ps(p0.xmm, boxcenter.xmm);
    v4f cV = v4f_abs(v0V);
    uint32_t test = _mm_movemask_ps(_mm_sub_ps(cV, extents.xmm));

    if ((test & 7) == 7)
        return true;

    v4f v1V = _mm_sub_ps(p1.xmm, boxcenter.xmm);
    v4f v2V = _mm_sub_ps(p2.xmm, boxcenter.xmm);
    v4f minV = _mm_min_ps(v0V, v1V);
    minV = _mm_min_ps(minV, v2V);
    test = _mm_movemask_ps(_mm_cmpgt_ps(minV, extents.xmm));

    if (test & 7)
        return false;

    v4f maxV = _mm_max_ps(v0V, v1V);
    maxV = _mm_max_ps(maxV, v2V);
    cV = _mm_sub_ps(_mm_setzero_ps(), extents.xmm);
    test = _mm_movemask_ps(_mm_cmpgt_ps(cV, maxV));

    if (test & 7)
        return false;

    v4f e0V = _mm_sub_ps(v1V, v0V);
    v4f e1V = _mm_sub_ps(v2V, v1V);
    v4f normalV = v4f_cross(e0V, e1V);
    v4f dV = v4f_dot33(normalV, v0V);

    v4f normalSignsV = _mm_and_ps(normalV, c_v4f_Sign);
    maxV = _mm_or_ps(extents.xmm, normalSignsV);

    v4f tmpV = v4f_dot33(normalV, maxV);
    test = _mm_movemask_ps(_mm_cmpgt_ps(dV, tmpV));

    if (test & 7)
        return false;

    normalSignsV = _mm_xor_ps(normalSignsV, c_v4f_Sign);
    minV = _mm_or_ps(extents.xmm, normalSignsV);

    tmpV = v4f_dot33(normalV, minV);
    test = _mm_movemask_ps(_mm_cmpgt_ps(tmpV, dV));

    if (test & 7)
        return false;

    if (TestClassIII(e0V, v0V, v1V, v2V, extents.xmm))
        return false;

    if (TestClassIII(e1V, v0V, v1V, v2V, extents.xmm))
        return false;

    v4f e2V = _mm_sub_ps(v0V, v2V);

    if (TestClassIII(e2V, v0V, v1V, v2V, extents.xmm))
        return false;

    return true;
}

// NOTE: barycentric ray-triangle test by Tomas Akenine-Moller
ML_INLINE bool IsIntersectRayTriangle(const float3& origin, const float3& dir, const float3& v1, const float3& v2, const float3& v3, float3& out_tuv) {
    // find vectors for two edges sharing vert0
    float3 e1 = v2 - v1;
    float3 e2 = v3 - v1;

    // begin calculating determinant - also used to calculate U parameter
    float3 pvec = cross(dir, e2);

    // if determinant is near zero, ray lies in plane of triangle
    float det = dot(e1, pvec);

    if (det < -1e-6f)
        return false;

    // calculate distance from vert0 to ray origin
    float3 tvec = origin - v1;

    // calculate U parameter and test bounds
    float u = dot(tvec, pvec);

    if (u < 0.0f || u > det)
        return false;

    // prepare to test V parameter
    float3 qvec = cross(tvec, e1);

    // calculate V parameter and test bounds
    float v = dot(dir, qvec);

    if (v < 0.0f || u + v > det)
        return false;

    // calculate t, scale parameters, ray intersects triangle
    out_tuv.x = dot(e2, qvec);
    out_tuv.y = u; // v
    out_tuv.z = v; // 1 - (u + v)

    out_tuv /= det;

    return true;
}

ML_INLINE bool IsIntersectRayTriangle(const float3& from, const float3& to, const float3& v1, const float3& v2, const float3& v3, float3& out_intersection, float3& out_normal) {
    // find vectors for two edges sharing vert0
    float3 e1 = v2 - v1;
    float3 e2 = v3 - v1;

    // begin calculating determinant - also used to calculate U parameter
    float3 dir = to - from;
    float len = length(dir);
    dir = normalize(dir);

    float3 pvec = cross(dir, e2);

    // if determinant is near zero, ray lies in plane of triangle
    float det = dot(e1, pvec);

    if (det < -1e-6f)
        return false;

    // calculate distance from vert0 to ray origin point "from"
    float3 tvec = from - v1;

    // calculate U parameter and test bounds
    float u = dot(tvec, pvec);

    if (u < 0.0f || u > det)
        return false;

    // prepare to test V parameter
    float3 qvec = cross(tvec, e1);

    // calculate V parameter and test bounds
    float v = dot(dir, qvec);

    if (v < 0.0f || u + v > det)
        return false;

    // calculate t, scale parameters, ray intersects triangle
    float t = dot(e2, qvec) / det;

    if (t > len)
        return false;

    out_intersection = from + dir * t;
    out_normal = normalize(cross(e1, e2));

    return true;
}
