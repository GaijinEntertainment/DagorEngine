// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <distribComp2/distribComp2.h>
#include <ioSys/dag_genIo.h>
#include <libTools/util/progressInd.h>
#include <generic/dag_tab.h>

namespace DistribCompProvider2
{
static Tab<IDistribCompSubmittingClientMaker *> submit_client_makers(inimem_ptr());
static Tab<IDistribCompComputingClientMaker *> comp_client_makers(inimem_ptr());
static Tab<IDistribCompServerMaker *> server_makers(inimem_ptr());

int find_submit_client(int four_cc)
{
  for (int i = 0; i < submit_client_makers.size(); i++)
    if (submit_client_makers[i]->getFourCC() == four_cc)
      return i;
  return -1;
}
int find_comp_client(int four_cc)
{
  for (int i = 0; i < comp_client_makers.size(); i++)
    if (comp_client_makers[i]->getFourCC() == four_cc)
      return i;
  return -1;
}
int find_server(int four_cc)
{
  for (int i = 0; i < server_makers.size(); i++)
    if (server_makers[i]->getFourCC() == four_cc)
      return i;
  return -1;
}
} // namespace DistribCompProvider2

bool DistribCompProvider2::registerMaker(IDistribCompSubmittingClientMaker *maker)
{
  if (find_submit_client(maker->getFourCC()) != -1)
  {
    maker->unregister();
    return false;
  }
  submit_client_makers.push_back(maker);
  return true;
}
bool DistribCompProvider2::registerMaker(IDistribCompComputingClientMaker *maker)
{
  if (find_comp_client(maker->getFourCC()) != -1)
  {
    maker->unregister();
    return false;
  }
  comp_client_makers.push_back(maker);
  return true;
}
bool DistribCompProvider2::registerMaker(IDistribCompServerMaker *maker)
{
  if (find_server(maker->getFourCC()) != -1)
  {
    maker->unregister();
    return false;
  }
  server_makers.push_back(maker);
  return true;
}

bool DistribCompProvider2::unregisterMaker(int four_cc, bool subm_cli, bool comp_cli, bool server)
{
  int idx;
  bool unreg = false;

  if (subm_cli)
  {
    idx = find_submit_client(four_cc);
    if (idx != -1)
    {
      submit_client_makers[idx]->unregister();
      erase_items(submit_client_makers, idx, 1);
      unreg = true;
    }
  }

  if (comp_cli)
  {
    idx = find_comp_client(four_cc);
    if (idx != -1)
    {
      comp_client_makers[idx]->unregister();
      erase_items(comp_client_makers, idx, 1);
      unreg = true;
    }
  }

  if (server)
  {
    idx = find_server(four_cc);
    if (idx != -1)
    {
      server_makers[idx]->unregister();
      erase_items(server_makers, idx, 1);
      unreg = true;
    }
  }

  return unreg;
}

void DistribCompProvider2::unregisterAll()
{
  int i;

  for (i = 0; i < submit_client_makers.size(); i++)
    submit_client_makers[i]->unregister();
  for (i = 0; i < comp_client_makers.size(); i++)
    comp_client_makers[i]->unregister();
  for (i = 0; i < server_makers.size(); i++)
    server_makers[i]->unregister();

  clear_and_shrink(submit_client_makers);
  clear_and_shrink(comp_client_makers);
  clear_and_shrink(server_makers);
}


// creates client task for data submission
IDistribCompSubmittingClientTask *DistribCompProvider2::newTask(int task_fourcc)
{
  int idx = find_submit_client(task_fourcc);
  if (idx == -1)
    return NULL;
  return submit_client_makers[idx]->make();
}

// creates server task from data submitted by client
IDistribCompServerTask *DistribCompProvider2::receiveServerTask(IGenLoad &crd)
{
  int four_cc = crd.readInt(), ver = crd.readInt();
  crd.seekrel(-8);

  int idx = find_server(four_cc);
  if (idx == -1)
    return NULL;
  if (server_makers[idx]->getTaskFormatVer() != ver)
    return NULL;

  IDistribCompServerTask *svr;
  svr = server_makers[idx]->make();
  if (!svr->receiveSourceData(crd))
  {
    delete svr;
    return NULL;
  }
  return svr;
}

// creates server task from data stored by server
IDistribCompServerTask *DistribCompProvider2::loadServerTask(IGenLoad &crd)
{
  int four_cc = crd.readInt(), ver = crd.readInt();
  crd.seekrel(-8);

  int idx = find_server(four_cc);
  if (idx == -1)
    return NULL;
  if (server_makers[idx]->getTaskFormatVer() != ver)
    return NULL;

  IDistribCompServerTask *svr;
  svr = server_makers[idx]->make();
  if (!svr->loadTask(crd))
  {
    delete svr;
    return NULL;
  }
  return svr;
}

// creates client task from data received from server
IDistribCompComputingClientTask *DistribCompProvider2::loadClientTask(IGenLoad &crd, const char *persistant_fname,
  const char *home_dir, IGenericProgressIndicator *pbar)
{
  int four_cc = crd.readInt(), ver = crd.readInt();
  crd.seekrel(-8);

  int idx = find_comp_client(four_cc);
  if (idx == -1)
    return NULL;
  if (comp_client_makers[idx]->getTaskFormatVer() != ver)
    return NULL;

  QuietProgressIndicator qpbar;
  if (!pbar)
    pbar = &qpbar;

  IDistribCompComputingClientTask *cli;
  cli = comp_client_makers[idx]->make();
  if (!cli->receiveSourceData(crd, persistant_fname, home_dir, *pbar))
  {
    delete cli;
    return NULL;
  }
  return cli;
}

// enumeration of available submission types and their descriptions
int DistribCompProvider2::getTaskTypeIdCount() { return submit_client_makers.size(); }
int DistribCompProvider2::getTaskTypeId(int idx)
{
  if (idx < 0 || idx >= submit_client_makers.size())
    return 0;
  return submit_client_makers[idx]->getFourCC();
}
const char *DistribCompProvider2::getTaskTypeDescription(int idx)
{
  if (idx < 0 || idx >= submit_client_makers.size())
    return "Invalid task type";
  return submit_client_makers[idx]->description();
}
const char *DistribCompProvider2::getTaskTypeDescriptionByFourCC(int four_cc)
{
  int idx = find_submit_client(four_cc);
  if (idx < 0 || idx >= submit_client_makers.size())
    return "Unknown task type";
  return submit_client_makers[idx]->description();
}
