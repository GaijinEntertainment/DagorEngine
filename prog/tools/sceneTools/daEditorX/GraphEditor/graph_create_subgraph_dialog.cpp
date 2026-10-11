// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_create_subgraph_dialog.h"

#include <de3_interface.h>
#include <oldEditor/de_interface.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <propPanel/control/customControl.h>
#include <libTools/util/hdpiUtil.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <winGuiWrapper/wgw_input.h>

#include <EASTL/algorithm.h>

#include <imgui/imgui.h>

#include <cstring>

namespace
{
enum
{
  PID_SG_NAME = 11200,
  PID_SG_NAME_HINT,
  PID_SG_LOCATION,
  PID_SG_FINAL_PATH,
};

// Revalidates on every keystroke, so the hint, the final path and the Create button never lag.
class CreateSubgraphDialogHandler final : public PropPanel::ControlEventHandler, public PropPanel::ICustomControl
{
public:
  CreateSubgraphDialogHandler(PropPanel::DialogWindow &dlg, dag::ConstSpan<SubgraphRoot> subgraph_roots) :
    dialog(dlg), roots(subgraph_roots)
  {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_SG_NAME || pcb_id == PID_SG_LOCATION)
    {
      revalidate(*panel);
    }
  }

  // An edit box keeps Enter for itself, so the dialog would never see it.
  long onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key) override
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
    if (v_key == wingw::V_ENTER && valid)
    {
      dialog.clickDialogButton(PropPanel::DIALOG_ID_OK);
    }
    return 0;
  }

  void revalidate(PropPanel::ContainerPropertyControl &panel)
  {
    // The typed name is not rewritten: the caret would jump.
    name = sanitize_subgraph_name(panel.getText(PID_SG_NAME).str());
    rootIndex = eastl::clamp(panel.getInt(PID_SG_LOCATION), 0, static_cast<int>(roots.size()) - 1);
    finalPath.printf(0, "%s%s.subgraph.blk", roots[rootIndex].dir.str(), name.str());
    const char *hint = "";
    valid = false;
    if (name.empty())
    {
      hint = "The name cannot be empty!";
    }
    else if (dd_file_exist(finalPath.str()))
    {
      hint = "Graph already exists with this name";
    }
    else
    {
      valid = true;
    }
    panel.setText(PID_SG_NAME_HINT, hint);
    dialog.setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, valid);
  }

  // constraint: a disabled PropPanel edit box can be neither selected nor copied from.
  void customControlUpdate(int id) override
  {
    G_UNUSED(id);
    // constraint: separateLineLabel needs ImguiHelper::labelOnly from propPanel, which the plugin does not link.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Final path");
    ImGui::PopStyleVar();

    const bool hasName = !name.empty();
    const char *shown = hasName ? finalPath.str() : "";
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    // ReadOnly: ImGui never writes the buffer.
    const bool entered = ImGui::InputText("##final_path", const_cast<char *>(shown), strlen(shown) + 1,
      ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleColor();

    if (ImGui::BeginPopupContextItem("final_path_menu"))
    {
      ImGui::TextDisabled("Subgraph menu");
      ImGui::Separator();
      if (ImGui::BeginMenu("Copy"))
      {
        if (ImGui::MenuItem("File path", nullptr, false, hasName))
        {
          ImGui::SetClipboardText(finalPath.str());
        }
        if (ImGui::MenuItem("Folder path"))
        {
          const String &dir = roots[rootIndex].dir;
          ImGui::SetClipboardText(String(dir.str(), dir.length() - 1).str()); // without the trailing '/'
        }
        if (ImGui::MenuItem("Name", nullptr, false, hasName))
        {
          ImGui::SetClipboardText(name.str());
        }
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("Reveal in Explorer", nullptr, false, hasName))
      {
        DAEDITOR3.revealInExplorer(finalPath.str());
      }
      ImGui::EndPopup();
    }
    if (entered && valid)
    {
      dialog.clickDialogButton(PropPanel::DIALOG_ID_OK);
    }
  }

  String name;
  int rootIndex = 0;
  String finalPath;
  bool valid = false;

private:
  PropPanel::DialogWindow &dialog;
  dag::ConstSpan<SubgraphRoot> roots;
};
} // namespace

String sanitize_subgraph_name(const char *name)
{
  String sanitized(name);
  trim(sanitized);
  make_ident_like_name(sanitized);
  return sanitized;
}

bool prompt_create_subgraph(dag::ConstSpan<SubgraphRoot> roots, const char *proposed_name, CreateSubgraphChoice &out)
{
  if (roots.empty())
  {
    return false;
  }

  // The design's width; autoSize fits the height to the rows.
  PropPanel::DialogWindow *dlg = DAGORED2->createDialog(hdpi::_pxScaled(775), hdpi::_pxScaled(220), "Create subgraph");
  dlg->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Create");
  dlg->setInitialFocus(PID_SG_NAME);

  PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
  CreateSubgraphDialogHandler handler(*dlg, roots);
  panel.setEventHandler(&handler); // must precede the controls: a control copies the panel's handler at creation
  panel.createEditBox(PID_SG_NAME, "Name", proposed_name);
  panel.createStatic(PID_SG_NAME_HINT, "");
  Tab<String> locations;
  for (const SubgraphRoot &root : roots)
  {
    locations.push_back(String(0, "%s - %s", root.label.str(), root.dir.str()));
  }
  panel.createCombo(PID_SG_LOCATION, "Location", locations, 0);
  panel.createCustomControlHolder(PID_SG_FINAL_PATH, &handler);
  handler.revalidate(panel);
  dlg->autoSize();

  const bool accepted = dlg->showDialog() == PropPanel::DIALOG_ID_OK && handler.valid;
  if (accepted)
  {
    out.name = handler.name;
    out.rootIndex = handler.rootIndex;
    out.finalPath = handler.finalPath;
  }
  del_it(dlg);
  return accepted;
}
