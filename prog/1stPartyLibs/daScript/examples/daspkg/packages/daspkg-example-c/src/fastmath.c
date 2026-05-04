// Pure C module using daScriptC.h — no C++ needed
#include "daScript/daScriptC.h"
#include <math.h>

// --- Math implementations ---

static float fast_inv_sqrt_impl(float x) {
    float xhalf = 0.5f * x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);
    return x;
}

static float fast_lerp_impl(float a, float b, float t) {
    return a + (b - a) * t;
}

static int fast_clamp_int_impl(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// --- Interop wrappers (vec4f calling convention) ---

static vec4f c_fast_inv_sqrt(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    float x = das_argument_float(args[0]);
    return das_result_float(fast_inv_sqrt_impl(x));
}

static vec4f c_fast_lerp(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    float a = das_argument_float(args[0]);
    float b = das_argument_float(args[1]);
    float t = das_argument_float(args[2]);
    return das_result_float(fast_lerp_impl(a, b, t));
}

static vec4f c_fast_clamp_int(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    int x  = das_argument_int(args[0]);
    int lo = das_argument_int(args[1]);
    int hi = das_argument_int(args[2]);
    return das_result_int(fast_clamp_int_impl(x, lo, hi));
}

// --- Module export ---

#ifdef _MSC_VER
    #define EXPORT_API __declspec(dllexport)
#else
    #define EXPORT_API __attribute__((visibility("default")))
#endif

EXPORT_API das_module * register_dyn_Module_FastMath() {
    das_module * mod = das_module_create("fastmath");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);

    das_module_bind_interop_function(mod, lib, &c_fast_inv_sqrt,
        "fast_inv_sqrt", "c_fast_inv_sqrt", SIDEEFFECTS_none, "f f");
    das_module_bind_interop_function(mod, lib, &c_fast_lerp,
        "fast_lerp", "c_fast_lerp", SIDEEFFECTS_none, "f f f f");
    das_module_bind_interop_function(mod, lib, &c_fast_clamp_int,
        "fast_clamp_int", "c_fast_clamp_int", SIDEEFFECTS_none, "i i i i");

    das_modulegroup_release(lib);
    return mod;
}
