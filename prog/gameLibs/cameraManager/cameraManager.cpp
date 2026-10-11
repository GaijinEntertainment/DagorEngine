// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cameraManager/cameraManager_api.h>

#if DAGOR_DBGLEVEL > 0

#include <math/dag_mathBase.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <image/dag_jpeg.h>
#include <image/dag_png.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <folders/folders.h>
#include <util/dag_string.h>
#include <memory/dag_mem.h>
#include <debug/dag_log.h>
#include <EASTL/vector.h>
#include <EASTL/sort.h>
#include <cstring>
#include <cctype>

namespace camera_manager
{

static constexpr int THUMBNAIL_DOWNSCALE = 4;
static constexpr int NAME_MAX_LEN = 64;
static constexpr int PENDING_TIMEOUT_MSEC = 3000;

struct SavedCamera
{
  eastl::string name;
  CameraState state;
  UniqueTex thumbnail;
};

enum class PendingKind
{
  None,
  Add,
  Update
};

struct PendingCapture
{
  PendingKind kind = PendingKind::None;
  eastl::string name;
  eastl::string token;
  CameraState state;
  int requestedAtMsec = 0;
};

static GameHooks &get_hooks()
{
  static GameHooks hooks;
  return hooks;
}
static eastl::vector<SavedCamera> cameras;
static eastl::string loadedMission;
static int selectedIdx = -1;
static float thumbnailWidth = 192.f;
static PendingCapture pending;
static eastl::string consumedToken;
static unsigned requestSerial = 0;
static char nameInput[NAME_MAX_LEN] = "";
static eastl::string lastError;

static String mission_dir() { return String(0, "%s/CameraManager/%s", folders::get_game_dir().str(), loadedMission.c_str()); }
static String blk_path(const char *name) { return String(0, "%s/%s.blk", mission_dir().str(), name); }
static String png_path(const char *name) { return String(0, "%s/%s.png", mission_dir().str(), name); }

static eastl::string current_mission()
{
  const eastl::string raw = get_hooks().getMissionName();
  if (raw.empty())
    return raw;
  char buf[DAGOR_MAX_PATH];
  return eastl::string(dd_get_fname_without_path_and_ext(buf, sizeof(buf), raw.c_str()));
}

static eastl::string sanitize_name(const char *raw)
{
  eastl::string name;
  for (const char *p = raw; *p; ++p)
    name += (isalnum((unsigned char)*p) || strchr("_-.() ", *p)) ? *p : '_';
  while (!name.empty() && name.back() == ' ')
    name.pop_back();
  while (!name.empty() && name.front() == ' ')
    name.erase(name.begin());
  if (name.find_first_not_of(". ") == eastl::string::npos)
    name.clear();
  return name;
}

static int find_camera(const eastl::string &name)
{
  for (int i = 0; i < (int)cameras.size(); ++i)
    if (dd_stricmp(cameras[i].name.c_str(), name.c_str()) == 0)
      return i;
  return -1;
}

static void sort_cameras()
{
  eastl::sort(cameras.begin(), cameras.end(), [](const SavedCamera &a, const SavedCamera &b) { return a.name < b.name; });
}

static void set_thumbnail(SavedCamera &cam, TexImage32 *img)
{
  cam.thumbnail.close();
  String texName(0, "camera_manager_thumb_%s_%s", loadedMission.c_str(), cam.name.c_str());
  cam.thumbnail = dag::create_tex(img, img->w, img->h, TEXFMT_A8R8G8B8, 1, texName);
}

static void load_cameras(const eastl::string &mission)
{
  cameras.clear();
  selectedIdx = -1;
  loadedMission = mission;
  if (mission.empty())
    return;
  const String dir = mission_dir();
  for (const alefind_t &ff : dd_find_iterator(String(0, "%s/*.blk", dir.str()), DA_FILE))
  {
    DataBlock blk;
    if (!blk.load(String(0, "%s/%s", dir.str(), ff.name)))
      continue;
    SavedCamera cam;
    char buf[DAGOR_MAX_PATH];
    cam.name = dd_get_fname_without_path_and_ext(buf, sizeof(buf), ff.name);
    cam.state.itm = blk.getTm("itm", TMatrix::IDENT);
    cam.state.fovDeg = blk.getReal("fovDeg", cam.state.fovDeg);
    if (TexImage32 *img = load_png32(png_path(cam.name.c_str()), tmpmem))
    {
      set_thumbnail(cam, img);
      memfree(img, tmpmem);
    }
    cameras.push_back(eastl::move(cam));
  }
  sort_cameras();
}

static bool save_camera(const SavedCamera &cam, const TexImage32 *thumb)
{
  const String blkPath = blk_path(cam.name.c_str());
  DataBlock blk;
  blk.setTm("itm", cam.state.itm);
  blk.setReal("fovDeg", cam.state.fovDeg);
  if (!dd_mkpath(blkPath) || !blk.saveToTextFile(blkPath))
  {
    logerr("camera_manager: cannot write '%s'", blkPath.str());
    lastError = "Cannot write the camera file";
    return false;
  }
  if (thumb && !save_png32(png_path(cam.name.c_str()), thumb->getPixels(), thumb->w, thumb->h, int(thumb->w * sizeof(TexPixel32)),
                 nullptr, 0, false))
    logerr("camera_manager: cannot write '%s'", png_path(cam.name.c_str()).str());
  return true;
}

static bool has_selection() { return selectedIdx >= 0 && selectedIdx < (int)cameras.size(); }

static void remove_selected()
{
  if (!has_selection())
    return;
  const eastl::string name = cameras[selectedIdx].name;
  dd_erase(blk_path(name.c_str()));
  dd_erase(png_path(name.c_str()));
  cameras.erase(cameras.begin() + selectedIdx);
  selectedIdx = -1;
}

static TexImage32 *make_thumbnail(const TexPixel32 *src, int w, int h, int stride_bytes, int factor)
{
  const int dw = max(w / factor, 1);
  const int dh = max(h / factor, 1);
  TexImage32 *img = TexImage32::create(dw, dh, tmpmem);
  TexPixel32 *dst = img->getPixels();
  for (int y = 0; y < dh; ++y)
    for (int x = 0; x < dw; ++x)
    {
      unsigned r = 0, g = 0, b = 0, n = 0;
      const int syEnd = min((y + 1) * factor, h);
      const int sxEnd = min((x + 1) * factor, w);
      for (int sy = y * factor; sy < syEnd; ++sy)
      {
        const TexPixel32 *row = (const TexPixel32 *)((const char *)src + sy * stride_bytes);
        for (int sx = x * factor; sx < sxEnd; ++sx, ++n)
        {
          r += row[sx].r;
          g += row[sx].g;
          b += row[sx].b;
        }
      }
      TexPixel32 &o = dst[y * dw + x];
      o.r = r / n;
      o.g = g / n;
      o.b = b / n;
      o.a = 255;
    }
  return img;
}

static bool request_capture(PendingKind kind, const eastl::string &name)
{
  CameraState state;
  if (!get_hooks().getCamera(state))
  {
    lastError = "No camera to read";
    return false;
  }
  pending.kind = kind;
  pending.name = name;
  pending.token = String(0, "camera_manager_%u", ++requestSerial).str();
  pending.state = state;
  pending.requestedAtMsec = get_time_msec();
  lastError.clear();
  get_hooks().requestScreenshot(pending.token.c_str());
  return true;
}

bool is_capture_pending()
{
  if (pending.kind != PendingKind::None && get_time_msec() - pending.requestedAtMsec > PENDING_TIMEOUT_MSEC)
  {
    logwarn("camera_manager: screenshot '%s' did not arrive, the thumbnail request is dropped", pending.token.c_str());
    lastError = "The game saved no screenshot, the request was dropped";
    pending = PendingCapture();
  }
  return pending.kind != PendingKind::None;
}

static TexImage32 *load_screenshot(const char *path)
{
  const char *ext = dd_get_fname_ext(path);
  if (!ext)
    return nullptr;
  if (dd_stricmp(ext, ".jpg") == 0 || dd_stricmp(ext, ".jpeg") == 0)
    return load_jpeg32(path, tmpmem);
  if (dd_stricmp(ext, ".png") == 0)
    return load_png32(path, tmpmem);
  if (dd_stricmp(ext, ".tga") == 0)
    return load_tga32(path, tmpmem);
  return nullptr;
}

bool on_screenshot_saved(const char *path)
{
  if (!path || !*path)
    return false;
  char buf[DAGOR_MAX_PATH];
  const char *stem = dd_get_fname_without_path_and_ext(buf, sizeof(buf), path);
  if (!consumedToken.empty() && strstr(stem, consumedToken.c_str()))
    return dd_erase(path);
  if (!is_capture_pending() || !strstr(stem, pending.token.c_str()))
    return false;
  const PendingCapture req = eastl::move(pending);
  pending = PendingCapture();

  TexImage32 *thumb = nullptr;
  if (TexImage32 *src = load_screenshot(path))
  {
    int screenW = 0, screenH = 0;
    d3d::get_screen_size(screenW, screenH);
    const int factor = screenW > 0 ? max((src->w * THUMBNAIL_DOWNSCALE + screenW / 2) / screenW, 1) : THUMBNAIL_DOWNSCALE;
    thumb = make_thumbnail(src->getPixels(), src->w, src->h, src->w * (int)sizeof(TexPixel32), factor);
    memfree(src, tmpmem);
  }
  else
    logwarn("camera_manager: cannot load screenshot '%s', the thumbnail is not updated", path);

  if (loadedMission.empty())
  {
    if (thumb)
      memfree(thumb, tmpmem);
    return false;
  }

  SavedCamera *cam = nullptr;
  if (req.kind == PendingKind::Add)
  {
    cameras.push_back();
    cam = &cameras.back();
    cam->name = req.name;
  }
  else
  {
    const int idx = find_camera(req.name);
    if (idx >= 0)
      cam = &cameras[idx];
  }

  bool consumed = false;
  if (cam)
  {
    cam->state = req.state;
    const bool saved = save_camera(*cam, thumb);
    if (thumb)
    {
      set_thumbnail(*cam, thumb);
      consumed = saved && dd_erase(path);
      if (consumed)
        consumedToken = req.token;
    }
    if (req.kind == PendingKind::Add)
    {
      sort_cameras();
      selectedIdx = find_camera(req.name);
    }
  }
  if (thumb)
    memfree(thumb, tmpmem);
  return consumed;
}

static void draw_add_popup()
{
  if (!ImGui::BeginPopupModal("Add camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  if (ImGui::IsWindowAppearing())
    ImGui::SetKeyboardFocusHere();
  bool confirm = ImGui::InputText("Name", nameInput, sizeof(nameInput), ImGuiInputTextFlags_EnterReturnsTrue);
  confirm |= ImGui::Button("Add");
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  if (!lastError.empty())
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", lastError.c_str());
  if (confirm)
  {
    const eastl::string name = sanitize_name(nameInput);
    if (name.empty())
      lastError = "Enter a name";
    else if (find_camera(name) >= 0)
      lastError = "A camera with this name already exists";
    else if (request_capture(PendingKind::Add, name))
      ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}

static void draw_remove_popup()
{
  if (!ImGui::BeginPopupModal("Remove camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  if (has_selection())
    ImGui::Text("Remove camera '%s'?", cameras[selectedIdx].name.c_str());
  if (ImGui::Button("Remove"))
  {
    remove_selected();
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

static void draw_grid()
{
  const ImGuiStyle &style = ImGui::GetStyle();
  ImGui::BeginChild("cameras", ImVec2(0.f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

  int screenW = 16, screenH = 9;
  d3d::get_screen_size(screenW, screenH);
  const float cellW = thumbnailWidth;
  const float thumbH = floorf(cellW * screenH / max(screenW, 1));
  const float cellH = thumbH + ImGui::GetTextLineHeightWithSpacing();
  const int columns = max(1, int((ImGui::GetContentRegionAvail().x + style.ItemSpacing.x) / (cellW + style.ItemSpacing.x)));
  ImDrawList *dl = ImGui::GetWindowDrawList();

  if (cameras.empty())
    ImGui::TextDisabled("No saved cameras. Press Add to save the current camera.");

  for (int i = 0; i < (int)cameras.size(); ++i)
  {
    SavedCamera &cam = cameras[i];
    if (i % columns)
      ImGui::SameLine();
    ImGui::PushID(i);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    if (ImGui::Selectable("##cell", i == selectedIdx, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellW, cellH)))
    {
      selectedIdx = i;
      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        get_hooks().setCamera(cam.state);
    }
    const ImVec2 p1(p0.x + cellW, p0.y + thumbH);
    if (cam.thumbnail.getTex2D())
      dl->AddImage(ImGuiDagor::EncodeTexturePtr(cam.thumbnail.getTex2D()), p0, p1);
    else
    {
      dl->AddRectFilled(p0, p1, IM_COL32(60, 60, 60, 255));
      dl->AddText(ImVec2(p0.x + 4.f, p0.y + 4.f), ImGui::GetColorU32(ImGuiCol_TextDisabled), "no thumbnail");
    }
    const float textW = ImGui::CalcTextSize(cam.name.c_str()).x;
    const ImVec2 textPos(p0.x + max((cellW - textW) * 0.5f, 0.f), p0.y + thumbH + style.ItemInnerSpacing.y);
    const ImVec4 clip(p0.x, textPos.y, p1.x, p0.y + cellH);
    dl->AddText(nullptr, 0.f, textPos, ImGui::GetColorU32(ImGuiCol_Text), cam.name.c_str(), nullptr, 0.f, &clip);
    if (ImGui::IsItemHovered())
    {
      const Point3 pos = cam.state.itm.getcol(3);
      ImGui::SetTooltip("%s\npos: %.1f %.1f %.1f\nfov: %.1f\ndouble click to apply", cam.name.c_str(), pos.x, pos.y, pos.z,
        cam.state.fovDeg);
    }
    ImGui::PopID();
  }
  ImGui::EndChild();
}

static void imgui_window()
{
  const GameHooks &hooks = get_hooks();
  if (!hooks.getCamera || !hooks.setCamera || !hooks.getMissionName || !hooks.requestScreenshot)
  {
    ImGui::TextUnformatted("camera_manager::init() was not called by the game");
    return;
  }
  const eastl::string mission = current_mission();
  if (mission != loadedMission)
    load_cameras(mission);
  if (mission.empty())
  {
    ImGui::TextUnformatted("No mission loaded");
    return;
  }

  ImGui::Text("Mission: %s", mission.c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("(%d cameras)", (int)cameras.size());

  const bool hasSelection = has_selection();
  ImGui::BeginDisabled(is_capture_pending());
  if (ImGui::Button("Add"))
  {
    nameInput[0] = 0;
    lastError.clear();
    ImGui::OpenPopup("Add camera");
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(!hasSelection);
  if (ImGui::Button("Update"))
    request_capture(PendingKind::Update, cameras[selectedIdx].name);
  ImGui::SameLine();
  if (ImGui::Button("Remove"))
    ImGui::OpenPopup("Remove camera");
  ImGui::EndDisabled();
  ImGui::EndDisabled();
  if (!lastError.empty() && !ImGui::IsPopupOpen("Add camera"))
  {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", lastError.c_str());
  }

  draw_add_popup();
  draw_remove_popup();
  draw_grid();
  ImGui::SliderFloat("Zoom", &thumbnailWidth, 64.f, 512.f, "%.0f px");
}

REGISTER_IMGUI_WINDOW("Camera", "Camera manager", imgui_window);

void init(const GameHooks &game_hooks) { get_hooks() = game_hooks; }

void shutdown()
{
  cameras.clear();
  loadedMission.clear();
  selectedIdx = -1;
  pending = PendingCapture();
  get_hooks() = GameHooks();
}

} // namespace camera_manager

#endif
