// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// SepClient version

namespace sepclient
{

// Version in Semantic version format: MAJOR.MINOR.PATCH
inline constexpr char SEP_VERSION[] = "1.0.2";


// Release date in ISO 8601 format: YYYY-MM-DD
inline constexpr char SEP_VERSION_RELEASE_DATE[] = "2026-03-09";


// Any text-like description of the version; can be empty
// It is normal to contain a list of features, fixes, or other notes

inline constexpr char SEP_VERSION_DESCRIPTION[] = "patch2";


/*
Here will go SEP client version history:

1.0.0, 2026-01-28
Initial version used in Enlisted for Profile service only

1.0.1, 2026-03-06
1.0.2, 2026-03-09
- Fixed bug with no timeout in CAres hostname resolver.
- Fixed log flooding in case of network issues or SEP server being unavailable.
- Do not log user SSO JWT into to game log file any more
- Report less errors to logs when connection is lost, or when network is unavailable,
  since these are normal cases and not errors in most cases. Log warnings instead.
  Those warnings are marked with "[sep_error]" text now.
- SepClient config is merged between two sources during reading since now.
  It makes easier to override just single option from command line.
  E.g. `-config:debug/sep/verboseLogging:b=true`
- More potential compatibility with various platforms when using `eastl::string::sprintf`
- Remove unnecessary delay for SqEventBus (Squirrel connector)

*/

} // namespace sepclient
