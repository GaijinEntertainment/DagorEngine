// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// forward declarations for external classes
class IGenLoad;
class IGenSave;
class DataBlock;
class IGenericProgressIndicator;
class TMatrix;
class BBox3;


// interface for storing unit data (with const unit size)
class IDistribCompUnitDataStore
{
public:
  virtual int getUnitSize() const = 0;
  virtual void storeUnitData(int unit_number, const void *data) = 0;
};

// interface for accessing unit data (with const unit size)
class IDistribCompUnitDataAccess
{
public:
  virtual int getUnitSize() const = 0;
  virtual const void *getUnitData(int unit_number) = 0;
};

// struct contains data for one version of computed units
struct DistribCompQualityTestSample
{
  void *unitStorage; //< points to memory containing computed units
  void *userData;    //< arbirtary use data handle
  bool dataValid;    //< OUT: contains results of comparison test
};


// Submitting client interface
class IDistribCompSubmittingClientTask
{
public:
  virtual ~IDistribCompSubmittingClientTask() {}

  // creates new task and writes it to cwr to transmit to server
  virtual bool writeSourceData(const DataBlock &task_blk, IGenSave &cwr) = 0;
};

// Computing client interface
class IDistribCompComputingClientTask
{
public:
  virtual ~IDistribCompComputingClientTask() {}

  // receives sourceData for task (called by provider on loadTask)
  virtual bool receiveSourceData(IGenLoad &crd, const char *persistent_fname, const char *home_dir,
    IGenericProgressIndicator &pbar) = 0;
  // receives passData for task (called by net)
  virtual void receivePassData(IGenLoad &crd, IGenericProgressIndicator &pbar) = 0;

  // prepares to computing
  virtual void beginComputing() = 0;

  // Compute units and store their data.
  virtual void computeUnits(int start_unit, int unit_num, IGenLoad &crd, IDistribCompUnitDataStore &uds,
    IGenericProgressIndicator &pbar) = 0;

  // Render task from specified point; useful for debug (Inspect GI mode)
  virtual void renderTask(const TMatrix &vtm) = 0;

  // finishing computing (restore state and deallocate resources)
  virtual void endComputing() = 0;

  // deallocates resources used for pass
  virtual void releasePassData() = 0;

  // information needed to provide storage
  virtual int getCompUnitSize() const = 0;

  // information on various geom boxes that set for current pass
  virtual const BBox3 *getComputeBox() const = 0;
  virtual const BBox3 *getShadowBox() const = 0;
  virtual const BBox3 *getLightBox() const = 0;

#define DISTRIBCOMP_CC_DEF_GET_BOX()                                                                    \
  virtual const BBox3 *getComputeBox() const                                                            \
  {                                                                                                     \
    return sceneGeom.settings.opt.whereCompute.isempty() ? NULL : &sceneGeom.settings.opt.whereCompute; \
  }                                                                                                     \
  virtual const BBox3 *getShadowBox() const                                                             \
  {                                                                                                     \
    return sceneGeom.pass.shadowBox.isempty() ? NULL : &sceneGeom.pass.shadowBox;                       \
  }                                                                                                     \
  virtual const BBox3 *getLightBox() const                                                              \
  {                                                                                                     \
    return sceneGeom.pass.lightBox.isempty() ? NULL : &sceneGeom.pass.lightBox;                         \
  }
};

// Server interface
class IDistribCompServerTask
{
public:
  virtual ~IDistribCompServerTask() {}

  // receives sourceData for task (called by provider on receiveTask)
  virtual bool receiveSourceData(IGenLoad &crd) = 0;

  // prepares task for processing and saves results (to be loaded in loadTask)
  virtual bool prepareAndSaveTask(IGenSave &cwr, const char *homedir) = 0;

  // loads previously saved task (called by provider on loadTask)
  virtual bool loadTask(IGenLoad &crd) = 0;

  // writes pass data to cwr to transmit to computing clients
  virtual void startPass(int pass_no, IGenSave &cwr) = 0;
  // finishes current pass (performs data processing and results writing)
  virtual void finishPass(int pass_no, IDistribCompUnitDataAccess &uda) = 0;

  // writes source data to cwr to transmit to computing clients
  virtual bool writeSourceData(IGenSave &cwr) = 0;
  // writes data for computing to cwr to transmit to computing clients
  virtual void writeComputeData(int start_unit, int num_unit, IGenSave &cwr) = 0;

  // return whether qualityTest() can be called
  virtual bool canDoQualityTest() = 0;

  // compares several version of the same units and returns proper result index
  // marks invalid versions (can be used to ban invalid clients)
  virtual int qualityTest(DistribCompQualityTestSample *qts, int qts_num, int unit_num, IGenSave *log_cwr) = 0;

  // information needed to provide storage
  virtual int getCompUnitCount() const = 0;
  virtual int getCompUnitSize() const = 0;

  // information on passes
  virtual int getCompPassesCount() const = 0;

  // hint for units distribution (return 0 if no hint available)
  virtual int defaultStartingUnitCountForClient() const = 0;
};

//
// makers for all distributed computation components
//
class IDistribCompSubmittingClientMaker
{
public:
  // identification for tasks, submitted by this client
  virtual int getFourCC() = 0;
  virtual int getTaskFormatVer() = 0;

  // task maker
  virtual IDistribCompSubmittingClientTask *make() = 0;

  // description (must be user friendly)
  virtual const char *description() = 0;

  // unregister (can be used for self-destruction)
  virtual void unregister() = 0;
};

class IDistribCompServerMaker
{
public:
  // identification for tasks, processed by this server
  virtual int getFourCC() = 0;
  virtual int getTaskFormatVer() = 0;

  virtual IDistribCompServerTask *make() = 0;

  // unregister (can be used for self-destruction)
  virtual void unregister() = 0;
};

class IDistribCompComputingClientMaker
{
public:
  // identification for tasks, processed by this client
  virtual int getFourCC() = 0;
  virtual int getTaskFormatVer() = 0;

  // task maker
  virtual IDistribCompComputingClientTask *make() = 0;

  // unregister (can be used for self-destruction)
  virtual void unregister() = 0;
};


namespace DistribCompProvider2
{
// components registration
bool registerMaker(IDistribCompSubmittingClientMaker *maker);
bool registerMaker(IDistribCompComputingClientMaker *maker);
bool registerMaker(IDistribCompServerMaker *maker);

// components deletion
bool unregisterMaker(int four_cc, bool subm_cli, bool comp_cli, bool server);
void unregisterAll();


// creates client task for data submission
IDistribCompSubmittingClientTask *newTask(int task_fourcc);

// creates server task from data submitted by client
IDistribCompServerTask *receiveServerTask(IGenLoad &crd);

// creates server task from data stored by server
IDistribCompServerTask *loadServerTask(IGenLoad &crd);

// creates client task from data received from server
// NOTE: 'persistant_fname' is either source file for 'crd', or NULL when no persistent file
//       associated with 'crd'
IDistribCompComputingClientTask *loadClientTask(IGenLoad &crd, const char *persistent_fname, const char *home_dir,
  IGenericProgressIndicator *pbar);

// enumeration of available submission types and their descriptions
int getTaskTypeIdCount();
int getTaskTypeId(int idx);
const char *getTaskTypeDescription(int idx);
const char *getTaskTypeDescriptionByFourCC(int four_cc);
} // namespace DistribCompProvider2
