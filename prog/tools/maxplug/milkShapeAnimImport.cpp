// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <maxScript/maxScript.h>
#include <format>

#include "dagor.h"
#include "resource.h"

#include "debug.h"
#include "common.h"

#include <string>
#include <vector>


static TCHAR importFilename[256] = _T("F:\\SETUPS\\Graphics\\MilkShape\\ascii\\Walk_Forward.txt");


bool inputFilename(HWND hpanel)
{
  TCHAR fname[260] = {};

  _tcsncpy_s(fname, _countof(fname), ::importFilename, _TRUNCATE);

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
    _tcsncpy_s(::importFilename, _countof(::importFilename), fname, _TRUNCATE);
    return 1;
  }

  return 0;
}


static void tabAppend(Tab<wchar_t> &tab, const std::wstring &text)
{
  if (!tab.Count())
    tab.Append(1, (TCHAR *)_T(""));

  tab.Insert(tab.Count() - 1, (int)text.length(), (wchar_t *)text.c_str());
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
    if (!keys.Count())
      return Point3(0, 0, 0);

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
    if (!keys.Count())
      return Quat(0.f, 0.f, 0.f, 1.f);

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

  const int LINELEN = 1024;
  char str[LINELEN];

  int numBones = -1;

  Tab<TCHAR> missingNodes;

  // skip to bones
  for (; fgets(str, LINELEN, fh);)
    if (sscanf(str, "Bones: %d", &numBones) == 1)
      break;

  if (numBones < 0)
  {
    fclose(fh);
    MessageBox(hpanel, L"The file has no valid \"Bones:\" count", L"Milkshape animation import", MB_OK);
    return;
  }

  char name[256] = "";

  int errLine = 0;

  std::vector<MilkshapeNodeAnim> bones;

  float minTime = 1e30f, maxTime = -1e30f;

  int boneInd;
  for (boneInd = 0; boneInd < numBones; ++boneInd)
  {
    MilkshapeNodeAnim &anim = bones.emplace_back();

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (sscanf(str, "\"%255[^\"]\"", name) != 1)
      break;

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;

    anim.name = strToWide(name).c_str();

    int flags;
    Point3 lpos, lrot;

    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (parse_spaced_nums(str, flags, lpos.x, lpos.y, lpos.z, lrot.x, lrot.y, lrot.z) != 7)
      break;

    anim.lpos = lpos;
    anim.lrot = lrot;

    int num = 0;

    // pos keys
    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (parse_spaced_nums(str, num) != 1 || num < 0)
      break;

    Tab<MilkshapeKey> &posKeys = anim.pos.keys;

    int i;
    for (i = 0; i < num; ++i)
    {
      MilkshapeKey k;

      errLine = __LINE__;
      if (!fgets(str, LINELEN, fh))
        break;
      if (parse_spaced_nums(str, k.time, k.val.x, k.val.y, k.val.z) != 4)
        break;

      if (k.time < minTime)
        minTime = k.time;
      if (k.time > maxTime)
        maxTime = k.time;
      posKeys.Append(1, &k, 32);
    }
    if (i < num)
      break;

    // rot keys
    errLine = __LINE__;
    if (!fgets(str, LINELEN, fh))
      break;
    if (parse_spaced_nums(str, num) != 1 || num < 0)
      break;

    Tab<MilkshapeKey> &rotKeys = anim.rot.keys;

    for (i = 0; i < num; ++i)
    {
      MilkshapeKey k;

      errLine = __LINE__;
      if (!fgets(str, LINELEN, fh))
        break;
      if (parse_spaced_nums(str, k.time, k.val.x, k.val.y, k.val.z) != 4)
        break;

      if (k.time < minTime)
        minTime = k.time;
      if (k.time > maxTime)
        maxTime = k.time;
      rotKeys.Append(1, &k, 32);
    }
    if (i < num)
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

  fclose(fh);

  // handle read error
  if (boneInd < numBones)
  {
    TSTR msg;

    msg.printf(_T("Error reading bone #%d (%s), line %d"), boneInd + 1, strToWide(name).c_str(), errLine);
    MessageBox(hpanel, msg, _T("Milkshape animation import"), MB_OK);
    return;
  }

  if (missingNodes.Count())
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


  // create keys
  ip->DisableSceneRedraw();

  for (float keyTime = minTime; keyTime <= maxTime; keyTime += 1)
  {
    for (MilkshapeNodeAnim &anim : bones)
    {
      const TCHAR *name = anim.name;

      // get node and set keys
      INode *node = ip->GetINodeByName(name);
      if (!node)
        continue;

      TimeValue time(GetTicksPerFrame() * keyTime);

      Tab<TCHAR> s;

      // set time slider
      tabAppend(s, std::format(L"sliderTime={:f}f\n", keyTime));

      // set pos keys
      if (iequal(name, L"Bip01") || wcsstr(name, L"Pelvis"))
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

        Matrix3 tm = baseTm * ptm;

        Point3 pk = anim.pos.interp(keyTime);
        Point3 p = tm.PointTransform(Point3(-pk.x, pk.y, -pk.z));

        if (_tcsstr(name, _T("Pelvis")))
        {
          tabAppend(s, std::format(L"biped.setTransform $'Bip01' #pos [{:g}, {:g}, {:g}] true\n", p.x, p.y, p.z));
        }
        else
        {
          tabAppend(s, std::format(L"biped.setTransform $'{}' #pos [{:g}, {:g}, {:g}] true\n", name, p.x, p.y, p.z));
        }
      }

      // set rot keys
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
        q = Quat(tm);

        tabAppend(s, std::format(L"biped.setTransform $'{}' #rotation (quat {:g} {:g} {:g} {:g}) true\n", name, q.x, q.y, q.z, q.w));
      }

      ExecuteMAXScriptScript(&s[0], MAXScript::ScriptSource::NotSpecified);
    }
  }

  ip->EnableSceneRedraw();
  ip->RedrawViews(ip->GetTime());
}
