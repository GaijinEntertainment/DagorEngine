typedef uint8_t BYTE;
#define g_main g_default_clear_ps_src
#include "shaders/default_clear_ps.h"
#undef g_main

#define g_main g_default_clear_vs_src
#include "shaders/default_clear_vs.h"
#undef g_main

#define g_main g_default_copy_ps_src
#include "shaders/default_copy_ps.h"
#undef g_main

#define g_main g_default_copy_vs_src
#include "shaders/default_copy_vs.h"
#undef g_main

#define g_main g_default_debug_ps_src
#include "shaders/default_debug_ps.h"
#undef g_main

#define g_main g_default_debug_vs_src
#include "shaders/default_debug_vs.h"
#undef g_main
