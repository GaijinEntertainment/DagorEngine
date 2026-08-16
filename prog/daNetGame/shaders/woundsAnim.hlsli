inline float hole_close_ease(float spawnTime, float holdDuration, float closeDuration, float openDuration, float curTime)
{
  float t = curTime - spawnTime;
  float closeT = closeDuration > 0.f ? saturate((t - openDuration - holdDuration) / max(closeDuration, 1e-6f)) : 0.f;
  return 1.f - closeT * closeT * (3.f - 2.f * closeT);
}

inline float animated_hole_radius(
  float spawnTime, float holdDuration, float closeDuration, float openDuration, float pulseAmp, float curTime)
{
  float t = curTime - spawnTime;
  float openS = openDuration > 0.f ? smoothstep(0.f, max(openDuration, 1e-6f), t) : 1.f;
  float pulse = max(0.f, 1.f + pulseAmp * sin(6.2831853f * t));
  return openS * pulse * hole_close_ease(spawnTime, holdDuration, closeDuration, openDuration, curTime);
}
