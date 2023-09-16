#pragma once


class DataBlock;

namespace tools3d
{
extern bool inited;

bool init(const char *drv_name, const DataBlock *blkTexStreaming);
void destroy();
} // namespace tools3d
