#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/dag_color.h>
#include <3d/dag_render.h>
#include <math/dag_TMatrix.h>
#include <shaders/dag_shaders.h>

#include <fx/dag_leavesWind.h>
#include <debug/dag_debug.h>


static int windMatrixXId[LeavesWindEffect::WIND_GRP_NUM] = {-2};
static int windMatrixYId[LeavesWindEffect::WIND_GRP_NUM] = {-1};
static int windMatrixZId[LeavesWindEffect::WIND_GRP_NUM] = {-1};


LeavesWindEffect::LeavesWindEffect()
{
  memset(this, 0, sizeof(*this));
  for (int i = 0; i < WIND_GRP_NUM; ++i)
    windMatrixXId[i] = windMatrixYId[i] = windMatrixZId[i] = -1;
}

static void init_var_id()
{
  for (int i = 0; i < LeavesWindEffect::WIND_GRP_NUM; ++i)
  {
    windMatrixXId[i] = ::get_shader_glob_var_id(String(32, "windMatrix%dX", i + 1), true);
    windMatrixYId[i] = ::get_shader_glob_var_id(String(32, "windMatrix%dY", i + 1), true);
    windMatrixZId[i] = ::get_shader_glob_var_id(String(32, "windMatrix%dZ", i + 1), true);
    if (windMatrixXId[i] == -1 || windMatrixYId[i] == -1 || windMatrixZId[i] == -1)
      logwarn("missing wind vars: windMatrixXId[%d]=%d, windMatrixYId[%d]=%d, windMatrixZId[%d]=%d", i, windMatrixXId[i], i,
        windMatrixYId[i], i, windMatrixZId[i]);
  }
}

void LeavesWindEffect::initWind(const DataBlock &blk)
{
  windSeed = blk.getInt("seed", 12345);

  rocking.init(*blk.getBlockByNameEx("rocking"), 0.05f);
  rustling.init(*blk.getBlockByNameEx("rustling"), 0.20f);

  for (int i = 0; i < WIND_GRP_NUM; ++i)
    windGrp[i].init();
  init_var_id();
}


void LeavesWindEffect::updateWind(real dt)
{
  for (int i = 0; i < WIND_GRP_NUM; ++i)
    windGrp[i].update(dt, *this);
}


void LeavesWindEffect::ScalarWindParams::init(const DataBlock &blk, real def_amp)
{
  amp = blk.getReal("amp", def_amp);
  pk = blk.getReal("pk", 1.0f);
  ik = blk.getReal("ik", 0.0);
  dk = blk.getReal("dk", 0.05f);
}


void LeavesWindEffect::ScalarWindValue::init()
{
  pos = 0;
  vel = 0;
  target = 0;
}


void LeavesWindEffect::WindGroup::init()
{
  rocking.init();
  rustling.init();
}


void LeavesWindEffect::ScalarWindValue::update(real dt, const ScalarWindParams &params, LeavesWindEffect &wind)
{
  real ierr = (target - pos) + (target - pos - vel * dt);

  target = _srnd(wind.windSeed) * params.amp;

  real err = target - pos;
  real derr = -vel;

  pos += vel * dt;

  real imp = (err + derr * params.dk + ierr * params.ik) * params.pk;

  vel += imp * dt;

  if (fabs(vel) > 1.f)
  {
    pos = 0;
    vel = 0;
    target = 0;
  }
}


void LeavesWindEffect::WindGroup::update(real dt, LeavesWindEffect &wind)
{
  if (dt > 0.1)
    dt = 0.1;

  rocking.update(dt, wind.rocking, wind);
  rustling.update(dt, wind.rustling, wind);
}


void LeavesWindEffect::WindGroup::calcTm(Matrix3 &tm) { tm = rotyM3(rustling.pos) * rotzM3(rocking.pos); }


void LeavesWindEffect::setShaderVars(const TMatrix &view_itm, real rocking_scale, real rustling_scale)
{
  for (int i = 0; i < WIND_GRP_NUM; ++i)
  {
    Matrix3 tm;
    if (float_nonzero(rocking_scale) || float_nonzero(rustling_scale))
      windGrp[i].calcTm(tm);
    else
      tm = Matrix3::IDENT;

    tm = view_itm % tm;

    ShaderGlobal::set_color4_fast(windMatrixXId[i], Color4(tm[0][0], tm[0][1], tm[0][2], 0));
    ShaderGlobal::set_color4_fast(windMatrixYId[i], Color4(tm[1][0], tm[1][1], tm[1][2], 0));
    ShaderGlobal::set_color4_fast(windMatrixZId[i], -Color4(tm[2][0], tm[2][1], tm[2][2], 0));
  }
}

void LeavesWindEffect::setNoAnimShaderVars(const Point3 &colX, const Point3 &colY, const Point3 &colZ)
{
  if (windMatrixXId[0] < -1)
    init_var_id();
  Color4 x(colX.x, colX.y, colX.z, 0);
  Color4 y(colY.x, colY.y, colY.z, 0);
  Color4 z(-colZ.x, -colZ.y, -colZ.z, 0);
  for (int i = 0; i < WIND_GRP_NUM; ++i)
  {
    ShaderGlobal::set_color4_fast(windMatrixXId[i], x);
    ShaderGlobal::set_color4_fast(windMatrixYId[i], y);
    ShaderGlobal::set_color4_fast(windMatrixZId[i], z);
  }
}
