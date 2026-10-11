#pragma once

#include "daScript/misc/platform.h"
#include "daScript/simulate/cast.h"
#include "daScript/simulate/bind_enum.h"

namespace das {

    enum class TrayEventKind : int32_t {
        click = 0,
        double_click = 1,
        right_click = 2,
        menu = 3,
        notification = 4,
    };

    struct TrayEvent {
        TrayEventKind kind;
        int32_t id;
        int32_t x;
        int32_t y;
    };

    struct TrayMenuEntry {
        int32_t id;
        string label;
        bool enabled;
        bool checked;
        bool separator;
    };

    //! One backend per platform; the shared layer owns the single instance, the pending menu, and
    //! the icon and tooltip it replays into a backend created later.
    struct TrayBackend {
        virtual ~TrayBackend() {}
        virtual bool create(const char * tooltip) = 0;
        virtual void setIcon(const uint8_t * rgba8, int32_t width, int32_t height) = 0;
        virtual void setTooltip(const char * text) = 0;
        virtual void setMenu(const vector<TrayMenuEntry> & entries) = 0;
        virtual void poll(vector<TrayEvent> & out) = 0;
        virtual bool notify(const char * title, const char * body) = 0;
    };

    //! Platform file contract: `TrayPlatformCreate` returns null where no host can show an icon.
    bool TrayPlatformAvailable();
    TrayBackend * TrayPlatformCreate();

    //! Menu entry ids are positive and unique per menu; the largest icon side the API accepts.
    const int32_t TRAY_ICON_MAX_SIDE = 1024;

    class Context;
    struct LineInfoArg;
    template <typename TT> struct TArray;
    template <typename ResT, typename ...ArgT> struct TBlock;

    DAS_MOD_API bool TrayAvailable();
    DAS_MOD_API bool TrayCreate(const char * tooltip);
    DAS_MOD_API void TraySetIcon(const TArray<uint8_t> & rgba8, int32_t width, int32_t height, Context * context, LineInfoArg * at);
    DAS_MOD_API void TraySetTooltip(const char * text);
    DAS_MOD_API void TrayMenuClear();
    DAS_MOD_API void TrayMenuAdd(int32_t id, const char * label, bool enabled, bool checked, Context * context, LineInfoArg * at);
    DAS_MOD_API void TrayMenuAddSeparator();
    DAS_MOD_API void TrayMenuCommit();
    DAS_MOD_API void TrayPoll(const TBlock<void, const TrayEvent &> & blk, Context * context, LineInfoArg * at);
    DAS_MOD_API bool TrayNotify(const char * title, const char * body);
    DAS_MOD_API void TrayDestroy();
}

DAS_BIND_ENUM_CAST(das::TrayEventKind)
