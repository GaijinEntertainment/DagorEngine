#pragma once

#include "TextEditor.h"
#include <EASTL/algorithm.h>

#define FIND_POPUP_TEXT_FIELD_LENGTH 400

enum ImguiTextEditorRequests
{
	EDITOR_REQ_NONE = 0,
	EDITOR_REQ_SAVE = 1,
	EDITOR_REQ_AUTO_FORMAT = 2,
	EDITOR_REQ_CHANGED = 3,
};

struct FileTextEdit
{
	typedef void (*OnFocusedCallback)(int folderViewId);
	typedef void (*OnShowInFolderViewCallback)(const eastl::string& filePath, int folderViewId);

	FileTextEdit(const char* filePath = nullptr,
		int id = -1,
		int createdFromFolderView = -1,
		OnFocusedCallback onFocusedCallback = nullptr,
		OnShowInFolderViewCallback onShowInFolderViewCallback = nullptr);
	~FileTextEdit();
	bool OnImGui(bool windowIsOpen, uint32_t &currentDockId);
	void SetCursorPosition(int line, int column);
	void SetSelection(int startLine, int startChar, int endLine, int endChar);
	void CenterViewAtLine(int line);
	const char* GetAssociatedFile();
	void OnFolderViewDeleted(int folderViewId);
	void SetShowDebugPanel(bool value);
	void SetText(const char* text);
	const eastl::string GetText();

	static inline void SetDarkPaletteAsDefault() { TextEditor::SetDefaultPalette(TextEditor::PaletteId::Dark); }
	static inline void SetLightPaletteAsDefault() { TextEditor::SetDefaultPalette(TextEditor::PaletteId::Light); }

	ImguiTextEditorRequests pullEditorRequest()
	{
		if (editorRequests.empty())
			return EDITOR_REQ_NONE;
		else
		{
			ImguiTextEditorRequests req = editorRequests.front();
			editorRequests.erase(editorRequests.begin(), editorRequests.begin() + 1);
			return req;
		}
	}

	void pushEditorRequest(ImguiTextEditorRequests request)
	{
		if (eastl::find(editorRequests.begin(), editorRequests.end(), request) == editorRequests.end())
			editorRequests.push_back(request);
	}

	TextEditor* getEditor() { return editor; }
	eastl::string getAssociatedFile() { return associatedFile; }

private:

	// Commands
	void OnReloadCommand();
	void OnLoadFromCommand();
	void OnSaveCommand();

	void GetSuitableTextToFind(char * result, int max_size);

	int id = -1;
	int createdFromFolderView = -1;
	eastl::vector<ImguiTextEditorRequests> editorRequests;

	OnFocusedCallback onFocusedCallback = nullptr;
	OnShowInFolderViewCallback onShowInFolderViewCallback = nullptr;

	TextEditor* editor = nullptr;
	bool showDebugPanel = false;
	bool hasAssociatedFile = false;
	eastl::string panelName;
	eastl::string associatedFile;
	int tabSize = 4;
	float lineSpacing = 1.0f;
	int undoIndexInDisk = 0;
	int codeFontSize;

	char ctrlfTextToFind[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
	char ctrlfTextToReplace[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
	bool ctrlfCaseSensitive = false;
	bool ctrlfWholeWords = false;

	static eastl::unordered_map<eastl::string, TextEditor::LanguageDefinitionId> extensionToLanguageDefinition;
	static eastl::unordered_map<TextEditor::LanguageDefinitionId, const char*> languageDefinitionToName;
	static eastl::unordered_map<TextEditor::PaletteId, const char*> colorPaletteToName;
};
