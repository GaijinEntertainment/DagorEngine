// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

static int get_imm_regs_count(const char *src, int len)
{
  int count = 0;
  size_t pos = 0;
  auto source = eastl::string_view(src, len);
  for (;;)
  {
    pos = source.find("srt_immediate_data.data[", pos);
    if (pos == eastl::string_view::npos)
      break;
    pos++;
    count++;
  }
  return count;
}

static int find_string(const char *src, int len, const char *pattern)
{
  auto source = eastl::string_view(src, len);
  int pos = source.find(pattern, 0);
  return (pos == eastl::string_view::npos) ? -1 : pos;
}

static eastl::tuple<eastl::vector<uint8_t>, eastl::vector<uint8_t>> get_binary_buffer_with_header(void *data, unsigned int data_size,
  void *header, unsigned int header_size)
{
  eastl::vector<uint8_t> metadata(header_size + sizeof(data_size));
  memcpy(metadata.data() + 0, &data_size, sizeof(data_size));
  memcpy(metadata.data() + sizeof(data_size), header, header_size);

  eastl::vector<uint8_t> bytecode(data_size);
  memcpy(bytecode.data(), data, data_size);

  return {metadata, bytecode};
}
