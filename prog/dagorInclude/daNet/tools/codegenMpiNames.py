from pyparsing import *
import string
import sys
import re

templ = """#ifndef __GAIJIN_MPI_NAMES_TABLE__
#define __GAIJIN_MPI_NAMES_TABLE__
#pragma once

static const char* mpiNamesTable[] =
{{
#if DEBUG_INTERFACE_ENABLED
{ids}
#else
  ""
#endif
}};

#endif // __GAIJIN_MPI_NAMES_TABLE__

"""

debug_function_format = """#pragma once
#if DEBUG_INTERFACE_ENABLED
#include <daNet/mpi.h>
#endif

class MpiBlkGenerator
{{
public:
  static DataBlock generateBlkFromStream(danet::BitStream& bs)
  {{
  #if DEBUG_INTERFACE_ENABLED
    mpi::ObjectID oid = mpi::INVALID_OBJECT_ID;
    mpi::MessageID mid = mpi::INVALID_MESSAGE_ID;
    bs.Read(oid);
    bs.Read(mid);
    DataBlock blk;
    switch (mid & 0x0fff)
    {{
{msgs}
    }};
    blk.addInt("objectId", oid);
    return blk;
  #else
    return DataBlock();
  #endif
  }}
}};

"""

message_format = """
    case {id}:
      {{
        DataBlock resBlk = {name}::getDebugMpiBlk(bs);
        DataBlock* finalBlk = blk.addNewBlock(&resBlk, "{name}");
        finalBlk->addInt("objectID", oid);
        return blk;
      }}
      break;
"""


class parseFile:
  def __init__(self):
    self.messages = {}
    self.ids = {}
    self.maxId = 0
   
  def parse(self, inputFile, outputFile, debugFile):
    def parseRecord(str_, loc, toks):
      id_ = int(toks[1])
      self.messages[toks[0]] = id_
      self.ids[id_] = toks[0]
      self.maxId = max(self.maxId, id_)

    record = (Word(alphanums + '_') + Keyword("=").suppress() + Word(nums)).setParseAction(parseRecord)
    ZeroOrMore(record).parseFile(inputFile)
    records = sorted(self.messages.items(), lambda (k1, v1), (k2, v2): cmp(v1, v2))

    def to_id(msg, id_): return '  "%s",' % msg if msg else '  "UNKNOWN_%s",' % id_
    open(outputFile, "w").write(templ.format(ids='\n'.join([to_id(self.ids.get(r), r) for r in range(self.maxId+1)])))
    print  "generate '%s'" % outputFile

    messagesStr = ""
    for r in range(self.maxId + 1):
      if self.ids.get(r) != None:
        messagesStr += message_format.format(id = r, name = self.ids.get(r))
    open(debugFile, "w").write(debug_function_format.format(msgs = messagesStr))
    print  "generate '%s'" % debugFile


if __name__ == '__main__':
  if len(sys.argv) != 4:
    print "Wrong number of arguments presented."
    print "Usage: codegenMpiNames.py idFile outputFile debugFile"
  else:
    inputFile = sys.argv[1]
    outputFile = sys.argv[2]
    debugFile = sys.argv[3]
    parser = parseFile()
    parser.parse(inputFile, outputFile, debugFile)
    

