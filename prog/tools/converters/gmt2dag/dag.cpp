// Copyright (C) Gaijin Games KFT.  All rights reserved.

// dag.cpp : DLL wrapper of the library for DAG export
//
////////////////////////////////////////////////////////////////////////////////

#ifndef WINVER
#define WINVER 0x0500 // Specifies that the minimum required platform is Windows 2000
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0500
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#include <stdlib.h>
#include <malloc.h>

#include "dag.h"

#include <memory/dag_memBase.h>

#pragma warning(push)
#pragma warning(disable : 4244)
#include <libTools/dagFileRW/dagFileExport.h>
#pragma warning(pop)


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
////////////////////////////////////////////////////////////////////////////////

CDAGWriter::CDAGWriter()
{
  try
  {
    mpSaver = new DagSaver;
  }
  catch (...)
  {
    mpSaver = NULL;
  }
}

CDAGWriter::~CDAGWriter()
{
  if (mpSaver)
  {
    delete mpSaver;
  }
}

bool CDAGWriter::IsValid() { return mpSaver != NULL; }


////////////////////////////////////////////////////////////////////////////////
// Wrapper functions
////////////////////////////////////////////////////////////////////////////////

bool CDAGWriter::Start(char *pFilename)
{
  try
  {
    return mpSaver->start_save_dag(pFilename);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::Stop()
{
  try
  {
    return mpSaver->end_save_dag();
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveTextures(int iN, const char **pTexFn)
{
  try
  {
    return mpSaver->save_textures(iN, pTexFn);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveMaterial(CDAGMaterial &Material)
{
  try
  {
    return mpSaver->save_mater(*(DagExpMater *)&Material);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StartNodes()
{
  try
  {
    return mpSaver->start_save_nodes();
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StopNodes()
{
  try
  {
    return mpSaver->end_save_nodes();
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StartNode(char *pName, CDAGMatrix &Matrix, int iFlag, int iChildren)
{
  try
  {
    return mpSaver->start_save_node(pName, *(TMatrix *)&Matrix, iFlag, iChildren);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StopNode()
{
  try
  {
    return mpSaver->end_save_node();
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveNodeMaterials(int iN, unsigned short *pData)
{
  try
  {
    return mpSaver->save_node_mats(iN, pData);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveNodeScript(char *pData)
{
  try
  {
    return mpSaver->save_node_script(pData);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveBigMesh(int iNVertices, float *pVertices, int iNFaces, CDAGBigFace *pFaces, unsigned char NChannels,
  CDAGBigMapChannel *pTexCh, unsigned char *pFaceFlg)
{
  try
  {
    return mpSaver->save_dag_bigmesh(iNVertices, (Point3 *)pVertices, iNFaces, (DagBigFace *)pFaces, NChannels,
      (DagBigMapChannel *)pTexCh, pFaceFlg);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveLight(CDAGLight &Light, CDAGLight2 &Light2)
{
  try
  {
    return mpSaver->save_dag_light(*(DagLight *)&Light, *(DagLight2 *)&Light2);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveSpline(void **pData, int iCount)
{
  try
  {
    return mpSaver->save_dag_spline((DagSpline **)pData, iCount);
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StartNodeRaw(char *pName, int iFlag, int iChildren)
{
  try
  {
    mpSaver->start_save_node_raw(pName, iFlag, iChildren);

    return true;
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::SaveNodeTM(CDAGMatrix &TM)
{
  try
  {
    mpSaver->save_node_tm(*(TMatrix *)&TM);

    return true;
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StartChildren()
{
  try
  {
    mpSaver->start_save_children();

    return true;
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StopChildren()
{
  try
  {
    mpSaver->end_save_children();

    return true;
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}

bool CDAGWriter::StopNodeRaw()
{
  try
  {
    mpSaver->end_save_node_raw();

    return true;
  }
  catch (IGenSave::SaveException &)
  {
    return false;
  }
}
