// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_fileTracker.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_globDef.h>
#include <generic/dag_tab.h>


class GenFileTrackerService : public IFileChangeTracker
{
public:
  GenFileTrackerService() : ftime(midmem), links(midmem), hlist(midmem), hlistLocal(midmem), curLink(0), curHofs(0) {}

  virtual const char *getFileName(int file_name_id) { return (file_name_id < 0) ? NULL : fnames.getName(file_name_id); }
  virtual int addFileChangeTracking(const char *fname)
  {
    if (!fname)
      return -1;

    int id = fnames.addNameId(fname);
    G_ASSERT(id <= ftime.size());
    if (id == ftime.size())
      ftime.push_back(getFileTime(fname));
    return id;
  }
  virtual void subscribeUpdateNotify(int file_name_id, IFileChangedNotify *notify)
  {
    if (file_name_id < 0)
      return;

    int hidx = 0;
    for (int i = 0; i < links.size(); i++)
      if (links[i].fnameId == file_name_id)
      {
        // check handler against existing
        for (int j = 0; j < links[i].hCount; j++)
          if (hlist[hidx + j] == notify)
            return;

        // add handler to existing
        insert_item_at(hlist, hidx, notify);
        if (curHofs > hidx)
          curHofs++;
        links[i].hCount++;
        return;
      }
      else
        hidx += links[i].hCount;

    // no handlers for file, register new
    LinkRec &r = links.push_back();
    r.fnameId = file_name_id;
    r.hCount = 1;
    hlist.push_back(notify);
  }
  virtual void unsubscribeUpdateNotify(int file_name_id, IFileChangedNotify *notify)
  {
    if (file_name_id < 0)
      return;

    int hidx = 0;
    for (int i = 0; i < links.size(); i++)
      if (links[i].fnameId == file_name_id)
      {
        // find handler
        for (int j = 0; j < links[i].hCount; j++)
          if (hlist[hidx + j] == notify)
          {
            erase_items(hlist, hidx + j, 1);
            if (curHofs > hidx + j)
              curHofs--;

            links[i].hCount--;
            if (!links[i].hCount)
            {
              erase_items(links, i, 1);
              if (curLink > i)
                curLink--;
            }
            return;
          }

        // no such handler for thsi file
        return;
      }
      else
        hidx += links[i].hCount;

    // file not found, no unregister needed
  }

  virtual void lazyCheckOnAct()
  {
    checkOneFile();
    if (curLink >= links.size())
    {
      curLink = 0;
      curHofs = 0;
    }
  }
  virtual void immediateCheck()
  {
    curLink = 0;
    curHofs = 0;
    while (curLink < links.size())
      checkOneFile();
    curLink = 0;
    curHofs = 0;
  }

  void checkOneFile()
  {
    if (curLink >= links.size())
      return;

    int name_id = links[curLink].fnameId;
    int64_t ft = getFileTime(fnames.getName(name_id));

    if (ft != ftime[name_id])
    {
      ftime[name_id] = ft;

      hlistLocal = make_span_const(hlist).subspan(curHofs, links[curLink].hCount);
      for (int i = 0; i < hlistLocal.size(); i++)
        hlistLocal[i]->onFileChanged(name_id);
    }

    curHofs += links[curLink].hCount;
    curLink++;
  }

  static int64_t getFileTime(const char *fn)
  {
    DagorStat st;
    return df_stat(fn, &st) == 0 ? st.mtime : -1;
  }

protected:
  struct LinkRec
  {
    unsigned fnameId : 22;
    unsigned hCount : 10;
  };

  // fnames and ftime are parallel arrays
  OAHashNameMap<true> fnames;
  Tab<int64_t> ftime;

  // links array contains sequental indexing of hlist
  Tab<LinkRec> links;
  // handlers list indexed by links
  Tab<IFileChangedNotify *> hlist;
  Tab<IFileChangedNotify *> hlistLocal;

  int curLink, curHofs;
};

static GenFileTrackerService srv;

void *get_generic_file_change_tracking_service() { return &srv; }
