#pragma once

// one of three identical copies - include/daScript/das_config.h, include/misc/include_fmt.h,
// cmake/das_config_eastl/daScript/das_config.h; whichever a TU includes first must stand alone
namespace das {
  void das_throw(const char * msg);
}
#if (!defined(DAS_ENABLE_EXCEPTIONS)) || (!DAS_ENABLE_EXCEPTIONS)
#ifndef DAS_FMT_THROW_DEFINED
#define DAS_FMT_THROW_DEFINED
namespace das {
  void das_stash_throw(const char * msg);
  void das_throw_stashed();
  // the panic longjmp never unwinds (POSIX never does; the JIT stack cannot be unwound);
  // the temporary dies when the stash statement ends - the jump crosses nothing live
  template <typename BuildAndStash>
  inline void das_stash_then_throw(BuildAndStash && buildAndStash) {
      buildAndStash();
      das_throw_stashed();
  }
}
#define FMT_THROW(x)    das::das_stash_then_throw([&]{ das::das_stash_throw(((x).what())); })
#endif
#endif

#include <fmt/core.h>
#include <fmt/format.h>
