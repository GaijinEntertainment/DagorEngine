// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imguiUtil.h>
#include <util/dag_string.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <memory/dag_framemem.h>

// Sometimes we don't just pass string literals as format strings to ImGui controls
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif

static void param(const DataBlock *blk, DataBlock *changes, int i, String &strbuf)
{
  bool editMode = changes != nullptr;

  strbuf = blk->getParamName(i);
  switch (blk->getParamType(i))
  {
    case DataBlock::TYPE_NONE:
    {
      strbuf.append(":none");
      if (editMode)
        ImGui::TextUnformatted(strbuf);
      break;
    }
    case DataBlock::TYPE_STRING:
    {
      const char *str = blk->getStr(i);
      if (editMode)
      {
        eastl::string eastlstr(str);
        if (ImGuiDagor::InputText(strbuf, &eastlstr, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setStr(strbuf, eastlstr.c_str());
      }
      else
        strbuf.aprintf(0, ":t = %s", str);
      break;
    }
    case DataBlock::TYPE_INT:
    {
      int i32 = blk->getInt(i);
      if (editMode)
      {
        if (ImGui::InputInt(strbuf, &i32, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setInt(strbuf, i32);
      }
      else
        strbuf.aprintf(0, ":i = %d", i32);
      break;
    }
    case DataBlock::TYPE_INT64:
    {
      int64_t i64 = blk->getInt64(i);
      if (editMode)
      {
        if (ImGui::InputScalar(strbuf, ImGuiDataType_S64, &i64, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setInt64(strbuf, i64);
      }
      else
        strbuf.aprintf(0, ":i64 = %d", i64);
      break;
    }
    case DataBlock::TYPE_REAL:
    {
      float f = blk->getReal(i);
      if (editMode)
      {
        if (ImGui::InputFloat(strbuf, &f, 0.f, 0.f, "%g", ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setReal(strbuf, f);
      }
      else
        strbuf.aprintf(0, ":r = %g", f);
      break;
    }
    case DataBlock::TYPE_POINT2:
    {
      Point2 p2 = blk->getPoint2(i);
      if (editMode)
      {
        if (ImGui::InputFloat2(strbuf, &p2.x, "%g", ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setPoint2(strbuf, p2);
      }
      else
        strbuf.aprintf(0, ":p2 = %g, %g", p2.x, p2.y);
      break;
    }
    case DataBlock::TYPE_POINT3:
    {
      Point3 p3 = blk->getPoint3(i);
      if (editMode)
      {
        if (ImGui::InputFloat3(strbuf, &p3.x, "%g", ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setPoint3(strbuf, p3);
      }
      else
        strbuf.aprintf(0, ":p3 = %g, %g, %g", p3.x, p3.y, p3.z);
      break;
    }
    case DataBlock::TYPE_POINT4:
    {
      Point4 p4 = blk->getPoint4(i);
      if (editMode)
      {
        if (ImGui::InputFloat4(strbuf, &p4.x, "%g", ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setPoint4(strbuf, p4);
      }
      else
        strbuf.aprintf(0, ":p4 = %g, %g, %g, %g", p4.x, p4.y, p4.z, p4.w);
      break;
    }
    case DataBlock::TYPE_IPOINT2:
    {
      IPoint2 ip2 = blk->getIPoint2(i);
      if (editMode)
      {
        if (ImGui::InputInt2(strbuf, &ip2.x, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setIPoint2(strbuf, ip2);
      }
      else
        strbuf.aprintf(0, ":ip2 = %d, %d", ip2.x, ip2.y);
      break;
    }
    case DataBlock::TYPE_IPOINT3:
    {
      IPoint3 ip3 = blk->getIPoint3(i);
      if (editMode)
      {
        if (ImGui::InputInt3(strbuf, &ip3.x, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setIPoint3(strbuf, ip3);
      }
      else
        strbuf.aprintf(0, ":ip3 = %d, %d, %d", ip3.x, ip3.y, ip3.z);
      break;
    }
    case DataBlock::TYPE_IPOINT4:
    {
      IPoint4 ip4 = blk->getIPoint4(i);
      if (editMode)
      {
        if (ImGui::InputInt4(strbuf, &ip4.x, ImGuiInputTextFlags_EnterReturnsTrue))
          changes->setIPoint4(strbuf, ip4);
      }
      else
        strbuf.aprintf(0, ":ip4 = %d, %d, %d, %d", ip4.x, ip4.y, ip4.z, ip4.w);
      break;
    }
    case DataBlock::TYPE_BOOL:
    {
      bool b = blk->getBool(i);
      if (editMode)
      {
        if (ImGui::Checkbox(strbuf, &b))
          changes->setBool(strbuf, b);
      }
      else
        strbuf.aprintf(0, ":b = %s", b ? "yes" : "no");
      break;
    }
    case DataBlock::TYPE_E3DCOLOR:
    {
      E3DCOLOR c = blk->getE3dcolor(i);
      if (editMode)
      {
        float cf[4] = {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
        if (ImGui::ColorEdit4(strbuf, cf))
        {
          c.r = (unsigned char)roundf(cf[0] * 255.f);
          c.g = (unsigned char)roundf(cf[1] * 255.f);
          c.b = (unsigned char)roundf(cf[2] * 255.f);
          c.a = (unsigned char)roundf(cf[3] * 255.f);
          changes->setE3dcolor(strbuf, c);
        }
      }
      else
        strbuf.aprintf(0, ":c = %d, %d, %d, %d", c.r, c.g, c.b, c.a);
      break;
    }
    case DataBlock::TYPE_MATRIX:
    {
      TMatrix m = blk->getTm(i);
      if (editMode)
      {
        ImGui::TextUnformatted(strbuf);
        ImGui::Indent();
        bool changed = false;
        changed |= ImGui::InputFloat3("col0##" + strbuf, m.m[0], "%g", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat3("col1##" + strbuf, m.m[1], "%g", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat3("col2##" + strbuf, m.m[2], "%g", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat3("col3##" + strbuf, m.m[3], "%g", ImGuiInputTextFlags_EnterReturnsTrue);
        if (changed)
          changes->setTm(strbuf, m);
        ImGui::Unindent();
      }
      else
      {
        strbuf.aprintf(0, ":m = [%g, %g, %g] [%g, %g, %g] [%g, %g, %g] [%g, %g, %g]", m.getcol(0).x, m.getcol(0).y, m.getcol(0).z,
          m.getcol(1).x, m.getcol(1).y, m.getcol(1).z, m.getcol(2).x, m.getcol(2).y, m.getcol(2).z, m.getcol(3).x, m.getcol(3).y,
          m.getcol(3).z);
      }
      break;
    }
  }

  if (!editMode)
  {
    ImGui::TextUnformatted(strbuf);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", strbuf.c_str());
    if (ImGui::BeginPopupContextItem(strbuf))
    {
      if (ImGui::MenuItem("Copy line as text"))
        ImGui::SetClipboardText(strbuf);
      if (ImGui::MenuItem("Copy value as text"))
      {
        const char *valueSubStr = strstr(strbuf, " = ");
        if (valueSubStr != nullptr)
          ImGui::SetClipboardText(valueSubStr + 3);
      }
      ImGui::EndPopup();
    }
  }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

static void blk_internal(const DataBlock *blk, DataBlock *changes, bool default_open, const char *block_name_override)
{
  if (blk == nullptr)
    return;

  String strbuf(framemem_ptr());
  strbuf = block_name_override != nullptr ? block_name_override : blk->getBlockName();

  bool open = ImGui::TreeNodeEx(strbuf, default_open ? ImGuiTreeNodeFlags_DefaultOpen : 0);

  if (ImGui::BeginPopupContextItem(strbuf))
  {
    if (ImGui::MenuItem("Copy block as text"))
    {
      DataBlock blkWithOnlyThisBlock(framemem_ptr());
      blkWithOnlyThisBlock.addNewBlock(blk, strbuf);
      DynamicMemGeneralSaveCB dump(framemem_ptr());
      blkWithOnlyThisBlock.saveToTextStream(dump);
      dump.write("\0", 1);
      String blockAsText(0, "%s", dump.data());
      ImGui::SetClipboardText(blockAsText);
    }
    ImGui::EndPopup();
  }

  if (open)
  {
    for (int i = 0; i < blk->blockCount(); i++)
    {
      const DataBlock *nextBlock = blk->getBlock(i);
      if (changes != nullptr) // edit mode
      {
        DataBlock *nextChangesBlock = changes->addBlock(nextBlock->getBlockName());
        blk_internal(nextBlock, nextChangesBlock, false, nullptr);
        if (nextChangesBlock->isEmpty())
          changes->removeBlock(nextChangesBlock->getBlockName());
      }
      else
      {
        blk_internal(nextBlock, nullptr, false, nullptr);
      }
    }

    for (int i = 0; i < blk->paramCount(); i++)
      param(blk, changes, i, strbuf);

    ImGui::TreePop();
  }
}

void ImGuiDagor::Blk(const DataBlock *blk, bool default_open, const char *block_name_override)
{
  blk_internal(blk, nullptr, default_open, block_name_override);
}

void ImGuiDagor::BlkEdit(const DataBlock *blk, DataBlock *changes, bool default_open, const char *block_name_override)
{
  blk_internal(blk, changes, default_open, block_name_override);
}