// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/skies.h>
#include <net/userid.h>

DaSkies *lightning_get_skies() { return get_daskies(); }
int lightning_get_session_id() { return net::get_session_id(); }
