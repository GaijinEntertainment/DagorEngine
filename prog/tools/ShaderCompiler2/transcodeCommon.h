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

static eastl::vector<uint8_t> get_binary_buffer_with_header(void *data, unsigned int data_size, void *header, unsigned int header_size)
{
  eastl::vector<uint8_t> result(data_size + header_size + 4);
  memcpy(result.data() + 4, header, header_size);
  memcpy(result.data() + header_size + 4, data, data_size);
  *(int *)result.data() = data_size;
  return result;
}
