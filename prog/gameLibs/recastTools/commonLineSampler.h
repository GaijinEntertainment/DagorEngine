// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <recastTools/recastBuildCovers.h>

#include <math/dag_mathUtils.h>

namespace recastbuild
{
#define MAX_COUNT_EXTPT_LINES 4
enum SamplePointState : unsigned char
{
  NAVMESH_CHECKED = 1,
  HEIGHT_CHECKED = 4
};

struct LineSampler
{
  struct LinePoint
  {
    int pos;
    float height = -1.f;

    LinePoint() : pos(0.f), height(-1.f){};
    LinePoint(int p, float h) : pos(p), height(h){};


    float getVect(LinePoint pt)
    {
      float dH = height - pt.height;
      int dP = pos - pt.pos;
      return safediv(dH, float(dP));
    }
  };

  struct Line
  {
    LinePoint start;
    LinePoint end;
    float vect;
    bool is = true;
    int ctPoints = 0;

    float getRelPos(int pos) const { return float(pos - start.pos) / float(end.pos - start.pos); }

    float getVectToPoint(LinePoint pt) const
    {
      float u = getRelPos(pt.pos);
      float origH = lerp(start.height, end.height, u);
      return pt.height - origH;
    }

    Line(LinePoint s, LinePoint e) : start(s), end(e), vect(s.getVect(e)) {}
  };

  struct SamplePoint
  {
    float height;

  private:
    unsigned char flag;

  public:
    bool haveBit(SamplePointState bit) { return bool(flag & bit); }
    bool exist() { return haveBit(HEIGHT_CHECKED); }
    void AddBit(SamplePointState bit) { flag |= bit; }
    void RemoveBit(SamplePointState bit) { flag &= ~bit; }
    void ClearBitState() { flag = 0; }

    struct LineDheight
    {
      int line;
      float dheight;
    };
    LineDheight lines[MAX_COUNT_EXTPT_LINES];
    float minADheight = FLT_MAX;
    int ctLines = 0;

    SamplePoint(float h) : height(h), flag(0), ctLines(0), minADheight(FLT_MAX) {} //-V730

    inline void clearLines(Tab<Line> &lines_arr)
    {
      for (int i = 0; i < ctLines; i++)
        --lines_arr[lines[i].line].ctPoints;
      ctLines = 0;
      minADheight = FLT_MAX;
    }

    void addLine(int pos, Tab<Line> &lines_arr, int nom)
    {
      float dh = lines_arr[nom].getVectToPoint(LinePoint(pos, height));
      float adh = abs(dh);
      float offset = 0.01f;

      if (adh < minADheight) // new champion, check all others
      {
        minADheight = adh;
        int newct = 1;
        LineDheight newLines[MAX_COUNT_EXTPT_LINES];
        newLines[0] = LineDheight({nom, dh});

        for (int i = 0; i < ctLines; i++)
          if (abs(lines[i].dheight) < minADheight + offset && newct < MAX_COUNT_EXTPT_LINES && lines[i].line != nom)
            newLines[newct++] = lines[i];

        for (int i = 0; i < newct; i++)
          lines[i] = newLines[i];
        ctLines = newct;
        return;
      }

      if (adh < minADheight + offset && ctLines < MAX_COUNT_EXTPT_LINES) // enter into offset
      {
        for (int i = 0; i < ctLines; i++)
        {
          if (lines[i].line == nom)
          {
            lines[i].dheight = dh;
            return;
          }
        }
        lines[ctLines++] = LineDheight({nom, dh});
      }
    }

    void pushLine(int pos, Tab<Line> &lines_arr, int nom)
    {
      if (ctLines < MAX_COUNT_EXTPT_LINES)
      {
        float dh = lines_arr[nom].getVectToPoint(LinePoint(pos, height));
        lines[ctLines++] = LineDheight({nom, dh});
      }
    }

    bool isBelongLine(int line)
    {
      for (int i = 0; i < ctLines; i++)
        if (lines[i].line == line)
          return true;
      return false;
    }

    void renameLine(int old, int newName)
    {
      for (int i = 0; i < ctLines; i++)
      {
        if (lines[i].line == old)
        {
          lines[i].line = newName;
          return;
        }
      }
    }
  };

  Edge edge;
  Tab<SamplePoint> points;

  LineSampler() {}

  inline LineSampler(const Edge &ed, const int nsamples) : edge(ed)
  {
    points = Tab<SamplePoint>(tmpmem);
    points.reserve(nsamples);
    for (int i = 0; i < nsamples; i++)
      points.push_back(SamplePoint(-1.f));
  }

  inline void checkGround(const rcCompactHeightfield *m_chf, float range, float groundRange)
  {
    for (int i = 0; i < points.size(); i++)
    {
      const float u = (float)i / (float)(int(points.size()) - 1);
      Point3 pt = lerp(edge.sp, edge.sq, u);
      points[i].ClearBitState();
      if (recastcoll::get_compact_heightfield_height(m_chf, pt, range, groundRange, points[i].height))
      {
        if (abs(points[i].height - pt.y) < groundRange)
          points[i].AddBit(NAVMESH_CHECKED);
        else
          points[i].height = pt.y;
      }
    }
  }


private: // findLines block
  bool isLinesNear(Line &l1, Line &l2, float dhVect, float dH)
  {
    if (abs(l1.vect - l2.vect) > dhVect)
      return false;
    float dist1 = l1.getVectToPoint(l2.start);
    float dist2 = l1.getVectToPoint(l2.end);
    float dist3 = l2.getVectToPoint(l1.start);
    float dist4 = l2.getVectToPoint(l1.start);
    float dist = min(min(dist1, dist2), min(dist3, dist4));
    return dist < dH;
    return true;
  }

  LinePoint updatePointsPossess(Tab<Line> &lines, int nom)
  {
    LinePoint max = LinePoint(-1, -1.f);
    for (int i = 0; i < points.size(); i++)
    {
      if (!points[i].exist())
        continue;
      points[i].addLine(i, lines, nom);
      if (points[i].minADheight > max.height)
      {
        max.height = points[i].minADheight;
        max.pos = i;
      }
    }
    return max;
  }

  void updateAllPointsPossess(Tab<Line> &lines)
  {
    for (auto &point : points)
      point.clearLines(lines);

    for (int i = 0; i < lines.size(); i++)
      if (lines[i].is)
        updatePointsPossess(lines, i);
  }

  int GetNearestExistingPoint(int nom)
  {
    for (int i = nom - 1; i > -1; i--)
      if (points[i].exist())
        return i;
    for (int i = nom + 1; i < points.size(); i++)
      if (points[i].exist())
        return i;
    return -1;
  }

  Line buildLine(int line1, int line2)
  {
    float svect = 0.f;
    float sheight = 0.f;
    int spos = 0;

    int minPos = 0, maxPos = 0;
    int it = 0;

    int prev = -1;
    for (int i = 0; i < points.size(); i++)
    {
      if (!points[i].exist())
        continue;
      if (points[i].isBelongLine(line1) || points[i].isBelongLine(line2))
      {
        LinePoint point = LinePoint(i, points[i].height);
        if (prev == -1)
        {
          sheight += point.height;
          spos += point.pos;
          minPos = point.pos;
          maxPos = point.pos;
        }
        else
        {
          sheight += point.height;
          spos += point.pos;
          svect += LinePoint(prev, points[prev].height).getVect(point);
          minPos = min(minPos, point.pos);
          maxPos = max(maxPos, point.pos);
        }
        prev = i;
        it++;
      }
    }

    if (it < 2)
    {
      Line ret(LinePoint(0, 0.f), LinePoint(0, 0.f));
      ret.is = false;
      return ret;
    }

    float vect = svect / (it - 1);
    float height = sheight / it;
    float pos = float(spos) / float(it);

    float maxh = ((float(maxPos) - pos) * vect) + height;
    float minh = ((float(minPos) - pos) * vect) + height;

    return Line(LinePoint(minPos, minh), LinePoint(maxPos, maxh));
  }

  bool clearShortLines(Tab<Line> &lines, int minSize)
  {
    bool isClear = false;
    minSize++;

    for (int i = 0; i < lines.size(); i++)
      lines[i].ctPoints = 0;

    for (auto &point : points)
      for (int i = 0; i < point.ctLines; i++)
        lines[point.lines[i].line].ctPoints++;

    for (int i = 0; i < lines.size(); i++)
    {
      if (lines[i].ctPoints < minSize)
      {
        lines[i].is = false;
        isClear = true;
      }
    }
    return isClear;
  }

  bool tryRepairHole(int start, int end)
  {
    if (end - start > 2 || !points[start + 1].exist())
      return false;

    float expectedHeight = max(points[start].height, points[end].height);
    return points[start + 1].height < expectedHeight + 0.01f;
  }

  void checkLineContinuity(Tab<Line> &lines, int line)
  {
    int prev = -2;
    int newLine = -1;
    for (int i = 0; i < points.size(); i++)
    {
      if (!points[i].exist() || !points[i].isBelongLine(line))
        continue;

      if (prev == -2 || (prev == i - 1) || tryRepairHole(prev, i)) // continue line
      {
        prev = i;
        if (newLine != -1)
          points[i].renameLine(line, newLine);
        continue;
      }

      // start new line
      newLine = (int)lines.size();
      points[i].renameLine(line, newLine);
      lines.push_back(Line(lines[line].start, lines[line].end));
      prev = i;
    }
  }

  void expanseAndUpdateLine(Tab<Line> &lines, int lineNom)
  {
    const Line &line = lines[lineNom];
    int last = line.end.pos;
    for (int i = line.end.pos + 1; i < points.size(); i++)
    {
      if (!points[i].exist())
        continue;
      if (abs(line.getVectToPoint(LinePoint(i, points[i].height))) > 0.3f)
        continue;

      if (last != i - 1 && !tryRepairHole(last, i))
        break;

      points[i].addLine(i, lines, lineNom);
      last = i;
    }

    last = line.start.pos;
    for (int i = line.start.pos - 1; i > -1; i--)
    {
      if (!points[i].exist())
        continue;
      if (abs(line.getVectToPoint(LinePoint(i, points[i].height))) > 0.3f)
        continue;

      if (last != i + 1 && !tryRepairHole(i, last))
        break;

      points[i].addLine(i, lines, lineNom);
      last = i;
    }

    int end = min<int>(points.size(), line.end.pos + 1);
    for (int i = line.start.pos; i < end; i++)
      points[i].addLine(i, lines, lineNom);
  }

  void cutCrossLines(Line &line1, Line &line2)
  {
    int len1 = line1.end.pos - line1.start.pos;
    int len2 = line2.end.pos - line2.start.pos;

    if (len1 < len2)
    {
      if (line1.start.pos < line2.start.pos)
      {
        float h = lerp(line1.start.height, line1.end.height, line1.getRelPos(line2.start.pos));
        line1.end = LinePoint(line2.start.pos, h);
      }
      else
      {
        float h = lerp(line1.start.height, line1.end.height, line1.getRelPos(line2.end.pos));
        line1.start = LinePoint(line2.end.pos, h);
      }
    }
    else
    {
      if (line1.start.pos < line2.start.pos)
      {
        float h = lerp(line2.start.height, line2.end.height, line2.getRelPos(line1.start.pos));
        line2.end = LinePoint(line1.start.pos, h);
      }
      else
      {
        float h = lerp(line2.start.height, line2.end.height, line2.getRelPos(line1.end.pos));
        line2.start = LinePoint(line1.end.pos, h);
      }
    }
  }

  bool overlapRange(int amin, int amax, int bmin, int bmax) { return amin < bmin ? bmin < amax : amin < bmax; }

  bool checkCrossLines(Tab<Line> &lines)
  {
    bool isCut = false;
    for (int i = 0; i < lines.size(); i++)
    {
      if (lines[i].is)
      {
        for (int j = i + 1; j < lines.size(); j++)
        {
          if (lines[j].is && overlapRange(lines[i].start.pos, lines[i].end.pos, lines[j].start.pos, lines[j].end.pos))
          {
            cutCrossLines(lines[i], lines[j]);
            isCut = true;
          }
        }
      }
    }
    return isCut;
  }

  void markPoints(Tab<Line> &lines)
  {
    for (int i = 0; i < points.size(); i++)
      points[i].clearLines(lines);

    for (int i = 0; i < lines.size(); i++)
      if (lines[i].is)
        for (int j = 0; j < points.size(); j++)
          if (points[j].exist() && j >= lines[i].start.pos && j <= lines[i].end.pos)
            points[j].pushLine(j, lines, i);
  }

public:
  void findJumpLinkLines(int minWeight, float maxDelta, Tab<Edge> &ret)
  {
    if (!points.size())
      return;
    Tab<Line> lines(tmpmem);
    lines.reserve(min(10u, points.size() / 2));
    LinePoint maxADH = LinePoint(GetNearestExistingPoint(0), 1.f);

    while (maxADH.height > 0.1f)
    {
      int p1 = maxADH.pos;
      int p2 = GetNearestExistingPoint(p1);
      if (p1 == -1 || p2 == -1)
        return;

      int start = min(p1, p2);
      int end = max(p1, p2);
      lines.push_back(Line(LinePoint(start, points[start].height), LinePoint(end, points[end].height)));
      if (lines.size() > points.size())
      {
        logerr("detected infinite loop in 'findJumpLinkLines'");
        return;
      }
      maxADH = updatePointsPossess(lines, (int)lines.size() - 1);
    }

    { // merge common lines
      bool isMerge = false;
      for (int i = 0; i < lines.size(); i++)
      {
        if (!lines[i].is)
          continue;
        for (int j = i + 1; j < lines.size(); j++)
        {
          if (!lines[j].is)
            continue;
          if (isLinesNear(lines[i], lines[j], maxDelta, maxDelta))
          {
            lines[i] = buildLine(i, j);
            lines[j].is = false;
            isMerge = true;
          }
        }
      }

      if (isMerge)
        updateAllPointsPossess(lines);
    }

    if (clearShortLines(lines, 2))
      updateAllPointsPossess(lines);

    // clear far points
    for (auto &point : points)
      if (point.ctLines && point.minADheight > maxDelta)
        point.clearLines(lines);

    // rebuild clear lines
    for (int i = 0; i < lines.size(); i++)
      if (lines[i].is)
        lines[i] = buildLine(i, -1);

    ////check continuity
    {
      int startSize = (int)lines.size();
      for (int i = 0; i < startSize; i++)
        if (lines[i].is)
          checkLineContinuity(lines, i);

      clearShortLines(lines, 2);

      // rebuild continuity lines
      for (int i = 0; i < lines.size(); i++)
        if (lines[i].is)
          lines[i] = buildLine(i, -1);
    }

    { // try found better lines for points
      for (auto &point : points)
        point.clearLines(lines);

      for (int i = 0; i < lines.size(); i++)
        if (lines[i].is)
          expanseAndUpdateLine(lines, i);

      for (int i = 0; i < lines.size(); i++)
        if (lines[i].is)
          lines[i] = buildLine(i, -1);

      if (checkCrossLines(lines))
      {
        markPoints(lines);
        for (int i = 0; i < lines.size(); i++)
          if (lines[i].is)
            lines[i] = buildLine(i, -1);
      }

      clearShortLines(lines, 2);
    }

    for (int i = 0; i < lines.size(); i++)
    {
      if (lines[i].is && lines[i].end.pos - lines[i].start.pos >= minWeight)
      {
        const float u0 = (float)lines[i].start.pos / (float)((int)points.size() - 1);
        const float u1 = (float)lines[i].end.pos / (float)((int)points.size() - 1);
        Point3 sp = lerp(edge.sp, edge.sq, u0);
        Point3 sq = lerp(edge.sp, edge.sq, u1);
        sp.y = lines[i].start.height;
        sq.y = lines[i].end.height;
        ret.push_back(Edge({sp, sq}));
      }
    }
  }

  void findCoverLines(int minWeight, const CoversParams &covParams, Tab<Edge> &ret)
  {
    if (!points.size())
      return;
    float mergeDistSq = sqr(covParams.mergeDist);
    float maxDelta = covParams.deltaHeightThreshold;

    int prevCovIdx = -1;
    int start = 0;
    float prevH = points[0].height;
    float minH = FLT_MAX;
    for (int i = 1; i < points.size(); i++)
    {
      bool isEnd = i == (int)points.size() - 1;
      if (abs(points[i].height - prevH) > maxDelta || isEnd)
      {
        float groundH = lerp(edge.sp.y, edge.sq.y, (float)i / (float)((int)points.size() - 1));
        if (minH - groundH >= covParams.minHeight)
        {
          int end = isEnd ? i : i - 1;
          Point3 sp = lerp(edge.sp, edge.sq, (float)start / (float)((int)points.size() - 1));
          Point3 sq = lerp(edge.sp, edge.sq, (float)end / (float)((int)points.size() - 1));

          if (prevCovIdx > -1 && lengthSq(Point3::x0z(ret[prevCovIdx].sq - sp)) < mergeDistSq &&
              (abs(ret[prevCovIdx].sq.y - minH) <= maxDelta || ((end - start) < minWeight && minH > ret[prevCovIdx].sq.y)))
            ret[prevCovIdx].sq = Point3(sq.x, min(minH, ret[prevCovIdx].sq.y), sq.z);
          else
          {
            sp.y = sq.y = minH;
            prevCovIdx = (int)ret.size();
            ret.push_back(Edge({sp, sq}));
          }
        }

        start = i;
        minH = FLT_MAX;
      }

      minH = min(minH, points[i].height);
      prevH = points[i].height;
    }
  }
};
} // namespace recastbuild
