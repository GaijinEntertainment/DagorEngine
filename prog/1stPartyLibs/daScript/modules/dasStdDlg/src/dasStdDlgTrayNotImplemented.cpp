#include "daScript/misc/platform.h"

#include "dasStdDlgTray.h"

namespace das {

    bool TrayPlatformAvailable() {
        return false;
    }

    TrayBackend * TrayPlatformCreate() {
        return nullptr;
    }
}
