#include "FileTextEdit.h"

//#include <fstream>

#include "FontManager.h"
#include "FileNameUtils.h"

#if _TARGET_PC_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

eastl::unordered_map<eastl::string, TextEditor::LanguageDefinitionId> FileTextEdit::extensionToLanguageDefinition = {
	{".cpp", TextEditor::LanguageDefinitionId::Cpp},
	{".cc", TextEditor::LanguageDefinitionId::Cpp},
	{".hpp", TextEditor::LanguageDefinitionId::Cpp},
	{".h", TextEditor::LanguageDefinitionId::Cpp},
	{".das", TextEditor::LanguageDefinitionId::Daslang},
};
eastl::unordered_map<TextEditor::LanguageDefinitionId, const char *> FileTextEdit::languageDefinitionToName = {
	{TextEditor::LanguageDefinitionId::None, "None"},
	{TextEditor::LanguageDefinitionId::Cpp, "C++"},
	{TextEditor::LanguageDefinitionId::Daslang, "Daslang"},
};
eastl::unordered_map<TextEditor::PaletteId, const char *> FileTextEdit::colorPaletteToName = {
	{TextEditor::PaletteId::Dark, "Dark"},
	{TextEditor::PaletteId::Light, "Light"},
	{TextEditor::PaletteId::Mariana, "Mariana"},
	{TextEditor::PaletteId::RetroBlue, "Retro blue"}
};


FileTextEdit::FileTextEdit(
	const char* filePath,
	int id,
	int createdFromFolderView,
	OnFocusedCallback onFocusedCallback,
	OnShowInFolderViewCallback onShowInFolderViewCallback)
{
	this->id = id;
	this->createdFromFolderView = createdFromFolderView;
	this->onFocusedCallback = onFocusedCallback;
	this->onShowInFolderViewCallback = onShowInFolderViewCallback;
	this->codeFontSize = 18;
	editor = new TextEditor();
	if (filePath == nullptr)
		panelName = "untitled##" + eastl::to_string((size_t)this);
	else
	{
		hasAssociatedFile = true;
		associatedFile = eastl::string(filePath);
		auto fileName = extract_file_name(filePath);
		auto fileExt = extract_file_extension(filePath);
		panelName = fileName + "##" + eastl::to_string((size_t)this) + filePath;
		auto lang = extensionToLanguageDefinition.find(fileExt);
		if (lang != extensionToLanguageDefinition.end())
			editor->SetLanguageDefinition(lang->second);
	}
}

FileTextEdit::~FileTextEdit()
{
	delete editor;
}


void FileTextEdit::SetText(const char* text)
{
	editor->SetText(text);
}

const eastl::string FileTextEdit::GetText()
{
	return editor->GetText();
}

/*static void ClearInputKeys(ImGuiIO * io)
{
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
	memset(io->KeysDown, 0, sizeof(io->KeysDown));
#endif
	for (int n = 0; n < IM_ARRAYSIZE(io->KeysData); n++)
	{
		io->KeysData[n].Down             = false;
		io->KeysData[n].DownDuration     = -1.0f;
		io->KeysData[n].DownDurationPrev = -1.0f;
	}
	io->KeyCtrl = io->KeyShift = io->KeyAlt = io->KeySuper = false;
	io->KeyMods = ImGuiMod_None;
	io->MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	for (int n = 0; n < IM_ARRAYSIZE(MouseDown); n++)
	{
		io->MouseDown[n] = false;
		io->MouseDownDuration[n] = MouseDownDurationPrev[n] = -1.0f;
	}
	io->MouseWheel = io->MouseWheelH = 0.0f;
} */


bool FileTextEdit::OnImGui(bool windowIsOpen, uint32_t &currentDockId)
{
	ImFont* codeFontEditor = FontManager::GetCodeFont();
	ImFont* codeFontTopBar = FontManager::GetCodeFont();

	if (showDebugPanel)
		editor->ImGuiDebugPanel("Debug " + panelName);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Begin(panelName.c_str(), &windowIsOpen,
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoSavedSettings |
		(editor->GetUndoIndex() != undoIndexInDisk ? ImGuiWindowFlags_UnsavedDocument : 0x0));
	ImGui::PopStyleVar();

	if (ImGui::IsWindowFocused() && onFocusedCallback != nullptr)
		onFocusedCallback(this->createdFromFolderView);

	currentDockId = ImGui::GetWindowDockID();

	bool isFocused = ImGui::IsWindowFocused();
	bool requestingGoToLinePopup = false;
	bool requestingFindPopup = false;
	bool requestingReplacePopup = false;
	bool requestingFontSizeIncrease = false;
	bool requestingFontSizeDecrease = false;
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			/*if (hasAssociatedFile && ImGui::MenuItem("Reload", "Ctrl+R"))
				OnReloadCommand();*/
			/*if (ImGui::MenuItem("Load from"))
				OnLoadFromCommand();*/
			if (ImGui::MenuItem("Save", "Ctrl+S"))
				OnSaveCommand();
			/*if (this->hasAssociatedFile && ImGui::MenuItem("Show in file explorer"))
				Utils::ShowInFileExplorer(this->associatedFile);
			if (this->hasAssociatedFile &&
				this->onShowInFolderViewCallback != nullptr &&
				this->createdFromFolderView > -1 && ImGui::MenuItem("Show in folder view"))
				this->onShowInFolderViewCallback(this->associatedFile, this->createdFromFolderView);;*/
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			bool ro = editor->IsReadOnlyEnabled();
			if (ro)
				ImGui::MenuItem("Read only mode", nullptr, false);

			bool ai = editor->IsAutoIndentEnabled();
			if (ImGui::MenuItem("Auto indent on enter enabled", nullptr, &ai))
				editor->SetAutoIndentEnabled(ai);
			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr, !ro && editor->CanUndo()))
				editor->Undo(true);
			if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", nullptr, !ro && editor->CanRedo()))
				editor->Redo(true);

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, editor->AnyCursorHasSelection()))
				editor->Copy();
			if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !ro && editor->AnyCursorHasSelection()))
				editor->Cut();
			if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
				editor->Paste();

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				editor->SelectAll();

			ImGui::Separator();

			if (ImGui::MenuItem("Auto format", "Ctrl+Alt+F"))
				pushEditorRequest(EDITOR_REQ_AUTO_FORMAT);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			//ImGui::SliderInt("Font size", &codeFontSize, FontManager::GetMinCodeFontSize(), FontManager::GetMaxCodeFontSize());
			ImGui::SliderInt("Tab size", &tabSize, 1, 8);
			ImGui::SliderFloat("Line spacing", &lineSpacing, 1.0f, 2.0f);
			editor->SetTabSize(tabSize);
			editor->SetLineSpacing(lineSpacing);
			static bool showSpaces = editor->IsShowWhitespacesEnabled();
			if (ImGui::MenuItem("Show spaces", nullptr, &showSpaces))
				editor->SetShowWhitespacesEnabled(!(editor->IsShowWhitespacesEnabled()));
			static bool showLineNumbers = editor->IsShowLineNumbersEnabled();
			if (ImGui::MenuItem("Show line numbers", nullptr, &showLineNumbers))
				editor->SetShowLineNumbersEnabled(!(editor->IsShowLineNumbersEnabled()));
			static bool showShortTabs = editor->IsShortTabsEnabled();
			if (ImGui::MenuItem("Short tabs", nullptr, &showShortTabs))
				editor->SetShortTabsEnabled(!(editor->IsShortTabsEnabled()));
			if (ImGui::BeginMenu("Language"))
			{
				for (int i = (int)TextEditor::LanguageDefinitionId::None; i < (int)TextEditor::LanguageDefinitionId::LanguageDefinitionCount; i++)
				{
					bool isSelected = i == (int)editor->GetLanguageDefinition();
					if (ImGui::MenuItem(languageDefinitionToName[(TextEditor::LanguageDefinitionId)i], nullptr, &isSelected))
						editor->SetLanguageDefinition((TextEditor::LanguageDefinitionId)i);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Color scheme"))
			{
				for (int i = (int)TextEditor::PaletteId::Dark; i <= (int)TextEditor::PaletteId::RetroBlue; i++)
				{
					bool isSelected = i == (int)editor->GetPalette();
					if (ImGui::MenuItem(colorPaletteToName[(TextEditor::PaletteId)i], nullptr, &isSelected))
						editor->SetPalette((TextEditor::PaletteId)i);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Find"))
		{
			if (ImGui::MenuItem("Go to line", "Ctrl+G"))
				requestingGoToLinePopup = true;
			if (ImGui::MenuItem("Find", "Ctrl+F"))
				requestingFindPopup = true;
			if (ImGui::MenuItem("Replace", "Ctrl+H"))
				requestingReplacePopup = true;
			ImGui::EndMenu();
		}

		int line, column;
		editor->GetCursorPosition(line, column);

		if (codeFontTopBar != nullptr) ImGui::PushFont(codeFontTopBar);
		ImGui::Text("%6d/%-6d %6d lines | %s | %s", line + 1, column + 1, editor->GetLineCount(),
			editor->IsOverwriteEnabled() ? "Ovr" : "Ins",
			editor->GetLanguageDefinitionName());
		if (codeFontTopBar != nullptr) ImGui::PopFont();

		ImGui::EndMenuBar();
	}

	if (codeFontEditor != nullptr) ImGui::PushFont(codeFontEditor);
	isFocused |= editor->Render("TextEditor", isFocused);
	if (codeFontEditor != nullptr) ImGui::PopFont();

	if (isFocused)
	{
		bool ctrlPressed = ImGui::GetIO().KeyCtrl;
		bool shiftPressed = ImGui::GetIO().KeyShift;
		bool altPressed = ImGui::GetIO().KeyAlt;
		if (ctrlPressed && !shiftPressed && !altPressed)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_S, false))
				OnSaveCommand();
			if (ImGui::IsKeyPressed(ImGuiKey_R, false))
				OnReloadCommand();
			if (ImGui::IsKeyDown(ImGuiKey_G))
				requestingGoToLinePopup = true;
			if (ImGui::IsKeyDown(ImGuiKey_F))
				requestingFindPopup = true;
			if (ImGui::IsKeyDown(ImGuiKey_H))
				requestingReplacePopup = true;
			if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::GetIO().MouseWheel > 0.0f)
				requestingFontSizeIncrease = true;
			if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::GetIO().MouseWheel < 0.0f)
				requestingFontSizeDecrease = true;
		}

		if (ctrlPressed && !shiftPressed && altPressed)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_F, false))
				pushEditorRequest(EDITOR_REQ_AUTO_FORMAT);
		}
	}

	if (requestingGoToLinePopup) ImGui::OpenPopup("go_to_line_popup");
	if (ImGui::BeginPopup("go_to_line_popup"))
	{
		static int targetLine;
		ImGui::SetKeyboardFocusHere();
		ImGui::InputInt("Line", &targetLine);
		if (ImGui::IsKeyDown(ImGuiKey_Enter) || ImGui::IsKeyDown(ImGuiKey_KeypadEnter))
		{
			static int targetLineFixed;
			targetLineFixed = targetLine < 1 ? 0 : targetLine - 1;
			editor->ClearExtraCursors();
			editor->ClearSelections();
			editor->SelectLine(targetLineFixed);
			CenterViewAtLine(targetLineFixed);
			ImGui::CloseCurrentPopup();
		//	ClearInputKeys(&ImGui::GetIO());
		}
		else if (ImGui::IsKeyDown(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (requestingFindPopup)
	{
		ImGui::OpenPopup("code_editor_find_popup");
		editor->GetSuitableTextToFind(ctrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH);
	}

	if (ImGui::BeginPopup("code_editor_find_popup"))
	{
		ImGui::Checkbox("Case sensitive", &ctrlfCaseSensitive);
		ImGui::Checkbox("Whole words", &ctrlfWholeWords);
		if (requestingFindPopup)
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("Search for", ctrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
		int toFindTextSize = strlen(ctrlfTextToFind);
		if ((ImGui::Button("Find next") || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0)
		{
			editor->ClearExtraCursors();
			if (editor->SelectNextOccurrenceOf(ctrlfTextToFind, toFindTextSize, ctrlfCaseSensitive, ctrlfWholeWords))
			{
				int nextOccurrenceLine, _;
				editor->GetCursorPosition(nextOccurrenceLine, _);
				CenterViewAtLine(nextOccurrenceLine);
			}
		}

		if (ImGui::IsKeyDown(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	if (requestingReplacePopup)
	{
		ImGui::OpenPopup("code_editor_replace_popup");
		editor->GetSuitableTextToFind(ctrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH);
	}

	if (ImGui::BeginPopup("code_editor_replace_popup"))
	{
		ImGui::Checkbox("Case sensitive", &ctrlfCaseSensitive);
		ImGui::Checkbox("Whole words", &ctrlfWholeWords);
		if (requestingFindPopup)
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("Search for", ctrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
		ImGui::InputText("Replace with", ctrlfTextToReplace, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
		int toFindTextSize = strlen(ctrlfTextToFind);
		int toReplaceTextSize = strlen(ctrlfTextToReplace);
		if ((ImGui::Button("Replace") || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0)
		{
			editor->ClearExtraCursors();
			if (editor->Replace(ctrlfTextToFind, toFindTextSize, ctrlfTextToReplace, toReplaceTextSize, ctrlfCaseSensitive, ctrlfWholeWords, false))
			{
				int nextOccurrenceLine, _;
				editor->GetCursorPosition(nextOccurrenceLine, _);
				CenterViewAtLine(nextOccurrenceLine);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Replace all") && toFindTextSize > 0)
		{
			if (editor->Replace(ctrlfTextToFind, toFindTextSize, ctrlfTextToReplace, toReplaceTextSize, ctrlfCaseSensitive, ctrlfWholeWords, true))
			{
				int nextOccurrenceLine, _;
				editor->GetCursorPosition(nextOccurrenceLine, _);
				CenterViewAtLine(nextOccurrenceLine);
			}
		}
		else if (ImGui::IsKeyDown(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	//if (requestingFontSizeIncrease && codeFontSize < FontManager::GetMaxCodeFontSize())
	//	codeFontSize++;
	//if (requestingFontSizeDecrease && codeFontSize > FontManager::GetMinCodeFontSize())
	//	codeFontSize--;

	ImGui::End();

	return windowIsOpen;
}


void FileTextEdit::SetCursorPosition(int line, int column)
{
	editor->SetCursorPosition(line, column);
	CenterViewAtLine(line);
}

void FileTextEdit::SetSelection(int startLine, int startChar, int endLine, int endChar)
{
	editor->SetCursorPosition(endLine, endChar);
	editor->SelectRegion(startLine, startChar, endLine, endChar);
}

void FileTextEdit::CenterViewAtLine(int line)
{
	editor->SetViewAtLine(line, TextEditor::SetViewAtLineMode::Centered);
}

const char* FileTextEdit::GetAssociatedFile()
{
	if (!hasAssociatedFile)
		return nullptr;
	return associatedFile.c_str();
}

void FileTextEdit::OnFolderViewDeleted(int folderViewId)
{
	if (createdFromFolderView == folderViewId)
		createdFromFolderView = -1;
}

void FileTextEdit::SetShowDebugPanel(bool value)
{
	showDebugPanel = value;
}

// Commands

void FileTextEdit::OnReloadCommand()
{
/*	eastl::ifstream t(Utf8ToWstring(associatedFile));
	eastl::string str((eastl::istreambuf_iterator<char>(t)),
		eastl::istreambuf_iterator<char>());
	editor->SetText(str);
	undoIndexInDisk = 0; */
}

void FileTextEdit::OnLoadFromCommand()
{
/*	eastl::vector<eastl::string> selection = pfd::open_file("Open file", "", { "Any file", "*" }).result();
	if (selection.size() == 0)
		eastl::cout << "File not loaded\n";
	else
	{
		eastl::ifstream t(Utf8ToWstring(selection[0]));
		eastl::string str((eastl::istreambuf_iterator<char>(t)),
			eastl::istreambuf_iterator<char>());
		editor->SetText(str);
		auto pathObject = eastl::filesystem::path(selection[0]);
		auto lang = extensionToLanguageDefinition.find(pathObject.extension().string());
		if (lang != extensionToLanguageDefinition.end())
			editor->SetLanguageDefinition(extensionToLanguageDefinition[pathObject.extension().string()]);
	}
	undoIndexInDisk = -1; // assume they are loading text from some other file*/
}

void FileTextEdit::OnSaveCommand()
{
	pushEditorRequest(EDITOR_REQ_SAVE);
	undoIndexInDisk = editor->GetUndoIndex();
/*	eastl::string textToSave = editor->GetText();
	eastl::string destination = hasAssociatedFile ?
		associatedFile :
		pfd::save_file("Save file", "", { "Any file", "*" }).result();
	if (destination.length() > 0)
	{
		associatedFile = destination;
		hasAssociatedFile = true;
		panelName = eastl::filesystem::path(destination).filename().string() + "##" + eastl::to_string((int)this);
		eastl::ofstream outFile(Utf8ToWstring(destination), eastl::ios::binary);
		outFile << textToSave;
		outFile.close();
	}
	*/
}
