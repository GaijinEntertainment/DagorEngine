require math

[generic]
def beaufort_to_meter_per_second(beaufort : float)
  if (abs(beaufort) > FLT_EPSILON)
    return pow(beaufort, 1.5f) * 0.836f
  return 0.0f

[generic]
def meter_per_second_to_beaufort(mps : float)
  if (abs(mps) > FLT_EPSILON)
    return pow(mps * (1.0f / 0.836f), 2.0f / 3.0f)
  return 0.0f