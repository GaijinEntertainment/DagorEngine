
#pragma once

#include <util/dag_stdint.h>

namespace danet
{

class BitStream;

void writeSize(BitStream &to, uint32_t size_in_bits);
uint32_t readSize(const BitStream &from);

} // namespace danet
