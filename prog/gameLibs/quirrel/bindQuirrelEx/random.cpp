#include <quirrel/sqModules/sqModules.h>
#include <math/random/dag_random.h>
#include <dag_noise/dag_uint_noise.h>

namespace bindquirrel
{

static int script_rnd_seed = 1233453241;

static int script_rnd() { return _rnd(script_rnd_seed); }
static float script_frnd() { return _frnd(script_rnd_seed); }
static float script_srnd() { return _srnd(script_rnd_seed); }
static float script_rnd_float(float a, float b) { return _rnd_float(script_rnd_seed, a, b); }
static int script_rnd_int(int a, int b) { return _rnd_int(script_rnd_seed, a, b); }

static void script_set_rnd_seed(int seed) { script_rnd_seed = seed; }
static int script_get_rnd_seed() { return script_rnd_seed; }

static SQInteger gauss_rnd_safe(HSQUIRRELVM vm)
{
  SQInteger n = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &n)));
  if (n < 0 || n > 2)
    return sq_throwerror(vm, "Argument (lookup table number) must be in range [0..2]");
  sq_pushfloat(vm, _gauss_rnd(script_rnd_seed, n));
  return 1;
}


void register_random(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());

  ///@module dagor.random
  exports.Func("rnd", script_rnd)
    .Func("frnd", script_frnd)
    .Func("srnd", script_srnd)
    .Func("rnd_float", script_rnd_float)
    .Func("rnd_int", script_rnd_int)
    .Func("set_rnd_seed", script_set_rnd_seed)
    .Func("get_rnd_seed", script_get_rnd_seed)
    .SquirrelFunc("gauss_rnd", gauss_rnd_safe, 2, ".n")

    //  !!do not bind _rnd, _frnd, _srnd functions! seed cannot be setup via script as seed is reference, not int

    .Func("uint32_hash", uint32_hash)
    .Func("uint_noise1D", uint_noise1D)
    .Func("uint_noise2D", uint_noise2D)
    .Func("uint_noise3D", uint_noise3D);

  module_mgr->addNativeModule("dagor.random", exports);
}


} // namespace bindquirrel
