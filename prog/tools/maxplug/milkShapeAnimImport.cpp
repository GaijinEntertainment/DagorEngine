// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#if defined(MAX_RELEASE_R14) && MAX_RELEASE >= MAX_RELEASE_R14
#include <maxScript/maxScript.h>
#else
#include <maxscrpt/maxscrpt.h>
#endif
#include <locale.h>

#include "dagor.h"
#include "resource.h"

#include "debug.h"

#include <string>

M_STD_STRING strToWide(const char *sz);
std::string wideToStr(const TCHAR *sw);


static TCHAR importFilename[256] = _T("F:\\SETUPS\\Graphics\\MilkShape\\ascii\\Walk_Forward.txt");


bool inputFilename(HWND hpanel)
{
  static TCHAR fname[260];

  _tcsncpy(fname, ::importFilename, 259);
  fname[259] = 0;

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(_T("Milkshape ASCII"));
  fl.Append(_T("*.txt"));
  TSTR title = _T("Import MilkShape Animation");

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hpanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = _T ("txt");
  ofn.lpstrTitle = title;

  if (GetOpenFileName(&ofn))
  {
    _tcscpy(::importFilename, fname);
    return 1;
  }

  return 0;
}


static void tabPrintf(Tab<TCHAR> &tab, const TCHAR *fmt, ...)
{
  TCHAR buf[2048];

  va_list ap;
  va_start(ap, fmt);
  _vstprintf(buf, fmt, ap);
  va_end(ap);

  if (!tab.Count())
    tab.Append(1, (TCHAR *)_T(""));


  tab.Insert(tab.Count() - 1, (int)_tcslen(buf), buf);
}


struct MilkshapeKey
{
  float time;
  Point3 val;

  Quat quatVal()
  {
    Quat q;
    EulerToQuat(Point3(-val.x, -val.y, val.z), q, EULERTYPE_XYZ);
    return q;
  }
};


static inline Quat qadd(const Quat &a, const Quat &b)
{
  Quat q(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
  q.Normalize();
  return q;
}

static inline Quat SLERP(const Quat &a, const Quat &b, float t)
{
  float f = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (f >= 0.9999)
  {
    return qadd(a * (1 - t), b * t);
  }
  else if (f <= -0.9999)
  {
    return qadd(a * (t - 1), b * t);
  }
  else
  {
    float w = acosf(f);
    if (f >= 0)
    {
      float sinw = sinf(w);
      return qadd(a * (sinf(w * (1 - t)) / sinw), b * (sinf(w * t) / sinw));
    }
    w = PI - w;
    float sinw = sinf(w);
    return qadd(a * (-sinf(w * (1 - t)) / sinw), b * (sinf(w * t) / sinw));
  }
}


struct MilkshapeAnimTrack
{
  Tab<MilkshapeKey> keys;

  Point3 interp(float time)
  {
    int i;
    for (i = 0; i < keys.Count(); ++i)
      if (time < keys[i].time)
        break;

    if (i == 0)
      return keys[i].val;
    if (i >= keys.Count())
      return keys[i - 1].val;

    float t = (time - keys[i - 1].time) / (keys[i].time - keys[i - 1].time);
    return keys[i].val * t + keys[i - 1].val * (1 - t);
  }


  Quat interpRot(float time)
  {
    int i;
    for (i = 0; i < keys.Count(); ++i)
      if (time < keys[i].time)
        break;

    if (i == 0)
      return keys[i].quatVal();
    if (i >= keys.Count())
      return keys[i - 1].quatVal();

    Quat q1 = keys[i - 1].quatVal();
    Quat q2 = keys[i].quatVal();

    float t = (time - keys[i - 1].time) / (keys[i].time - keys[i - 1].time);
    return SLERP(q1, q2, t);
  }
};


struct MilkshapeNodeAnim
{
  TSTR name;
  MilkshapeAnimTrack pos, rot;
  Point3 lpos, lrot;
};


void import_milkshape_anim(Interface *ip, HWND hpanel)
{
  if (!::inputFilename(hpanel))
    return;

  FILE *fh = _tfopen(::importFilename, _T("rt"));
  if (!fh)
  {
    TSTR msg;
    msg.printf(_T("Can't open file %s"), ::importFilename);
    MessageBox(hpanel, msg, _T("Milkshape animation import"), MB_OK);
    return;
  }

  setlocale(LC_NUMERIC, "C");

  const int LINELEN = 1024;
  char str[LINELEN];

  int numBones = 0;

  Tab<TCHAR> missingNodes;

  // skip to bones
  for (; fgets(str, LINELEN, fh);)
    if (sscanf(str, "Bones: %d", &numBones) == 1)
      break;

  char name[256] = "", parentName[256];

  int errLine = 0;

  MilkshapeNodeAnim *bones = new MilkshapeNodeAnim[numBones];

  float minTime = 1e30f, maxTime = -1e30f;

  int boneInd;
  for (boneInd = 0; boneInd < numBones; ++boneInd)
  {
    MilkshapeNodeAnim &anim = bones[boneInd];

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (sscanf(str, "\"%[^\"]\"", name) != 1)
      break;

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (strncmp(str, "\"\"", 2) == 0)
      strcpy(parentName, "");
    else if (sscanf(str, "\"%[^\"]\"", parentName) != 1)
      break;

    anim.name = strToWide(name).c_str();

    int flags;
    Point3 lpos, lrot;

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (sscanf(str, "%d %f %f %f %f %f %f", &flags, &lpos.x, &lpos.y, &lpos.z, &lrot.x, &lrot.y, &lrot.z) != 7)
      break;

    anim.lpos = lpos;
    anim.lrot = lrot;

    int num = 0;

    // pos keys
    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (sscanf(str, "%d", &num) != 1)
      break;

    Tab<MilkshapeKey> &posKeys = anim.pos.keys;
    posKeys.SetCount(num);

    int i;
    for (i = 0; i < posKeys.Count(); ++i)
    {
      MilkshapeKey &k = posKeys[i];

      errLine = __LINE__;
      if (!fgets(str, LINELEN, fh))
        break;
      if (sscanf(str, "%f %f %f %f", &k.time, &k.val.x, &k.val.y, &k.val.z) != 4)
        break;

      if (k.time < minTime)
        minTime = k.time;
      if (k.time > maxTime)
        maxTime = k.time;
    }
    if (i < posKeys.Count())
      break;

    // rot keys
    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (sscanf(str, "%d", &num) != 1)
      break;

    Tab<MilkshapeKey> &rotKeys = anim.rot.keys;
    rotKeys.SetCount(num);

    for (i = 0; i < rotKeys.Count(); ++i)
    {
      MilkshapeKey &k = rotKeys[i];

      errLine = __LINE__;
      if (!fgets(str, LINELEN, fh))
        break;
      if (sscanf(str, "%f %f %f %f", &k.time, &k.val.x, &k.val.y, &k.val.z) != 4)
        break;

      if (k.time < minTime)
        minTime = k.time;
      if (k.time > maxTime)
        maxTime = k.time;
    }
    if (i < rotKeys.Count())
      break;

    // get node and set keys

    INode *node = ip->GetINodeByName(anim.name);
    if (!node)
    {
      TSTR s;
      s.printf(_T("\"%s\", "), anim.name);
      missingNodes.Append(s.Length(), (TCHAR *)s.data());
      continue;
    }
  }

  // handle read error
  if (boneInd < numBones)
  {
    TSTR msg;

    msg.printf(_T("Error reading bone #%d (%s), line %d"), boneInd + 1, strToWide(name).c_str(), errLine);
    MessageBox(hpanel, msg, _T("Milkshape animation import"), MB_OK);
  }
  else if (missingNodes.Count())
  {
    missingNodes.Append(1, (TCHAR *)_T(""));
    TSTR msg;
    msg = _T("Missing bones:\n");
    msg += &missingNodes[0];
    MessageBox(hpanel, msg, _T("Milkshape animation import"), MB_OK);
  }
  else
  {
    MessageBox(hpanel, _T("Imported successfully"), _T("Milkshape animation import"), MB_OK);
  }

  fclose(fh);


  // create keys
  ip->DisableSceneRedraw();

  for (float keyTime = minTime; keyTime <= maxTime; keyTime += 1)
  {
    for (boneInd = 0; boneInd < numBones; ++boneInd)
    {
      MilkshapeNodeAnim &anim = bones[boneInd];
      const TCHAR *name = anim.name;

      // get node and set keys
      INode *node = ip->GetINodeByName(name);
      if (!node)
        continue;

      TimeValue time(GetTicksPerFrame() * keyTime);

      Tab<TCHAR> s;

      // set time slider
      tabPrintf(s, _T("sliderTime=%ff\n"), keyTime);

      // set pos keys
      // if (0)
      // if (stricmp(name, "Bip01")==0)
      if (_tcsicmp(name, _T("Bip01")) == 0 || _tcsstr(name, _T("Pelvis")))
      {
        Matrix3 baseTm;

        Point3 r = anim.lrot;
        EulerToMatrix(Point3(-r.x, -r.y, r.z), baseTm, EULERTYPE_XYZ);

        baseTm.SetTrans(anim.lpos);

        Matrix3 ptm = node->GetParentNode()->GetNodeTM(time);

        if (node->GetParentNode()->IsRootNode())
        {
          Point3 a = ptm.GetRow(1);
          ptm.SetRow(1, ptm.GetRow(2));
          ptm.SetRow(2, a);
          ptm.SetRow(0, -ptm.GetRow(0));
        }

        // Matrix3 ntm=node->GetNodeTM(time);

        Matrix3 tm = baseTm * ptm;

        Point3 pk = anim.pos.interp(keyTime);
        Point3 p = tm.PointTransform(Point3(-pk.x, pk.y, -pk.z));

        if (_tcsstr(name, _T("Pelvis")))
        {
          tabPrintf(s, _T("biped.setTransform $'Bip01' #pos [%g, %g, %g] true\n"), p.x, p.y, p.z);
        }
        else
        {
          tabPrintf(s, _T("biped.setTransform $'%s' #pos [%g, %g, %g] true\n"), name, p.x, p.y, p.z);
        }
      }

      // set rot keys
      // if (stricmp(name, "Bip01 Spine")==0 || stricmp(name, "Bip01 Spine1")==0)
      // if (stricmp(name, "Bip01")==0 || stricmp(name, "Bip01 PelvisX")==0)
      {
        Matrix3 baseTm;

        Point3 r = anim.lrot;
        EulerToMatrix(Point3(-r.x, -r.y, r.z), baseTm, EULERTYPE_XYZ);

        Matrix3 ptm = node->GetParentNode()->GetNodeTM(time);

        if (node->GetParentNode()->IsRootNode())
        {
          Point3 a = ptm.GetRow(1);
          ptm.SetRow(1, ptm.GetRow(2));
          ptm.SetRow(2, a);
          ptm.SetRow(0, -ptm.GetRow(0));
        }

        Quat q;
        q = anim.rot.interpRot(keyTime);
        Matrix3 rtm;
        q.MakeMatrix(rtm, TRUE);

        Matrix3 tm = rtm * baseTm * ptm;
        // Matrix3 tm=baseTm*ptm;
        q = Quat(tm);

        tabPrintf(s, _T("biped.setTransform $'%s' #rotation (quat %g %g %g %g) true\n"), name, q.x, q.y, q.z, q.w);
      }

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
      ExecuteMAXScriptScript(&s[0], MAXScript::ScriptSource::NotSpecified);
#else
      ExecuteMAXScriptScript(&s[0]);
#endif
    }
  }

  ip->EnableSceneRedraw();
  ip->RedrawViews(ip->GetTime());


  delete[] bones;

  setlocale(LC_NUMERIC, "");
}
