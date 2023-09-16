#if BUILD_AOT_LIB
extern size_t math_aot_DAS_pull_AOT;
size_t pull_das_math_aot_lib() { return math_aot_DAS_pull_AOT; }
#else
size_t pull_das_math_aot_lib() { return 0; }
#endif
