#ifndef SNOWFLAKE_HLSL
#define SNOWFLAKE_HLSL 1

struct Snowflake
{
  float2 pos;
  float2 size;
  float opacity;
  float seed;

#ifdef __cplusplus
  bool operator==(const Snowflake&other) const
  {
    return this == &other;
  }
#endif
};

#endif // SNOWFLAKE_HLSL