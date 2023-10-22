#ifndef __ASSERT_HLSLI__
#define __ASSERT_HLSLI__

struct AssertEntry
{
  uint assert_message_id;
  uint asserted;
  int stack_id;
  uint __pad;
  float variables[16];
};

#endif