// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN

#include <supp/dag_comPtr.h>

#include <EASTL/utility.h>
#include <EASTL/optional.h>

class String;
struct IDXGIOutput;
struct IDXGIFactory1;

String get_monitor_name_from_output(IDXGIOutput *pOutput);

/* Returns with the monitor's ptr if it exists with the given name,
 otherwise returns with nullptr.
 All parameters must be valid pointers. */
ComPtr<IDXGIOutput> get_output_monitor_by_name(IDXGIFactory1 *factory, const char *monitorName);

/* Finds a monitor with the given name and returns with that.
 if isn't exist return with the primary monitor's pointer if it's available. */
ComPtr<IDXGIOutput> get_output_monitor_by_name_or_default(IDXGIFactory1 *factory, const char *monitorName = nullptr);

/* Returns with the primary monitor's pointer if it's available. */
ComPtr<IDXGIOutput> get_default_monitor(IDXGIFactory1 *factory);

bool resolutions_have_same_ratio(eastl::pair<uint32_t, uint32_t> l_res, eastl::pair<uint32_t, uint32_t> r_res);

eastl::optional<eastl::pair<uint32_t, uint32_t>> get_recommended_resolution(IDXGIOutput *dxgi_output);

#endif
