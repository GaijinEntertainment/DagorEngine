#pragma once

// Eden's private pugixml, isolated in the `pugi_eden` namespace.
//
// The engine also links assimp, whose Collada importer bundles its OWN header-only copy of
// pugixml in the global `pugi` namespace. Two independent pugixml copies with the same symbol
// names collide at link time (duplicate `pugi::xml_node::attribute`, ...). Renaming the
// namespace of THIS copy to `pugi_eden` (via the token-level `#define pugi pugi_eden`, which
// only rewrites the bare `pugi` namespace token - never the longer `pugixml`/`pugiconfig`
// identifiers) makes it a distinct, self-contained library that never clashes with assimp's.
//
// The lib itself (pugixml.cpp) is compiled with the same define (see the jamfile), so both
// sides agree on the mangled names. Consumers include THIS header (not <pugixml.hpp>) and use
// the `pugi_eden::` namespace.

#define pugi pugi_eden
#include "pugixml.hpp"
#undef pugi
