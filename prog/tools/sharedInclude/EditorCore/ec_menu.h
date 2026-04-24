// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{
class IMenu;
}

// EditorCore version of PropPanel::create_menu(). It supports editor commands.
PropPanel::IMenu *ec_create_menu();

// EditorCore version of PropPanel::create_context_menu(). It supports editor commands.
PropPanel::IMenu *ec_create_context_menu();
