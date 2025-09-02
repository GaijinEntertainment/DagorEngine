// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_geomTreeMap.h>
#include <math/dag_geomTree.h>
#include <util/dag_roNameMap.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>

void GeomTreeIdxMap::init(const GeomNodeTree &tree, const RoNameMapEx &names)
{
  clear();

  Tab<Pair> map(framemem_ptr());
  map.reserve(names.nameCount());
  for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
    if (int id = names.getNameId(i.index() == 0 ? "@root" : tree.getNodeName(i)); id != -1)
      map.push_back(Pair(id, i));

  entryMap = (Pair *)memalloc(data_size(map), midmem);
  entryCount = map.size();
  mem_copy_to(map, entryMap);
}
