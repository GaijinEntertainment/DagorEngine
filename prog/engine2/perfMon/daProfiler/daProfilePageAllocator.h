#pragma once

namespace da_profiler
{
void *allocate_page(size_t bytes, size_t page_size);
void free_page(void *p, size_t page_size);
} // namespace da_profiler
