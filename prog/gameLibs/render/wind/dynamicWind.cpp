#include <render/wind/dynamicWind.h>

#include <math/dag_Point2.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <math/dag_hlsl_floatx.h>

#include <render/wind/rendinst_bend_desc.hlsli>


void DynamicWind::spawnWave(int index)
{
  WindWave waveToSpawn;
  Point3 cross = normalize(direction % Point3(0.0f, 1.0, 0.0f));
  float angleSpread = cvt(beaufortScale, 0, MAX_BEAUFORT_SCALE, desc.angleSpread.x, desc.angleSpread.y);
  angleSpread = rnd_float(-angleSpread, angleSpread);
  angleSpread = DegToRad(angleSpread);
  Matrix3 rot = rotyM3(angleSpread);
  float amount = rnd_float(-0.5f, 0.5f);
  waveToSpawn.direction = rot * spawner[index].direction;
  waveToSpawn.position = spawner[index].position + cross * (amount * spawner[index].width);
  amount = rnd_float(0.0f, 0.75f);
  waveToSpawn.position += direction * desc.internalCascadeBoxSize * amount;

  waveToSpawn.radius = rnd_float(desc.windRadiusLim.x, desc.windRadiusLim.y);
  waveToSpawn.speed = cvt(beaufortScale, 0, MAX_BEAUFORT_SCALE, desc.windSpeedLim.x, desc.windSpeedLim.y);
  waveToSpawn.lifeTimeSpan = (desc.internalCascadeBoxSize / waveToSpawn.speed) * (1.0 - amount);
  waveToSpawn.lifeTime = 0.0f;
  waveToSpawn.alive = true;

  waveToSpawn.maxStrength = cvt(beaufortScale, 0, MAX_BEAUFORT_SCALE, 0.1f, MAX_POWER_FOR_DYNAMIC_WIND);
  waveToSpawn.strength = 0.0f;

  int waveIndex = findWavesIndex();
  if (waveIndex == -1)
  {
    int amount = waves.size();
    waves.resize(amount * 2);
    waves[amount] = waveToSpawn;
  }
  else
    waves[waveIndex] = waveToSpawn;

  spawner[index].lastWaveSpawnTime = 0.0f;
  float oneBoxPassTime = desc.internalCascadeBoxSize / waveToSpawn.speed;
  spawner[index].randNextSpawnTime = safediv(rnd_float(oneBoxPassTime * 0.75f, oneBoxPassTime * 1.25f), desc.windSpawnTimeMult);
}

void DynamicWind::firstSimulation(const Point3 &pos)
{
  float dt = PRE_SIMULATION_TIME / (float)PRE_SIMULATION_STEP;
  for (int i = 0; i < PRE_SIMULATION_STEP; ++i)
    update(pos, dt);
}

void DynamicWind::reset()
{
  clear_and_shrink(spawner);
  clear_and_shrink(waves);
}

int DynamicWind::findWavesIndex()
{
  for (int i = 0; i < waves.size(); ++i)
    if (!waves[i].alive)
      return i;
  return -1;
}

DynamicWind::DynamicWind() : beaufortScale(0), isFirstIteration(true), isEnabled(false)
{
  reset();
  desc.startingPoolSize = 0;
  desc.internalCascadeBoxSize = 0.0f;
}

DynamicWind::~DynamicWind() { reset(); }

void DynamicWind::update(const Point3 &pos, float dt)
{
  if (!isEnabled)
    return;
  if (isFirstIteration)
  {
    isFirstIteration = false;
    firstSimulation(pos);
  }
  for (int i = 0; i < spawner.size(); ++i)
  {
    Point3 cross = normalize(direction % Point3(0.0f, 1.0, 0.0f));
    cross *= (i % 2) == 0 ? -1 : 1;
    spawner[i].position =
      pos - (spawner[i].direction * (desc.internalCascadeBoxSize * 0.5f)) + cross * (spawner[i].width * ((i + 1) / 2));
    spawner[i].lastWaveSpawnTime += dt;
    if (spawner[i].lastWaveSpawnTime >= spawner[i].randNextSpawnTime)
      spawnWave(i);
  }

  for (int i = 0; i < waves.size(); ++i)
  {
    WindWave wave = waves[i];
    if (!wave.alive)
      continue;
    waves[i].lifeTime += dt;
    if (waves[i].lifeTime >= waves[i].lifeTimeSpan)
    {
      waves[i].alive = false;
      continue;
    }

    auto pow8 = [](float a) {
      a *= a; // 2
      a *= a; // 4
      a *= a; // 8
      return a;
    };

    float relativTime = waves[i].lifeTime / waves[i].lifeTimeSpan;
    // https://graphtoy.com/?f1(x,t)=saturate(sqrt(sin(x*PI))*2)&v1=false&f2(x,t)=min(pow(x,%208),%20pow(x,%208))&v2=false&f3(x,t)=1%20-%20pow((1-x)%20*%202%20-%201,%208)&v3=true&f4(x,t)=&v4=false&f5(x,t)=&v5=false&f6(x,t)=&v6=false&grid=NaN&coords=1.1335699358958669,0.0007029822837792965,4.205926793776712
    waves[i].strength = (1.0f - pow8((1.0f - relativTime) * 2.0f - 1.0f)) * waves[i].maxStrength;
    waves[i].position += waves[i].direction * (waves[i].speed * dt);
  }
  waves.shrink_to_fit();
}

void DynamicWind::init(int _beaufortScale, const Point3 &_direction)
{
  reset();

  beaufortScale = _beaufortScale;
  direction = normalize(_direction);
  isFirstIteration = true;
  int amountOfSpawner = (int)cvt(beaufortScale, 0, MAX_BEAUFORT_SCALE, desc.spawnerAmountLim.x, desc.spawnerAmountLim.y);
  amountOfSpawner = (amountOfSpawner % 2) == 0 ? amountOfSpawner + 1 : amountOfSpawner; // odd for symmetry
  float spawnerWidth = safediv(desc.internalCascadeBoxSize, (float)amountOfSpawner);
  spawner.resize(amountOfSpawner);
  spawnTimeLim = Point2(1.5f, 3.0f);
  for (int i = 0; i < amountOfSpawner; ++i)
  {
    spawner[i].direction = direction;
    spawner[i].width = spawnerWidth;
    spawner[i].lastWaveSpawnTime = 0.0f;
    float potentialSpeed = cvt(beaufortScale, 0, MAX_BEAUFORT_SCALE, desc.windSpeedLim.x, desc.windSpeedLim.y);
    float oneBoxPassTime = desc.internalCascadeBoxSize / potentialSpeed;
    spawner[i].randNextSpawnTime = safediv(rnd_float(oneBoxPassTime * 0.75f, oneBoxPassTime * 1.25f), desc.windSpawnTimeMult);
  }

  waves.resize(desc.startingPoolSize);
}

void DynamicWind::loadWindDesc(WindDesc &_desc) { desc = _desc; }
