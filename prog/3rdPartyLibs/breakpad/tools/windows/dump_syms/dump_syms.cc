// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Windows utility to dump the line number data from a pdb file to
// a text-based format that we can use from the minidump processor.

#include <stdio.h>

#include <string>

#include "common/windows/pdb_source_line_writer.h"

using std::wstring;
using google_breakpad::PDBSourceLineWriter;
using google_breakpad::PDBModuleInfo;

int __cdecl wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    fprintf(stderr, "dump_syms: Usage: %ws <file.[pdb|exe|dll]> [output directory]\n", argv[0]);
    return 1;
  }

  wstring module = argv[1];
  PDBSourceLineWriter writer;
  if (!writer.Open(module, PDBSourceLineWriter::ANY_FILE)) {
    fwprintf(stderr, L"dump_syms: Open failed %s\n", module.c_str());
    return 1;
  }

  FILE *output = stdout;
  if (argc == 3)
  {
    PDBModuleInfo pdb;
    if (!writer.GetModuleInfo(&pdb)) {
      fprintf(stderr, "dump_syms: Could not get PDB info\n");
      return 1;
    }

    wstring dir = argv[2];
    dir += L"\\" + pdb.debug_file;
    if (!CreateDirectory(dir.c_str(), NULL)
        && ERROR_ALREADY_EXISTS != GetLastError())
    {
      fwprintf(stderr, L"dump_syms: Could not create directory %s\n", dir.c_str());
      return 1;
    }
    dir += L"\\" + pdb.debug_identifier;
    if (!CreateDirectory(dir.c_str(), NULL)
        && ERROR_ALREADY_EXISTS != GetLastError())
    {
      fwprintf(stderr, L"dump_syms: Could not create directory %s\n", dir.c_str());
      return 1;
    }
    wstring s = pdb.debug_file;
    wstring p = dir + L"\\" + s.replace(s.rfind(L'.'), 4, L".sym");
    if (_wfopen_s(&output, p.c_str(), L"w"))
    {
      fwprintf(stderr, L"dump_syms: Could not create '%s'\n", p.c_str());
      return 1;
    }
    wprintf(L"dump_syms: Writing symbols to '%s'\n", p.c_str());
  }

  bool success = writer.WriteMap(output);

  if (output != stdout)
    fclose(output);

  if (!success) {
    fprintf(stderr, "dump_syms: WriteMap failed\n");
    return 1;
  }
  writer.Close();

  if (output != stdout)
    wprintf(L"dump_syms: Created symbols for %s\n", module);
  return 0;
}

#include "../../../common/windows/pdb_source_line_writer.cc"
#include "../../../common/windows/omap.cc"
#include "../../../common/windows/dia_util.cc"
