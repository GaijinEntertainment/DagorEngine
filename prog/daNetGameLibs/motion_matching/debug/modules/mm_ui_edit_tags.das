options no_aot
module mm_ui_edit_tags shared

require ecs
require imgui
require imgui/imgui_boost
require MotionMatching
require danetlibs.motion_matching.debug.modules.mm_ui_imgui_helpers


def imgui_empty_space(width : float)
  imgui::Dummy(float2(width, 0.f))
  imgui::SameLine()

def table_with_tags(dataBase : AnimationDataBase; var controller : MotionMatchingController&)
  if hasAnimGraphTags(dataBase)
    safe_checkbox("use tags from animTree", controller.useTagsFromAnimGraph)
  else
    imgui::Text("no tags in animTree")
  let tagsEditEnabled = !controller.useTagsFromAnimGraph
  if !tagsEditEnabled
    imgui::PushStyleColor(imgui::ImGuiCol.Text, ImGuiStyle().Colors[int(ImGuiCol.TextDisabled)])
    imgui::PushStyleColor(imgui::ImGuiCol.FrameBgHovered, ImGuiStyle().Colors[int(ImGuiCol.FrameBg)])
  let tableFlags = ImGuiTableFlags.SizingFixedFit | ImGuiTableFlags.NoHostExtendX | ImGuiTableFlags.RowBg
  if imgui::BeginTable("TagsTable", 4, tableFlags)
    imgui::TableSetupColumn("Tag", ImGuiTableColumnFlags.WidthFixed)
    imgui::TableSetupColumn("Require", ImGuiTableColumnFlags.WidthFixed)
    imgui::TableSetupColumn("Exclude", ImGuiTableColumnFlags.WidthFixed)
    imgui::TableSetupColumn(" None ", ImGuiTableColumnFlags.WidthFixed)
    imgui::TableHeadersRow()
    for i in range(dataBase |> getTagsCount())
      imgui::TableNextRow()
      imgui::TableSetColumnIndex(0)
      let tagName = dataBase |> getTagName(i)
      imgui::Text("{tagName}")
      let tagIdx = uint(i)
      imgui::TableSetColumnIndex(1)
      imgui_empty_space(8.f)
      if imgui::RadioButton("##require{i}", controller.currentTags |> isTagRequired(tagIdx)) && tagsEditEnabled
        controller.currentTags |> requireTag(tagIdx)
      imgui::TableSetColumnIndex(2)
      imgui_empty_space(8.f)
      if imgui::RadioButton("##exclude{i}", controller.currentTags |> isTagExcluded(tagIdx)) && tagsEditEnabled
        controller.currentTags |> excludeTag(tagIdx)
      imgui::TableSetColumnIndex(3)
      imgui_empty_space(3.f)
      let tagRemoved = (!isTagRequired(controller.currentTags, tagIdx) &&
                        !isTagExcluded(controller.currentTags, tagIdx))
      if imgui::RadioButton("##none{i}", tagRemoved) && tagsEditEnabled
        controller.currentTags |> removeTag(tagIdx)
    if !tagsEditEnabled
      imgui::PopStyleColor()
      imgui::PopStyleColor()
    imgui::EndTable()

[unused_argument(eid)]
def edit_tags(eid : EntityId; dataBase : AnimationDataBase; var controller : MotionMatchingController&)
  if imgui::BeginTabItem("tags")
    table_with_tags(dataBase, controller)
    imgui::EndTabItem()
