/**
 * pugixml parser - version 1.12
 * --------------------------------------------------------
 * Copyright (C) 2006-2022, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https://pugixml.org/
 *
 * This library is distributed under the MIT License. See notice at the end
 * of this file.
 *
 * This work is based on the pugxml parser, which is:
 * Copyright (C) 2003, by Kristen Wegner (kristen@tima.net)
 */

#ifndef HEADER_PUGICONFIG_HPP
#define HEADER_PUGICONFIG_HPP

// Dagor build of pugixml: compiled as its own static lib (3rdPartyLibs/pugixml.lib),
// NOT header-only. This is a distinct copy from prog/3rdPartyLibs/assimp/contrib/pugixml
// (which is header-only and private to assimp); keep the include paths separate so a
// consumer never mixes the two headers. XPath is disabled (we only need DOM parsing),
// which also drops pugixml's only exception-throwing code path.

// XPath is unused by the das XML deserializer; disabling it shrinks the lib and removes
// pugixml's sole exception dependency (xpath_exception).
#define PUGIXML_NO_XPATH

// Enable long long support so xml_text/xml_attribute::as_llong / as_ullong are available
// (das deserialization reads int64/uint64 tile gids and ids straight from attributes).
#define PUGIXML_HAS_LONG_LONG

#endif

/**
 * Copyright (c) 2006-2022 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
