// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_tex3d.h>
#include <debug/dag_debug.h>
#include <math/dag_half.h>
#include <math/dag_mathBase.h>

static const int BATCH_PIXELS = 192;

static bool convert_image_line_batch(const void *__restrict input, int width, int in_channels, int in_bits_per_channel, bool in_float,
  void *__restrict output, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb, bool invert,
  float *__restrict float_buf)
{
  unsigned in_step_bits = in_channels * in_bits_per_channel;

  if (in_channels == out_channels && in_bits_per_channel == out_bits_per_channel && in_float == out_float && !invert)
  {
    memcpy(output, input, width * in_step_bits / 8);

    if (swap_rb && in_channels >= 3)
    {
      if (in_bits_per_channel == 8)
        for (int i = 0; i < width * in_channels; i += in_channels)
        {
          uint8_t tmp = ((uint8_t *)output)[i];
          ((uint8_t *)output)[i] = ((uint8_t *)output)[i + 2];
          ((uint8_t *)output)[i + 2] = tmp;
        }

      if (in_bits_per_channel == 16)
        for (int i = 0; i < width * in_channels; i += in_channels)
        {
          uint16_t tmp = ((uint16_t *)output)[i];
          ((uint16_t *)output)[i] = ((uint16_t *)output)[i + 2];
          ((uint16_t *)output)[i + 2] = tmp;
        }

      if (in_bits_per_channel == 32)
        for (int i = 0; i < width * in_channels; i += in_channels)
        {
          uint32_t tmp = ((uint32_t *)output)[i];
          ((uint32_t *)output)[i] = ((uint32_t *)output)[i + 2];
          ((uint32_t *)output)[i + 2] = tmp;
        }
    }

    return true;
  }

  G_ASSERT(width * in_channels <= BATCH_PIXELS * 4);

  unsigned in_count = in_channels * width;

  if (in_float)
  {
    if (in_bits_per_channel == 32)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = ((float *)input)[i];
    }
    else if (in_bits_per_channel == 16)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = half_to_float_unsafe(((uint16_t *)input)[i]);
    }
    else
    {
      G_ASSERTF(0, "expected 16-bit or 32-bit float input");
      return false;
    }
  }
  else // in_float == false
  {
    if (in_bits_per_channel == 1)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = (((uint8_t *)input)[i / 8] & (1 << (7 - (i & 7)))) ? 1.0f : 0.0f;
    }
    else if (in_bits_per_channel == 4 * 8)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = ((uint32_t *)input)[i] * (1.f / float(UINT_MAX));
    }
    else if (in_bits_per_channel == 2 * 8)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = ((uint16_t *)input)[i] * (1.f / 65535);
    }
    else if (in_bits_per_channel == 1 * 8)
    {
      for (unsigned i = 0; i < in_count; i++)
        float_buf[i] = ((uint8_t *)input)[i] * (1.f / 255);
    }
    else
    {
      G_ASSERTF(0, "expected 1-bit, 8-bit, 16-bit or 32-bit integer input");
      return false;
    }
  }

  if (invert)
  {
    for (unsigned i = 0; i < in_count; i++)
      float_buf[i] = 1.0f - float_buf[i];
  }


  unsigned in_pos = 0;
  unsigned out_pos = 0;

  if (out_bits_per_channel == 1 && out_channels == 1)
  {
    for (unsigned i = 0; i < width / 8; i++)
    {
      uint8_t b = 0;
      for (unsigned bit = 0; bit < 8; bit++)
      {
        for (unsigned c = 0; c < min(in_channels, 3); c++)
          if (float_buf[in_pos + c] >= 0.5f)
            b |= 1 << (7 - bit);
        in_pos += in_channels;
      }

      ((uint8_t *)output)[i] = b;
      out_pos++;
    }
  }
  else
  {
    if (in_channels == 2)
    {
      in_channels = 4;
      in_count = in_channels * width;
      for (int i = width - 1; i >= 0; i--)
      {
        float v0 = float_buf[i * 2 + 0];
        float v1 = float_buf[i * 2 + 1];
        float_buf[i * 4 + 0] = v0;
        float_buf[i * 4 + 1] = v1;
        float_buf[i * 4 + 2] = 0.0f;
        float_buf[i * 4 + 3] = 1.0f;
      }
    }

    if (in_channels == 3)
    {
      in_channels = 4;
      in_count = in_channels * width;
      for (int i = width - 1; i >= 0; i--)
      {
        float v0 = float_buf[i * 3 + 0];
        float v1 = float_buf[i * 3 + 1];
        float v2 = float_buf[i * 3 + 2];
        float_buf[i * 4 + 0] = v0;
        float_buf[i * 4 + 1] = v1;
        float_buf[i * 4 + 2] = v2;
        float_buf[i * 4 + 3] = 1.0f;
      }
    }


    unsigned step = (in_channels <= 2) ? 0 : 1;

    if (swap_rb && in_channels >= 3)
      for (unsigned i = 0; i < width * in_channels; i += in_channels)
      {
        float tmp = float_buf[i];
        float_buf[i] = float_buf[i + 2];
        float_buf[i + 2] = tmp;
      }

    for (unsigned i = 0; i < width; i++)
    {
      for (unsigned ch = 0; ch < out_channels; ch++)
      {
        if (!out_float && out_bits_per_channel == 8)
          ((uint8_t *)output)[out_pos] = (uint8_t)clamp(int(255.0f * float_buf[in_pos] + 0.5f), 0, 255);
        else if (!out_float && out_bits_per_channel == 16)
          ((uint16_t *)output)[out_pos] = (uint16_t)clamp(int(65535.0f * float_buf[in_pos] + 0.5f), 0, 65535);
        else if (!out_float && out_bits_per_channel == 32)
          ((uint32_t *)output)[out_pos] =
            (uint32_t)clamp(int64_t(float(0xFFFFFFFFu) * float_buf[in_pos]), int64_t(0), int64_t(0xFFFFFFFFu));
        else if (out_float && out_bits_per_channel == 16)
          ((uint16_t *)output)[out_pos] = float_to_half_unsafe(float_buf[in_pos]);
        else if (out_float && out_bits_per_channel == 32)
          ((float *)output)[out_pos] = float_buf[in_pos];
        else
        {
          G_ASSERTF(0, "invalid output format out_float=%d out_bits_per_channel=%d", int(out_float), out_bits_per_channel);
          return false;
        }

        in_pos += step;
        out_pos++;
      }
      in_pos -= step * out_channels;
      in_pos += in_channels;
    }
  }

  return true;
}


bool convert_image_line(const void *__restrict input, int width, int in_channels, int in_bits_per_channel, bool in_float,
  void *__restrict output, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb, bool invert)
{
  G_ASSERT(input != output);
  G_ASSERT(input);
  G_ASSERT(output);
  G_STATIC_ASSERT(BATCH_PIXELS % 8 == 0 && BATCH_PIXELS % 3 == 0);

  float floatBuf[BATCH_PIXELS * 4];

  for (int x = 0; x < width; x += BATCH_PIXELS)
  {
    int w = min(BATCH_PIXELS, width - x);
    int inputOffset = x * in_channels * in_bits_per_channel / 8;
    int outputOffset = x * out_channels * out_bits_per_channel / 8;
    if (!convert_image_line_batch((char *)input + inputOffset, w, in_channels, in_bits_per_channel, in_float,
          (char *)output + outputOffset, out_channels, out_bits_per_channel, out_float, swap_rb, invert, floatBuf))
      return false;
  }

  return true;
}
