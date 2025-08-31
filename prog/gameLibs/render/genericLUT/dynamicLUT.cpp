// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/genericLUT/genericLUT.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <render/genericLUT/dynamicLut.h>
#include <shaders/dag_shaders.h>

#define SHADER_VAR_LIST_TONEMAP \
  VAR(tonemapContrast)          \
  VAR(tonemapShoulder)          \
  VAR(tonemapMidIn)             \
  VAR(tonemapMidOut)            \
  VAR(tonemapHdrMax)


#define SHADER_VAR_LIST_COLOR_GRADING \
  VAR(colorCorrectionHighlightsMin)   \
  VAR(colorCorrectionShadowsMax)      \
  VAR(hueToSaturationKeyFalloff)

#define SHADER_VAR_LIST_COLOR     \
  VAR(colorSaturation)            \
  VAR(colorContrast)              \
  VAR(colorGamma)                 \
  VAR(colorGain)                  \
  VAR(colorOffset)                \
  VAR(shadows_colorSaturation)    \
  VAR(shadows_colorContrast)      \
  VAR(shadows_colorGamma)         \
  VAR(shadows_colorGain)          \
  VAR(shadows_colorOffset)        \
  VAR(midtones_colorSaturation)   \
  VAR(midtones_colorContrast)     \
  VAR(midtones_colorGamma)        \
  VAR(midtones_colorGain)         \
  VAR(midtones_colorOffset)       \
  VAR(highlights_colorSaturation) \
  VAR(highlights_colorContrast)   \
  VAR(highlights_colorGamma)      \
  VAR(highlights_colorGain)       \
  VAR(highlights_colorOffset)     \
  VAR(hueToSaturationKey)         \
  VAR(hueToSaturationValueMul)    \
  VAR(hueToSaturationValueAdd)

#define VAR(a) static int a##VarId = -1;
SHADER_VAR_LIST_COLOR
SHADER_VAR_LIST_TONEMAP
SHADER_VAR_LIST_COLOR_GRADING
#undef VAR

void DynamicLutManager::setColorGradingToShader(float baseWhiteTemp, float timeOfDay, int whiteTempVarId)
{
  if (dynamicLutParamsList.size() <= 0)
    return;

  auto setColorGradingToShader = [](ColorGradingParam param) {
#define VAR(a) ShaderGlobal::set_color4(a##VarId, param.a);
    SHADER_VAR_LIST_COLOR
#undef VAR
#define VAR(a) ShaderGlobal::set_real(a##VarId, param.a);
    SHADER_VAR_LIST_COLOR_GRADING
#undef VAR
  };

  if (dynamicLutParamsList.size() == 1)
  {
    DynamicLutParams dynamicLutAt = dynamicLutParamsList[0];
    float whiteTemp = baseWhiteTemp + dynamicLutAt.colorGradingParams.baseWhiteTemp;
    ShaderGlobal::set_real(whiteTempVarId, whiteTemp);
    setColorGradingToShader(dynamicLutAt.colorGradingParams);

    return;
  }

  DynamicLutParams finalParams = getLerpedLutParams(dynamicLutParamsList, timeOfDay, tonemappingEaseInEaseOutPower);

  setColorGradingToShader(finalParams.colorGradingParams);

  DynamicLutParams dynamicLutAt = dynamicLutParamsList[0];
  float whiteTemp = baseWhiteTemp + dynamicLutAt.colorGradingParams.baseWhiteTemp;
  ShaderGlobal::set_real(whiteTempVarId, whiteTemp);
}

void DynamicLutManager::setTonemapToShader(float timeOfDay)
{
  if (dynamicLutParamsList.size() <= 0)
    return;

  auto setTonemapToShader = [](TonemappingParam param) {
#define VAR(a) ShaderGlobal::set_real(a##VarId, param.a);
    SHADER_VAR_LIST_TONEMAP
#undef VAR
  };

  if (dynamicLutParamsList.size() == 1)
  {
    DynamicLutParams tonemapAt = dynamicLutParamsList[0];
    setTonemapToShader(tonemapAt.tonemappingParams);
    return;
  }

  DynamicLutParams finalParams = getLerpedLutParams(dynamicLutParamsList, timeOfDay, tonemappingEaseInEaseOutPower);
  setTonemapToShader(finalParams.tonemappingParams);
}

void DynamicLutManager::loadBlk(const DataBlock &weatherBlk)
{
  dynamicLutBlk = *weatherBlk.getBlockByNameEx("dynamicLut");

  tonemappingEaseInEaseOutPower = dynamicLutBlk.getReal("ease_in_ease_out_power", 2.f);

#define VAR(a) a##VarId = ::get_shader_variable_id(#a);
  SHADER_VAR_LIST_COLOR
  SHADER_VAR_LIST_TONEMAP
  SHADER_VAR_LIST_COLOR_GRADING
#undef VAR

  dynamicLutParamsList.clear();
  for (int i = 0; i < dynamicLutBlk.blockCount(); ++i)
  {
    dynamicLutParamsList.push_back();
    DataBlock *lutAtBlk = dynamicLutBlk.getBlock(i);
    dynamicLutParamsList[i].at = lutAtBlk->getReal("at", 0.0f);
    DataBlock *tonemapAtBlk = lutAtBlk->getBlockByName("tonemapping");
    if (tonemapAtBlk)
    {
      TonemappingParam tonemapAt;
#define VAR(a) tonemapAt.a = tonemapAtBlk->getReal(#a, 0.0f);
      SHADER_VAR_LIST_TONEMAP
#undef VAR
      dynamicLutParamsList[i].tonemappingParams = tonemapAt;
    }
    DataBlock *colorGradingAtBlk = lutAtBlk->getBlockByName("colorGrading");
    if (colorGradingAtBlk)
    {
      ColorGradingParam colorGradingAt;

#define VAR(a) colorGradingAt.a = colorGradingAtBlk->getPoint4(#a, Point4(0.f, 0.f, 0.f, 0.f));
      SHADER_VAR_LIST_COLOR
#undef VAR
#define VAR(a) colorGradingAt.a = colorGradingAtBlk->getReal(#a, 0.0f);
      SHADER_VAR_LIST_COLOR_GRADING
#undef VAR
      colorGradingAt.baseWhiteTemp = colorGradingAtBlk->getReal("baseWhiteTemp", 6500.0f);
      dynamicLutParamsList[i].colorGradingParams = colorGradingAt;
    }
  }
}

DynamicLutParams DynamicLutManager::getLerpedLutParams(const Tab<DynamicLutParams> &dynamicLut_array, float time,
  float lerping_power) const
{
  int nextTimeOfDayIndex = 0;
  for (int i = 0; i < dynamicLut_array.size(); ++i)
  {
    if (time <= dynamicLut_array[i].at)
    {
      nextTimeOfDayIndex = i;
      break;
    }
  }

  DynamicLutParams nextTimeOfTheDayTonemapParam = dynamicLut_array[nextTimeOfDayIndex];
  DynamicLutParams previousTimeOfTheDayTonemapParam;
  if (nextTimeOfDayIndex == 0) // first pos in array
    previousTimeOfTheDayTonemapParam = dynamicLut_array[dynamicLut_array.size() - 1];
  else
    previousTimeOfTheDayTonemapParam = dynamicLut_array[nextTimeOfDayIndex - 1];

  // we now calculate closest parameters for each value
  auto easeInEaseOut = [](float t, float power) {
    float easeIn = pow(t, power);
    float easeOut = 1 - pow(1 - t, power);
    return lerp(easeIn, easeOut, t);
  };
  DynamicLutParams toReturn;
  float t = safediv(time - previousTimeOfTheDayTonemapParam.at, nextTimeOfTheDayTonemapParam.at - previousTimeOfTheDayTonemapParam.at);

#define VAR(a)                                                                                \
  toReturn.colorGradingParams.a = lerp(previousTimeOfTheDayTonemapParam.colorGradingParams.a, \
    nextTimeOfTheDayTonemapParam.colorGradingParams.a, easeInEaseOut(t, lerping_power));
  SHADER_VAR_LIST_COLOR
  SHADER_VAR_LIST_COLOR_GRADING
#undef VAR
#define VAR(a)                                                                              \
  toReturn.tonemappingParams.a = lerp(previousTimeOfTheDayTonemapParam.tonemappingParams.a, \
    nextTimeOfTheDayTonemapParam.tonemappingParams.a, easeInEaseOut(t, lerping_power));
  SHADER_VAR_LIST_TONEMAP
#undef VAR

  toReturn.colorGradingParams.baseWhiteTemp = lerp(previousTimeOfTheDayTonemapParam.colorGradingParams.baseWhiteTemp,
    nextTimeOfTheDayTonemapParam.colorGradingParams.baseWhiteTemp, easeInEaseOut(t, lerping_power)); // not in the sahder list

  return toReturn;
}

#if DAGOR_DBGLEVEL > 0

void DynamicLutManager::setImGuiWeatherPath(const char *path) { strncpy(imGuiWeatherPath, path, sizeof(imGuiWeatherPath)); }

DynamicLutParams *DynamicLutManager::getDynamicLutParamsAt(int at)
{
  if (dynamicLutParamsList.size() <= at)
    return nullptr;

  return &dynamicLutParamsList[at];
}

void DynamicLutManager::setDynamicLutParamsAt(DynamicLutParams &param, int at)
{
  if (dynamicLutParamsList.size() <= at)
    return;
  dynamicLutParamsList[at] = param;
}

#include <stdio.h>

void DynamicLutManager::save()
{
  DataBlock imGuiWeatherBlk;
  dblk::load(imGuiWeatherBlk, imGuiWeatherPath, dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);

  DataBlock *weatherCommonBlk = imGuiWeatherBlk.addBlock("common");
  DataBlock *dynamicLutBlock = weatherCommonBlk->addBlock("dynamicLut");
  dynamicLutBlock->removeBlock("Lut");

  dynamicLutBlock->setReal("ease_in_ease_out_power", tonemappingEaseInEaseOutPower);
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    DynamicLutParams *dynamicLut = getDynamicLutParamsAt(i);
    DataBlock *dynamicLutAt = dynamicLutBlock->addNewBlock("Lut");
    dynamicLutAt->setReal("at", dynamicLut->at);

    DataBlock *tonemappingBlk = dynamicLutAt->addBlock("tonemapping");
#define VAR(a) tonemappingBlk->setReal(#a, dynamicLut->tonemappingParams.a);
    SHADER_VAR_LIST_TONEMAP
#undef VAR

    DataBlock *colorGradingBlk = dynamicLutAt->addBlock("colorGrading");
#define VAR(a) colorGradingBlk->setPoint4(#a, dynamicLut->colorGradingParams.a);
    SHADER_VAR_LIST_COLOR
#undef VAR
#define VAR(a) colorGradingBlk->setReal(#a, dynamicLut->colorGradingParams.a);
    SHADER_VAR_LIST_COLOR_GRADING
#undef VAR
  }

  char fileData[255];
  snprintf(fileData, sizeof(fileData), "%s%s", "../develop/gameBase/", imGuiWeatherPath);
  imGuiWeatherBlk.saveToTextFile(fileData);
}

void DynamicLutManager::removeDynamicLutParamAt(int at)
{
  if (dynamicLutParamsList.size() <= at)
    return;
  dynamicLutParamsList.erase_unsorted(&dynamicLutParamsList[at]);
}

void DynamicLutManager::removeAt(float at)
{
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    if (getDynamicLutParamsAt(i)->at == at)
      removeDynamicLutParamAt(i);
  }

  save();
}

bool DynamicLutManager::addNewCopyLut(float at, float keyToCopy)
{
  DynamicLutParams newParam;
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    if (at == getDynamicLutParamsAt(i)->at)
      return false;
    if (keyToCopy == getDynamicLutParamsAt(i)->at)
    {
      newParam = *getDynamicLutParamsAt(i);
      newParam.at = at;
      break;
    }
  }
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    if (at < getDynamicLutParamsAt(i)->at)
    {
      dynamicLutParamsList.push_back();
      for (int j = dynamicLutParamsList.size() - 2; j >= i; --j)
        setDynamicLutParamsAt(*getDynamicLutParamsAt(j), j + 1);
      setDynamicLutParamsAt(newParam, i);
      return true;
    }
  }
  dynamicLutParamsList.push_back();
  setDynamicLutParamsAt(newParam, dynamicLutParamsList.size() - 1);
  return true;
}

void DynamicLutManager::addNewLut(float at)
{
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    if (at == getDynamicLutParamsAt(i)->at)
      return;
  }

  TonemappingParam defaultToneMap{1.19f, 0.95f, 0.25f, 0.18f, 4.f};

  ColorGradingParam defaultColorGrading{
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(0, 0, 0, 0),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(0, 0, 0, 0),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(0, 0, 0, 0),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(1, 1, 1, 1),
    Point4(0, 0, 0, 0),
    Point4(1, 0, 0, 0),
    Point4(1, 1, 1, 1),
    Point4(0, 0, 0, 0),
    0.1,
    0.8,
    0.001,
    6500,
  };

  DynamicLutParams defaultLut{defaultToneMap, defaultColorGrading, at};

  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    if (at < getDynamicLutParamsAt(i)->at)
    {
      dynamicLutParamsList.push_back();
      for (int j = dynamicLutParamsList.size() - 2; j >= i; --j)
        setDynamicLutParamsAt(*getDynamicLutParamsAt(j), j + 1);
      setDynamicLutParamsAt(defaultLut, i);
      return;
    }
  }
  dynamicLutParamsList.push_back();
  setDynamicLutParamsAt(defaultLut, dynamicLutParamsList.size() - 1);
}

// for imgui panel
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

static int addButtonAtHour = 0;
static int addButtonAtMin = 0;
static float valueMinute = 0.0f;
void DynamicLutManager::showLutWindow(double star_julian_day, double &minute_change)
{
  minute_change = 0.0;
  ImGui::SliderInt("hour", &addButtonAtHour, 0, 24);
  ImGui::SliderInt("min", &addButtonAtMin, 0, 60);
  ImGui::SameLine();
  if (ImGui::Button(" + "))
  {
    addNewLut(addButtonAtHour * 60.0f + addButtonAtMin);
  }
  float tempEaseInEaseOutPower = tonemappingEaseInEaseOutPower;
  ImGui::SliderFloat("easeIn easeOut Power", &tempEaseInEaseOutPower, 0.0f, 10.0f);
  tonemappingEaseInEaseOutPower = tempEaseInEaseOutPower;
  for (int i = 0; i < dynamicLutParamsList.size(); ++i)
  {
    DynamicLutParams *dynamicLutParam = getDynamicLutParamsAt(i);
    float timeAsMin = dynamicLutParam->at;
    int hour = floor((int)timeAsMin / 60);
    timeAsMin -= 60 * hour;
    char buffer[64] = "";
    snprintf(buffer + strlen(buffer), sizeof(buffer), "Lut At : %d h : %d min", hour, (int)timeAsMin);
    if (ImGui::CollapsingHeader(buffer))
    {
      ImGui::Indent(32.0f);
      ImGui::PushID(i);
      if (ImGui::CollapsingHeader("tonemap"))
      {
        ImGui::Indent(32.0f);
        TonemappingParam &tonemapParam = dynamicLutParam->tonemappingParams;
        ImGui::SliderFloat("Tonemap Contrast", &tonemapParam.tonemapContrast, -10.0, 10.0);
        ImGui::SliderFloat("Tonemap Shoulder", &tonemapParam.tonemapShoulder, -10.0, 10.0);
        ImGui::SliderFloat("Tonemap Mid In", &tonemapParam.tonemapMidIn, -10.0, 10.0);
        ImGui::SliderFloat("Tonemap Mid Out", &tonemapParam.tonemapMidOut, -10.0, 10.0);
        ImGui::SliderFloat("Tonemap Hdr Max", &tonemapParam.tonemapHdrMax, -10.0, 10.0);
        ImGui::Unindent(32.0f);
      }
      if (ImGui::CollapsingHeader("colorGrading"))
      {
        ImGui::Indent(32.0f);
        ColorGradingParam &colorGradingParam = dynamicLutParam->colorGradingParams;

        float buff[4] = {colorGradingParam.colorSaturation.x, colorGradingParam.colorSaturation.y, colorGradingParam.colorSaturation.z,
          colorGradingParam.colorSaturation.w};
#define set_buffer(value) \
  buff[0] = value.x;      \
  buff[1] = value.y;      \
  buff[2] = value.z;      \
  buff[3] = value.w;

#define set_values(value) value.set(buff[0], buff[1], buff[2], buff[3]);

#define imgui_slider(name)                           \
  set_buffer(colorGradingParam.name);                \
  if (ImGui::SliderFloat4(#name, buff, -10.0, 10.0)) \
    set_values(colorGradingParam.name);

#define VAR(a) imgui_slider(a);
        SHADER_VAR_LIST_COLOR
#undef VAR
        ImGui::SliderFloat("colorCorrectionHighlightsMin", &colorGradingParam.colorCorrectionHighlightsMin, -10.0, 10.0);
        ImGui::SliderFloat("colorCorrectionShadowsMax", &colorGradingParam.colorCorrectionShadowsMax, -10.0, 10.0);
        ImGui::SliderFloat("hueToSaturationKeyFalloff", &colorGradingParam.hueToSaturationKeyFalloff, -10, 10);
        ImGui::SliderFloat("baseWhiteTemp", &colorGradingParam.baseWhiteTemp, -10000.0, 10000.0);
        ImGui::Unindent(32.0f);
      }
      if (ImGui::Button("Remove"))
        removeAt(dynamicLutParam->at);
      ImGui::PopID();
      ImGui::SliderInt("hour", &addButtonAtHour, 0, 24);
      ImGui::SliderInt("min", &addButtonAtMin, 0, 60);
      if (ImGui::Button("change time key"))
      {
        if (addNewCopyLut(addButtonAtHour * 60.0f + addButtonAtMin, dynamicLutParam->at))
          removeAt(dynamicLutParam->at);
      }
      if (ImGui::Button("copy"))
        addNewCopyLut(addButtonAtHour * 60.0f + addButtonAtMin, dynamicLutParam->at);
      ImGui::Unindent(32.0f);
    }
  }

  if (ImGui::Button(" save "))
    save();

  if (ImGui::InputFloat("##", &valueMinute))
    ImGui::SameLine();
  if (ImGui::Button("add Minute"))
  {
    minute_change = valueMinute / 1440.0f;
  }

  float debugTime = abs((floor(star_julian_day) - star_julian_day)) * 1440;
  char buffer[128] = "";
  int hour = floor((int)debugTime / 60);
  snprintf(buffer, sizeof(buffer), "toneMap At : %d h : %d min", hour, (int)debugTime - 60 * hour);
  ImGui::Text(buffer, "");
  if (ImGui::CollapsingHeader("Params at Time") && dynamicLutParamsList.size() > 0)
  {
    ImGui::Indent(32.0f);
    DynamicLutParams finalParams = getLerpedLutParams(dynamicLutParamsList, debugTime, tonemappingEaseInEaseOutPower);
#define imgui_text_tonemapping(name)                                                   \
  snprintf(buffer, sizeof(buffer), #name " : %f", finalParams.tonemappingParams.name); \
  ImGui::Text(buffer, "");

    if (ImGui::CollapsingHeader("Tonemapping"))
    {
      ImGui::Indent(32.0f);
#define VAR(a) imgui_text_tonemapping(a);
      SHADER_VAR_LIST_TONEMAP
#undef VAR
      ImGui::Unindent(32.0f);
    }

#define imgui_text_color_grading_xyzw(name)                                                                               \
  snprintf(buffer, sizeof(buffer), #name " : %f / %f / %f / %f", finalParams.colorGradingParams.name.x,                   \
    finalParams.colorGradingParams.name.y, finalParams.colorGradingParams.name.z, finalParams.colorGradingParams.name.w); \
  ImGui::Text(buffer, "");
#define imgui_text_color_grading(name)                                                  \
  snprintf(buffer, sizeof(buffer), #name " : %f", finalParams.colorGradingParams.name); \
  ImGui::Text(buffer, "");

    if (ImGui::CollapsingHeader("Color Grading"))
    {
      ImGui::Indent(32.0f);
#define VAR(a) imgui_text_color_grading_xyzw(a);
      SHADER_VAR_LIST_COLOR
#undef VAR
#define VAR(a) imgui_text_color_grading(a);
      SHADER_VAR_LIST_COLOR_GRADING
#undef VAR
      imgui_text_color_grading(baseWhiteTemp);
      ImGui::Unindent(32.0f);
    }
    ImGui::Unindent(32.0f);
  }
  ImGui::Text("if changing time from console, keep this window open to see the changes");
}

#endif
