// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_NOTREACHED_H_
#define MINI_CHROMIUM_BASE_NOTREACHED_H_

#include <stdlib.h>

#include "base/check.h"

// Using abort() for NOTREACHED() doesn't support streaming arguments. For a
// more complete implementation we could use LOG(FATAL) but that is currently
// not annotated as [[noreturn]] because ~LogMessage is not. See TODO in
// base/logging.h.
#define NOTREACHED() abort()

// TODO(crbug.com/40580068): Remove this once the NotReachedIsFatal experiment
// has been rolled out in Chromium.
#define NOTREACHED_IN_MIGRATION() DCHECK(false)

#endif  // MINI_CHROMIUM_BASE_NOTREACHED_H_
