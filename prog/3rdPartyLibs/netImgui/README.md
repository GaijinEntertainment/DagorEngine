# netImgui

Remote/network rendering of Dear ImGui: draws ImGui UI in a separate client
application over TCP, so the UI does not need to run in-process.

Upstream: netImgui, https://github.com/sammyfreg/netImgui (wiki:
https://github.com/sammyfreg/netImgui/wiki). Vendored version: v1.13.0.

This folder also bundles two other third-party libraries used by netImgui,
each in its own subdirectory:

- `nlohmann_json/` - JSON for Modern C++, https://github.com/nlohmann/json.
  Vendored version: 3.9.1.
- `quicklz/` - QuickLZ compression library, http://www.quicklz.com.
  Vendored version: 1.5.0 final.
