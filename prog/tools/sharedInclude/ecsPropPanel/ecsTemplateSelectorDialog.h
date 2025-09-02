// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_textFilter.h>
#include <propPanel/commonWindow/dialogWindow.h>

class IECSTemplateSelectorClient;
class ECSBasicObjectEditor;

class ECSTemplateSelectorDialog : public PropPanel::DialogWindow
{
public:
  explicit ECSTemplateSelectorDialog(const char caption[], ECSBasicObjectEditor *editable_object_objed);

  const char *getSelectedTemplate() const { return selectedTemplate; }
  void setClient(IECSTemplateSelectorClient *in_client) { client = in_client; }

private:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void fillTemplates();

  AssetsGuiTextFilter filter;
  String selectedTemplate;
  IECSTemplateSelectorClient *client = nullptr;
  ECSBasicObjectEditor &editor;
};
