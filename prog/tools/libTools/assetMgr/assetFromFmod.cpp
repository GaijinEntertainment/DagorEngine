// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetMgr.h>
#if _TARGET_PC_WIN
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <sound/dag_fmod4.h>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_event.hpp>
#include "assetCreate.h"
#include <util/dag_fastIntList.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#endif

bool DagorAssetMgr::mountFmodEvents(const char *mount_folder_name)
{
#if _TARGET_PC_WIN
  int atype = typeNames.getNameId("fmodEvent");
  if (atype == -1 || !dagor_fmod4::fmod_eventsys)
    return true;

  if (!folders.size())
  {
    folders.push_back(new DagorAssetFolder(-1, "Root", NULL));
    perFolderStartAssetIdx.push_back(0);
  }

  int s_asset_num = assets.size();
  int nsid = nspaceNames.addNameId("fmod");
  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(0, mount_folder_name, "::"));
  folders[0]->subFolderIdx.push_back(fidx);
  perFolderStartAssetIdx.push_back(assets.size());

  post_msg(*msgPipe, msgPipe->NOTE, "mounting FMOD events to %s...", mount_folder_name);

  int numProjects;
  if (dagor_fmod4::fmod_eventsys->getNumProjects(&numProjects) == FMOD_OK)
    for (int iPrj = 0; iPrj < numProjects; iPrj++)
    {
      int numGroups;

      FMOD::EventProject *proj;
      if (dagor_fmod4::fmod_eventsys->getProjectByIndex(iPrj, &proj) != FMOD_OK)
        continue;

      if (proj->getNumGroups(&numGroups) != FMOD_OK)
        continue;

      FMOD_EVENT_PROJECTINFO info;
      if (proj->getInfo(&info) != FMOD_OK)
        continue;

      int proj_fidx = folders.size();
      folders.push_back(new DagorAssetFolder(fidx, info.name, "::"));
      folders[fidx]->subFolderIdx.push_back(proj_fidx);
      perFolderStartAssetIdx.push_back(assets.size());

      for (int i = 0; i < numGroups; i++)
      {
        FMOD::EventGroup *grp;
        if (proj->getGroupByIndex(i, true, &grp) == FMOD_OK)
          addFmodAssets(proj_fidx, info.name, grp);
      }
    }

  updateGlobUniqueFlags();

  post_msg(*msgPipe, msgPipe->NOTE, "added %d fmodEvent assets", assets.size() - s_asset_num);
#endif
  return true;
}

void DagorAssetMgr::addFmodAssets(int parent_fidx, const char *base_path, void *fmod_group)
{
#if _TARGET_PC_WIN
  FMOD::EventGroup *grp = (FMOD::EventGroup *)fmod_group;
  int atype = typeNames.getNameId("fmodEvent");
  int nsid = nspaceNames.getNameId("fmod");

  char *groupName;
  int groupIndex;
  if (grp->getInfo(&groupIndex, &groupName) != FMOD_OK)
    return;

  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(parent_fidx, groupName, "::"));
  folders[parent_fidx]->subFolderIdx.push_back(fidx);
  perFolderStartAssetIdx.push_back(assets.size());

  String groupPath(256, "%s|%s", base_path, groupName);

  DagorAssetPrivate *ca = NULL;

  int numEvents;
  if (grp->getNumEvents(&numEvents) == FMOD_OK)
  {
    for (int i = 0; i < numEvents; i++)
    {
      FMOD::Event *evt;
      if (grp->getEventByIndex(i, FMOD_EVENT_INFOONLY, &evt) == FMOD_OK)
      {
        int eventIndex;
        char *eventName;
        if (evt->getInfo(&eventIndex, &eventName, NULL) == FMOD_OK)
        {
          String eventPath(128, "%s|%s", groupPath.str(), eventName);

          if (!ca)
            ca = new DagorAssetPrivate(*this);

          ca->setNames(assetNames.addNameId(eventPath), nsid, true);
          if (perTypeNameIds[atype].addInt(ca->getNameId()))
          {
            ca->setAssetData(fidx, -1, atype);
            assets.push_back(ca);
            ca = NULL;
          }
          else
            post_msg(*msgPipe, msgPipe->WARNING, "duplicate asset %s of type <%s> in namespace %s", ca->getName(),
              typeNames.getName(atype), nspaceNames.getName(nsid));
        }
      }
    }
  }
  del_it(ca);

  int numGroups;
  if (grp->getNumGroups(&numGroups) == FMOD_OK)
    for (int i = 0; i < numGroups; i++)
    {
      FMOD::EventGroup *next;
      if (grp->getGroupByIndex(i, true, &next) == FMOD_OK)
        addFmodAssets(fidx, groupPath, next);
    }
#endif
}
