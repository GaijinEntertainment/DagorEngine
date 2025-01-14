#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/set.h>
#include <math.h>

#include "TextEditor.h"
#include "TextEditorFuzzer.h"

#define IMGUI_SCROLLBAR_WIDTH 14.0f
#define POS_TO_COORDS_COLUMN_OFFSET 0.33f
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h" // for imGui::GetCurrentWindow()

// --------------------------------------- //
// ------------- Exposed API ------------- //

#define HIGHLIGHT_MAX_FILE_LINES 30000
#define HIGHLIGHT_MAX_LINE_LENGTH 4000
#define MAX_HIGHLIGHTS_PER_LINE 20
#define MAX_HIGHLIGHTS_PER_FILE 400
#define HIGHLIGHT_FIRST_PASS_LINES_DISTANCE 30


TextEditor::TextEditor()
{
	SetPalette(defaultPalette);
	mLines.push_back(Line());
	tabString = eastl::string(mTabSize, ' ');
}

TextEditor::~TextEditor()
{
	ClearHighlights();
}

void TextEditor::SetPalette(PaletteId aValue)
{
	mPaletteId = aValue;
	const Palette* palletteBase;
	switch (mPaletteId)
	{
	case PaletteId::Dark:
		palletteBase = &(GetDarkPalette());
		break;
	case PaletteId::Light:
		palletteBase = &(GetLightPalette());
		break;
	case PaletteId::Mariana:
		palletteBase = &(GetMarianaPalette());
		break;
	case PaletteId::RetroBlue:
		palletteBase = &(GetRetroBluePalette());
		break;
	}
	/* Update palette with the current alpha from style */
	for (int i = 0; i < (int)PaletteIndex::Max; ++i)
	{
		ImVec4 color = U32ColorToVec4((*palletteBase)[i]);
		color.w *= ImGui::GetStyle().Alpha;
		mPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}
}

void TextEditor::SetLanguageDefinition(LanguageDefinitionId aValue)
{
	mLanguageDefinitionId = aValue;
	switch (mLanguageDefinitionId)
	{
	case LanguageDefinitionId::Cpp:
		mLanguageDefinition = &(LanguageDefinition::Cpp());
		break;
	case LanguageDefinitionId::Daslang:
		mLanguageDefinition = &(LanguageDefinition::Daslang());
		break;
	default:
		mLanguageDefinition = nullptr;
		return;
	}

	ColorizeAll();
}

const char* TextEditor::GetLanguageDefinitionName() const
{
	return mLanguageDefinition != nullptr ? mLanguageDefinition->mName.c_str() : "None";
}

void TextEditor::SetTabSize(int aValue)
{
	mTabSize = Max(1, Min(8, aValue));
	tabString = eastl::string(mTabSize, ' ');
}

void TextEditor::SetLineSpacing(float aValue)
{
	mLineSpacing = Max(1.0f, Min(2.0f, aValue));
}

void TextEditor::SelectAll()
{
	ClearSelections();
	ClearExtraCursors();
	MoveTop();
	MoveBottom(true);
}

void TextEditor::SelectLine(int aLine)
{
	ClearSelections();
	ClearExtraCursors();
	SetSelection({ aLine, 0 }, { aLine, GetLineMaxColumn(aLine) });
}

void TextEditor::SelectRegion(int aStartLine, int aStartChar, int aEndLine, int aEndChar)
{
	ClearSelections();
	ClearExtraCursors();
	SetSelection(aStartLine, aStartChar, aEndLine, aEndChar);
}

bool TextEditor::SelectNextOccurrenceOf(const char* aText, int aTextSize, bool aCaseSensitive, bool aWholeWords)
{
	ClearSelections();
	ClearExtraCursors();
	return SelectNextOccurrenceOf(aText, aTextSize, -1, aCaseSensitive, aWholeWords);
}

void TextEditor::SelectAllOccurrencesOf(const char* aText, int aTextSize, bool aCaseSensitive, bool aWholeWords)
{
	ClearSelections();
	ClearExtraCursors();
	SelectNextOccurrenceOf(aText, aTextSize, -1, aCaseSensitive, aWholeWords);
	Coordinates startPos = mState.mCursors[mState.GetLastAddedCursorIndex()].mInteractiveEnd;
	while (true)
	{
		AddCursorForNextOccurrence(aCaseSensitive, aWholeWords);
		Coordinates lastAddedPos = mState.mCursors[mState.GetLastAddedCursorIndex()].mInteractiveEnd;
		if (lastAddedPos == startPos)
			break;
	}
}

bool TextEditor::AnyCursorHasSelection() const
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
		if (mState.mCursors[c].HasSelection())
			return true;
	return false;
}

bool TextEditor::AllCursorsHaveSelection() const
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
		if (!mState.mCursors[c].HasSelection())
			return false;
	return true;
}

void TextEditor::ClearExtraCursors()
{
	mState.mCurrentCursor = 0;
}

void TextEditor::ClearSelections()
{
	for (int c = mState.mCurrentCursor; c > -1; c--)
		mState.mCursors[c].mInteractiveEnd =
		mState.mCursors[c].mInteractiveStart =
		mState.mCursors[c].GetSelectionEnd();

	ClearHighlights();
}

void TextEditor::SetCursorPosition(int aLine, int aCharIndex)
{
	SetCursorPosition({ aLine, GetCharacterColumn(aLine, aCharIndex) }, -1, true);
}

int TextEditor::GetFirstVisibleLine()
{
	return mFirstVisibleLine;
}

int TextEditor::GetLastVisibleLine()
{
	return mLastVisibleLine;
}

void TextEditor::SetViewAtLine(int aLine, SetViewAtLineMode aMode)
{
	mSetViewAtLine = aLine;
	mSetViewAtLineMode = aMode;
}

void TextEditor::Copy()
{
	if (AnyCursorHasSelection())
	{
		eastl::string clipboardText = GetClipboardText();
		ImGui::SetClipboardText(clipboardText.c_str());
	}
	else
	{
		if (!mLines.empty())
		{
			eastl::string str;
			auto& line = mLines[GetActualCursorCoordinates().mLine];
			for (auto& g : line)
				str.push_back(g.mChar);
#ifdef _WIN32
			str.push_back('\r');
#endif
			str.push_back('\n');
			ImGui::SetClipboardText(str.c_str());
		}
	}
}

void TextEditor::Cut()
{
	if (mReadOnly)
	{
		Copy();
	}
	else
	{
		if (AnyCursorHasSelection())
		{
			UndoRecord u;
			u.mBefore = mState;

			Copy();
			for (int c = mState.mCurrentCursor; c > -1; c--)
			{
				u.mOperations.push_back({ GetSelectedText(c), mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd(), UndoOperationType::Delete });
				DeleteSelection(c);
			}

			u.mAfter = mState;
			AddUndo(u);
		}
	}
}

void TextEditor::Paste()
{
	const char* text = ImGui::GetClipboardText();
	if (!text || !text[0])
		return;

	if (!mLines.empty() && !AnyCursorHasSelection())
	{
		auto &line = mLines[GetActualCursorCoordinates().mLine];
		int lineLen = (int)strlen(text);
		if (lineLen > 0 && text[lineLen - 1] == '\n')
			lineLen--;
		if (lineLen > 0 && text[lineLen - 1] == '\r')
			lineLen--;

		if (bool sameLine = (lineLen == line.size() && lineLen > 0))
		{
			for (int i = 0; i < lineLen; i++)
				if (line[i].mChar != text[i])
				{
					sameLine = false;
					break;
				}

			if (sameLine)
			{
				eastl::string textToInsert = "\n";
				textToInsert += text;
				if (textToInsert.back() == '\n')
					textToInsert.pop_back();

				auto coords = GetActualCursorCoordinates();
				auto newCursorPos = coords;

				coords.mColumn = mLines[coords.mLine].size();
				SetCursorPosition(coords, 0);
				PasteText(textToInsert.c_str());

				newCursorPos.mLine++;
				SetCursorPosition(newCursorPos, 0, true);
				return;
			}
		}
	}

	PasteText(ImGui::GetClipboardText());
}

void TextEditor::PasteText(const char * text)
{
	if (mReadOnly)
		return;

	// check if we should do multicursor paste
	if (!text)
		return;

	eastl::string clipText = text;
	bool canPasteToMultipleCursors = false;
	eastl::vector<eastl::pair<int, int>> clipTextLines;
	if (mState.mCurrentCursor > 0)
	{
		clipTextLines.push_back({ 0,0 });
		for (int i = 0; i < clipText.length(); i++)
		{
			if (clipText[i] == '\n')
			{
				clipTextLines.back().second = i;
				clipTextLines.push_back({ i + 1, 0 });
			}
		}
		clipTextLines.back().second = clipText.length();
		canPasteToMultipleCursors = clipTextLines.size() == mState.mCurrentCursor + 1;
	}

	UndoRecord u;
	u.mBefore = mState;

	if (AnyCursorHasSelection())
	{
		for (int c = mState.mCurrentCursor; c > -1; c--)
		{
			u.mOperations.push_back({ GetSelectedText(c), mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
	}

	if (clipText.length() > 0)
	{
		for (int c = mState.mCurrentCursor; c > -1; c--)
		{
			Coordinates start = GetActualCursorCoordinates(c);
			if (canPasteToMultipleCursors)
			{
				eastl::string clipSubText = clipText.substr(clipTextLines[c].first, clipTextLines[c].second - clipTextLines[c].first);
				InsertTextAtCursor(clipSubText, c);
				u.mOperations.push_back({ clipSubText, start, GetActualCursorCoordinates(c), UndoOperationType::Add });
			}
			else
			{
				InsertTextAtCursor(clipText, c);
				u.mOperations.push_back({ clipText, start, GetActualCursorCoordinates(c), UndoOperationType::Add });
			}
		}
	}

	u.mAfter = mState;
	AddUndo(u);
}

void TextEditor::ClearHighlights()
{
	for (auto &h : highlights)
	{
		delete h;
		h = nullptr;
	}
}

void TextEditor::AddHighlight(int line, int start, int end)
{
	if (start > HIGHLIGHT_MAX_LINE_LENGTH || end > HIGHLIGHT_MAX_LINE_LENGTH)
		return;

	if (line >= (int)highlights.size())
	{
		for (int i = (int)highlights.size(); i <= line; i++)
			highlights.push_back(nullptr);
	}

	if (highlights[line] == nullptr)
		highlights[line] = new HighlightRanges();

	if (highlights[line]->size() > MAX_HIGHLIGHTS_PER_LINE)
		return;

	if (!highlights[line]->empty())
	{
		auto &back = highlights[line]->back();
		if (back.second == start)
		{
			back.second = end;
			return;
		}
	}

	highlights[line]->push_back({ start, end });
}


bool TextEditor::UndoRecord::isSimilarTo(const TextEditor::UndoRecord& other) const
{
	if (mOperations.size() != 1 || other.mOperations.size() != 1)
		return false;

	const auto & a = mOperations[0];
	const auto & b = other.mOperations[0];

	if (a.mType != b.mType)
		return false;

	if (a.mText.length() != b.mText.length())
		return false;

	if (a.mStart.mLine != b.mStart.mLine || a.mEnd.mLine != b.mEnd.mLine || a.mStart.mLine != a.mEnd.mLine)
		return false;

	if (abs(a.mStart.mColumn - b.mStart.mColumn) > 12)
		return false;

	return true;
}

void TextEditor::Undo(bool group)
{
	if (!CanUndo())
		return;

	mUndoBuffer[--mUndoIndex].Undo(this);
	if (group)
	{
		while (CanUndo() && mUndoBuffer[mUndoIndex - 1].isSimilarTo(mUndoBuffer[mUndoIndex]))
			mUndoBuffer[--mUndoIndex].Undo(this);
	}

	CancelCloseChars();
}

void TextEditor::Redo(bool group)
{
	if (!CanRedo())
		return;

	int redoLen = 1;
	if (group)
	{
		for (; mUndoIndex + redoLen < (int)mUndoBuffer.size(); redoLen++)
			if (!mUndoBuffer[mUndoIndex].isSimilarTo(mUndoBuffer[mUndoIndex + redoLen]))
				break;
	}

	while (redoLen--)
		mUndoBuffer[mUndoIndex++].Redo(this);

	CancelCloseChars();
}

void TextEditor::SetText(const eastl::string& aText)
{
	mLines.clear();
	mLines.emplace_back(Line());
	for (auto chr : aText)
	{
		if (chr == '\r')
			continue;

		if (chr == '\n')
			mLines.emplace_back(Line());
		else
		{
			mLines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
		}
	}

//	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	ColorizeAll();
	CancelCloseChars();
}

eastl::string TextEditor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
	eastl::string result;

	if (aEnd <= aStart)
		return result;

	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndexR(aStart);
	auto iend = GetCharacterIndexR(aEnd);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += mLines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)mLines.size())
			break;

		auto& line = mLines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].mChar;
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}

void TextEditor::SetTextLines(const eastl::vector<eastl::string>& aLines)
{
	mLines.clear();

	if (aLines.empty())
		mLines.emplace_back(Line());
	else
	{
		mLines.resize(aLines.size());

		for (size_t i = 0; i < aLines.size(); ++i)
		{
			const eastl::string& aLine = aLines[i];

			mLines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
				mLines[i].emplace_back(Glyph(aLine[j], PaletteIndex::Default));
		}
	}

//	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	ColorizeAll();
}

eastl::vector<eastl::string> TextEditor::GetTextLines() const
{
	eastl::vector<eastl::string> result;

	result.reserve(mLines.size());

	for (auto& line : mLines)
	{
		eastl::string text;

		text.resize(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text[i] = line[i].mChar;

		result.emplace_back(eastl::move(text));
	}

	return result;
}

bool TextEditor::Render(const char* aTitle, bool aParentIsFocused, const ImVec2& aSize, bool aBorder)
{
	if (mCursorPositionChanged)
	{
		mCursorPositionChanged = false;
		OnCursorPositionChanged();
		ClearHighlights();
		HighlightSelectedText();
	}

	if (colorizeTime != 0.0 && ImGui::GetTime() > colorizeTime)
	{
		ColorizeAll();
		ClearHighlights();
		HighlightSelectedText();
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(mPalette[(int)PaletteIndex::Background]));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	ImGui::BeginChild(aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs);

	bool isFocused = ImGui::IsWindowFocused();
	HandleKeyboardInputs(aParentIsFocused);
	HandleMouseInputs();
	Render(aParentIsFocused);

	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	return isFocused;
}

// ------------------------------------ //
// ---------- Generic utils ----------- //

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(char c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

// "Borrowed" from ImGui source
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}

static inline bool CharIsWordChar(char ch)
{
	int sizeInBytes = UTF8CharLength(ch);
	return sizeInBytes > 1 ||
		ch >= 'a' && ch <= 'z' ||
		ch >= 'A' && ch <= 'Z' ||
		ch >= '0' && ch <= '9' ||
		ch == '_';
}

// ------------------------------------ //
// ------------- Internal ------------- //


// ---------- Editor state functions --------- //

void TextEditor::EditorState::AddCursor()
{
	// vector is never resized to smaller size, mCurrentCursor points to last available cursor in vector
	mCurrentCursor++;
	mCursors.resize(mCurrentCursor + 1);
	mLastAddedCursor = mCurrentCursor;
}

int TextEditor::EditorState::GetLastAddedCursorIndex()
{
	return mLastAddedCursor > mCurrentCursor ? 0 : mLastAddedCursor;
}

void TextEditor::EditorState::SortCursorsFromTopToBottom()
{
	Coordinates lastAddedCursorPos = mCursors[GetLastAddedCursorIndex()].mInteractiveEnd;
	eastl::sort(mCursors.begin(), mCursors.begin() + (mCurrentCursor + 1), [](const Cursor& a, const Cursor& b) -> bool
		{
			return a.GetSelectionStart() < b.GetSelectionStart();
		});
	// update last added cursor index to be valid after sort
	for (int c = mCurrentCursor; c > -1; c--)
		if (mCursors[c].mInteractiveEnd == lastAddedCursorPos)
			mLastAddedCursor = c;
}

// ---------- Undo record functions --------- //

TextEditor::UndoRecord::UndoRecord(const eastl::vector<UndoOperation>& aOperations,
	TextEditor::EditorState& aBefore, TextEditor::EditorState& aAfter)
{
	mOperations = aOperations;
	mBefore = aBefore;
	mAfter = aAfter;
	for (const UndoOperation& o : mOperations)
		EASTL_ASSERT(o.mStart <= o.mEnd);
}

void TextEditor::UndoRecord::Undo(TextEditor* aEditor)
{
	for (int i = mOperations.size() - 1; i > -1; i--)
	{
		const UndoOperation& operation = mOperations[i];
		if (!operation.mText.empty())
		{
			switch (operation.mType)
			{
			case UndoOperationType::Delete:
			{
				auto start = operation.mStart;
				aEditor->InsertTextAt(start, operation.mText.c_str());
				aEditor->ColorizeLine(operation.mStart.mLine);
				break;
			}
			case UndoOperationType::Add:
			{
				aEditor->DeleteRange(operation.mStart, operation.mEnd);
				aEditor->ColorizeLine(operation.mStart.mLine);
				break;
			}
			}
		}
	}

	aEditor->mState = mBefore;
	aEditor->EnsureCursorVisible();
}

void TextEditor::UndoRecord::Redo(TextEditor* aEditor)
{
	for (int i = 0; i < mOperations.size(); i++)
	{
		const UndoOperation& operation = mOperations[i];
		if (!operation.mText.empty())
		{
			switch (operation.mType)
			{
			case UndoOperationType::Delete:
			{
				aEditor->DeleteRange(operation.mStart, operation.mEnd);
				aEditor->ColorizeLine(operation.mStart.mLine);
				break;
			}
			case UndoOperationType::Add:
			{
				auto start = operation.mStart;
				aEditor->InsertTextAt(start, operation.mText.c_str());
				aEditor->ColorizeLine(operation.mStart.mLine);
				break;
			}
			}
		}
	}

	aEditor->mState = mAfter;
	aEditor->EnsureCursorVisible();
}


void TextEditor::GetSuitableTextToFind(char * result, int max_size)
{
	max_size--; // '\0' terminator

	eastl::string s;
	if (mState.mCursors[0].GetSelectionStart() < mState.mCursors[0].GetSelectionEnd() &&
		mState.mCursors[0].GetSelectionStart().mLine == mState.mCursors[0].GetSelectionEnd().mLine)
	{
		s = GetText(mState.mCursors[0].GetSelectionStart(), mState.mCursors[0].GetSelectionEnd());
	}
	else
	{
		auto start = FindWordStart(mState.mCursors[0].GetSelectionEnd());
		auto end = FindWordEnd(mState.mCursors[0].GetSelectionEnd());
		if (start < end)
			s = GetText(start, end);
	}

	if (!s.empty())
	{
		strncpy(result, s.c_str(), max_size);
		result[max_size] = '\0';
	}
}

// ---------- Text editor internal functions --------- //

eastl::string TextEditor::GetText() const
{
	auto lastLine = (int)mLines.size() - 1;
	auto lastLineLength = GetLineMaxColumn(lastLine);
	return GetText(Coordinates(), Coordinates(lastLine, lastLineLength));
}

eastl::string TextEditor::GetClipboardText() const
{
	eastl::string result;
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		if (mState.mCursors[c].GetSelectionStart() < mState.mCursors[c].GetSelectionEnd())
		{
			if (result.length() != 0)
				result += '\n';
			result += GetText(mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd());
		}
	}
	return result;
}

eastl::string TextEditor::GetSelectedText(int aCursor) const
{
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;

	return GetText(mState.mCursors[aCursor].GetSelectionStart(), mState.mCursors[aCursor].GetSelectionEnd());
}

void TextEditor::SetCursorPosition(const Coordinates& aPosition, int aCursor, bool aClearSelection)
{
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;

	mCursorPositionChanged = true;
	if (aClearSelection)
		mState.mCursors[aCursor].mInteractiveStart = aPosition;
	if (mState.mCursors[aCursor].mInteractiveEnd != aPosition)
	{
		mState.mCursors[aCursor].mInteractiveEnd = aPosition;
		EnsureCursorVisible();
	}

	CancelCloseChars();
}

int TextEditor::InsertTextAt(Coordinates& /* inout */ aWhere, const char* aValue)
{
	EASTL_ASSERT(!mReadOnly);

	int cindex = GetCharacterIndexR(aWhere);
	int totalLines = 0;
	while (*aValue != '\0')
	{
		EASTL_ASSERT(!mLines.empty());

		if (*aValue == '\r')
		{
			// skip
			++aValue;
		}
		else if (*aValue == '\n')
		{
			if (cindex < (int)mLines[aWhere.mLine].size())
			{
				auto& newLine = InsertLine(aWhere.mLine + 1);
				auto& line = mLines[aWhere.mLine];
				AddGlyphsToLine(aWhere.mLine + 1, 0, line.begin() + cindex, line.end());
				RemoveGlyphsFromLine(aWhere.mLine, cindex);
			}
			else
			{
				InsertLine(aWhere.mLine + 1);
			}
			++aWhere.mLine;
			aWhere.mColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		}
		else
		{
			auto& line = mLines[aWhere.mLine];
			auto d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0')
				AddGlyphToLine(aWhere.mLine, cindex++, Glyph(*aValue++, PaletteIndex::Default));
			aWhere.mColumn = GetCharacterColumn(aWhere.mLine, cindex);
		}
	}

	return totalLines;
}

void TextEditor::InsertTextAtCursor(const eastl::string& aValue, int aCursor)
{
	InsertTextAtCursor(aValue.c_str(), aCursor);
}

void TextEditor::InsertTextAtCursor(const char* aValue, int aCursor)
{
	if (aValue == nullptr)
		return;
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;

	auto pos = GetActualCursorCoordinates(aCursor);
	auto start = eastl::min(pos, mState.mCursors[aCursor].GetSelectionStart());
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetCursorPosition(pos, aCursor);
	ColorizeLine(start.mLine);
}

bool TextEditor::Move(int& aLine, int& aCharIndex, bool aLeft, bool aLockLine) const
{
	// assumes given char index is not in the middle of utf8 sequence
	// char index can be line.length()

	// invalid line
	if (aLine >= mLines.size())
		return false;

	if (aLeft)
	{
		if (aCharIndex == 0)
		{
			if (aLockLine || aLine == 0)
				return false;
			aLine--;
			aCharIndex = mLines[aLine].size();
		}
		else
		{
			aCharIndex--;
			while (aCharIndex > 0 && IsUTFSequence(mLines[aLine][aCharIndex].mChar))
				aCharIndex--;
		}
	}
	else // right
	{
		if (aCharIndex == mLines[aLine].size())
		{
			if (aLockLine || aLine == mLines.size() - 1)
				return false;
			aLine++;
			aCharIndex = 0;
		}
		else
		{
			int seqLength = UTF8CharLength(mLines[aLine][aCharIndex].mChar);
			aCharIndex = eastl::min(aCharIndex + seqLength, (int)mLines[aLine].size());
		}
	}
	return true;
}

void TextEditor::MoveCharIndexAndColumn(int aLine, int& aCharIndex, int& aColumn) const
{
	EASTL_ASSERT(aLine < mLines.size());
	EASTL_ASSERT(aCharIndex < mLines[aLine].size());
	char c = mLines[aLine][aCharIndex].mChar;
	aCharIndex += UTF8CharLength(c);
	if (c == '\t')
		aColumn = (aColumn / mTabSize) * mTabSize + mTabSize;
	else
		aColumn++;
}

void TextEditor::MoveCoords(Coordinates& aCoords, MoveDirection aDirection, bool aWordMode, int aLineCount) const
{
	int charIndex = GetCharacterIndexR(aCoords);
	int lineIndex = aCoords.mLine;
	switch (aDirection)
	{
	case MoveDirection::Right:
		if (charIndex >= mLines[lineIndex].size())
		{
			if (lineIndex < mLines.size() - 1)
			{
				aCoords.mLine = eastl::max(0, eastl::min((int)mLines.size() - 1, lineIndex + 1));
				aCoords.mColumn = 0;
			}
		}
		else
		{
			Move(lineIndex, charIndex);
			int oneStepRightColumn = GetCharacterColumn(lineIndex, charIndex);
			if (aWordMode)
			{
				aCoords = FindWordEnd(aCoords);
				aCoords.mColumn = eastl::max(aCoords.mColumn, oneStepRightColumn);
			}
			else
				aCoords.mColumn = oneStepRightColumn;
		}
		break;
	case MoveDirection::Left:
		if (charIndex == 0)
		{
			if (lineIndex > 0)
			{
				aCoords.mLine = lineIndex - 1;
				aCoords.mColumn = GetLineMaxColumn(aCoords.mLine);
			}
		}
		else
		{
			Move(lineIndex, charIndex, true);
			aCoords.mColumn = GetCharacterColumn(lineIndex, charIndex);
			if (aWordMode)
				aCoords = FindWordStart(aCoords);
		}
		break;
	case MoveDirection::Up:
		aCoords.mLine = eastl::max(0, lineIndex - aLineCount);
		break;
	case MoveDirection::Down:
		aCoords.mLine = eastl::max(0, eastl::min((int)mLines.size() - 1, lineIndex + aLineCount));
		break;
	}
}

void TextEditor::MoveUp(int aAmount, bool aSelect)
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		Coordinates newCoords = mState.mCursors[c].mInteractiveEnd;
		MoveCoords(newCoords, MoveDirection::Up, false, aAmount);
		SetCursorPosition(newCoords, c, !aSelect);
	}
	EnsureCursorVisible();
}

void TextEditor::MoveDown(int aAmount, bool aSelect)
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		EASTL_ASSERT(mState.mCursors[c].mInteractiveEnd.mColumn >= 0);
		Coordinates newCoords = mState.mCursors[c].mInteractiveEnd;
		MoveCoords(newCoords, MoveDirection::Down, false, aAmount);
		SetCursorPosition(newCoords, c, !aSelect);
	}
	EnsureCursorVisible();
}

void TextEditor::MoveLeft(bool aSelect, bool aWordMode)
{
	if (mLines.empty())
		return;

	if (AnyCursorHasSelection() && !aSelect && !aWordMode)
	{
		for (int c = 0; c <= mState.mCurrentCursor; c++)
			SetCursorPosition(mState.mCursors[c].GetSelectionStart(), c);
	}
	else
	{
		for (int c = 0; c <= mState.mCurrentCursor; c++)
		{
			Coordinates newCoords = mState.mCursors[c].mInteractiveEnd;
			MoveCoords(newCoords, MoveDirection::Left, aWordMode);
			SetCursorPosition(newCoords, c, !aSelect);
		}
	}
	EnsureCursorVisible();
}

void TextEditor::MoveRight(bool aSelect, bool aWordMode)
{
	if (mLines.empty())
		return;

	if (AnyCursorHasSelection() && !aSelect && !aWordMode)
	{
		for (int c = 0; c <= mState.mCurrentCursor; c++)
			SetCursorPosition(mState.mCursors[c].GetSelectionEnd(), c);
	}
	else
	{
		for (int c = 0; c <= mState.mCurrentCursor; c++)
		{
			Coordinates newCoords = mState.mCursors[c].mInteractiveEnd;
			MoveCoords(newCoords, MoveDirection::Right, aWordMode);
			SetCursorPosition(newCoords, c, !aSelect);
		}
	}
	EnsureCursorVisible();
}

void TextEditor::MoveTop(bool aSelect)
{
	SetCursorPosition(Coordinates(0, 0), mState.mCurrentCursor, !aSelect);
}

void TextEditor::TextEditor::MoveBottom(bool aSelect)
{
	int maxLine = (int)mLines.size() - 1;
	Coordinates newPos = Coordinates(maxLine, GetLineMaxColumn(maxLine));
	SetCursorPosition(newPos, mState.mCurrentCursor, !aSelect);
}

void TextEditor::MoveHome(bool aSelect)
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
		SetCursorPosition(Coordinates(mState.mCursors[c].mInteractiveEnd.mLine, 0), c, !aSelect);
}

void TextEditor::MoveEnd(bool aSelect)
{
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		int lindex = mState.mCursors[c].mInteractiveEnd.mLine;
		SetCursorPosition(Coordinates(lindex, GetLineMaxColumn(lindex)), c, !aSelect);
	}
}

void TextEditor::FindPreferredIndentChar(int lineNum)
{
	int from = max(lineNum - 10, 0);
	int to = min(lineNum + 10, (int)mLines.size());
	int tabs = 0;
	int spaces = 0;
	for (int i = from; i < to; i++)
	{
		auto& line = mLines[i];
		if (line.empty())
			continue;
		if (line[0].mChar == '\t')
			tabs++;
		else if (line[0].mChar == ' ')
			spaces++;
	}

	preferredIndentChar = tabs > spaces ? '\t' : ' ';
}

void TextEditor::EnterCharacter(ImWchar aChar, bool aShift)
{
	EASTL_ASSERT(!mReadOnly);

	bool hasSelection = AnyCursorHasSelection();
	bool anyCursorHasMultilineSelection = false;
	for (int c = mState.mCurrentCursor; c > -1; c--)
		if (mState.mCursors[c].GetSelectionStart().mLine != mState.mCursors[c].GetSelectionEnd().mLine)
		{
			anyCursorHasMultilineSelection = true;
			break;
		}
	bool isIndentOperation = (aChar == '\t') && (aShift || (hasSelection && anyCursorHasMultilineSelection));
	if (isIndentOperation)
	{
		ChangeCurrentLinesIndentation(!aShift);
		return;
	}

	UndoRecord u;
	u.mBefore = mState;

	if (hasSelection)
	{
		for (int c = mState.mCurrentCursor; c > -1; c--)
		{
			u.mOperations.push_back({ GetSelectedText(c), mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
	}

	char closeChar = 0;

	eastl::vector<Coordinates> coords;
	for (int c = mState.mCurrentCursor; c > -1; c--) // order important here for typing \n in the same line at the same time
	{
		auto coord = GetActualCursorCoordinates(c);
		coords.push_back(coord);
		UndoOperation added;
		added.mType = UndoOperationType::Add;
		added.mStart = coord;

		EASTL_ASSERT(!mLines.empty());

		if (mState.mCurrentCursor == 0 && (aChar == '{' || aChar == '(' || aChar == '[' || aChar == '\"' || aChar == '\''))
		{
			auto& line = mLines[coord.mLine];
			char charBefore = coord.mColumn > 0 ? line[coord.mColumn - 1].mChar : 0;
			char charAfter = coord.mColumn < (int)line.size() ? line[coord.mColumn].mChar : 0;

			bool insideString = false;
			bool insideChar = false;
			bool insideComment = false; // single line comment
			for (int i = 0; i < coord.mColumn; i++)
			{
				if (line[i].mChar == '\"' && (i == 0 || line[i - 1].mChar != '\\'))
					insideString = !insideString;
				if (line[i].mChar == '\'' && (i == 0 || line[i - 1].mChar != '\\'))
					insideChar = !insideChar;
				if (line[i].mChar == '/' && i > 0 && line[i - 1].mChar == '/')
					insideComment = true;
			}

			if ((!insideString && !insideChar && !insideComment) || aChar == '{' || aChar == '(' || aChar == '[')
				closeChar = RequiredCloseChar(charBefore, aChar, charAfter);
		}

		if (mState.mCurrentCursor == 0 && closeCharCoords != Coordinates::Invalid() && GetActualCursorCoordinates(0) == closeCharCoords)
		{
			auto& line = mLines[coord.mLine];
			char charAfter = coord.mColumn < (int)line.size() ? line[coord.mColumn].mChar : 0;
			if (charAfter == aChar)
			{
				closeCharCoords = Coordinates::Invalid();
				MoveRight(false, false);
				continue;
			}
		}

		if (aChar == '\n')
		{
			InsertLine(coord.mLine + 1);
			auto& line = mLines[coord.mLine];
			auto& newLine = mLines[coord.mLine + 1];

			added.mText = "";
			added.mText += (char)aChar;
			if (mAutoIndent)
			{
				FindPreferredIndentChar(coord.mLine);
				char lastIndentChar = preferredIndentChar;

				for (int i = 0; i < line.size() && unsigned(line[i].mChar) < 128 && isblank(line[i].mChar); ++i)
				{
					if (i >= coord.mColumn)
						break;
					newLine.push_back(line[i]);
					added.mText += line[i].mChar;
					if (line[i].mChar == '\t' || line[i].mChar == ' ')
						lastIndentChar = line[i].mChar;
				}

				eastl::string lineBeforeCursor;
				for (int i = 0; i < coord.mColumn && i < (int)line.size(); i++)
					lineBeforeCursor += line[i].mChar;

				if (RequireIndentationAfterNewLine(lineBeforeCursor))
				{
					if (lastIndentChar == ' ')
						for (int i = 0; i < mTabSize; ++i)
							newLine.push_back(Glyph(' ', PaletteIndex::Background));

					if (lastIndentChar == '\t')
						newLine.push_back(Glyph('\t', PaletteIndex::Background));
				}
			}

			const size_t whitespaceSize = newLine.size();
			auto cindex = GetCharacterIndexR(coord);
			AddGlyphsToLine(coord.mLine + 1, newLine.size(), line.begin() + cindex, line.end());
			RemoveGlyphsFromLine(coord.mLine, cindex);
			SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, (int)whitespaceSize)), c);
		}
		else
		{
			char buf[9];
			int e = ImTextCharToUtf8(buf, 7, aChar);
			if (e > 0)
			{
				buf[e] = '\0';
				auto& line = mLines[coord.mLine];
				auto cindex = GetCharacterIndexR(coord);

				if (mOverwrite && cindex < (int)line.size())
				{
					auto d = UTF8CharLength(line[cindex].mChar);

					UndoOperation removed;
					removed.mType = UndoOperationType::Delete;
					removed.mStart = mState.mCursors[c].mInteractiveEnd;
					removed.mEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + d));

					while (d-- > 0 && cindex < (int)line.size())
					{
						removed.mText += line[cindex].mChar;
						RemoveGlyphsFromLine(coord.mLine, cindex, cindex + 1);
					}
					u.mOperations.push_back(removed);
				}

				if (aChar == '\t')
				{
					int spaces = TabSizeAtColumn(coord.mColumn);
					if (unsigned(spaces) > 8)
						spaces = 8;

					memset(buf, ' ', spaces);
					buf[spaces] = '\0';
				}

				for (auto p = buf; *p != '\0'; p++, ++cindex)
					AddGlyphToLine(coord.mLine, cindex, Glyph(*p, PaletteIndex::Default));
				added.mText = buf;

				SetCursorPosition(Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex)), c);
			}
			else
				continue;
		}

		added.mEnd = GetActualCursorCoordinates(c);
		u.mOperations.push_back(added);
	}

	u.mAfter = mState;
	AddUndo(u);

	if (closeChar)
	{
		EnterCharacter(closeChar, false);
		MoveLeft(false, false);
		closeCharCoords = GetActualCursorCoordinates(0);
	}

	for (const auto& coord : coords)
		ColorizeLine(coord.mLine);
	EnsureCursorVisible();
}

void TextEditor::Backspace(bool aWordMode)
{
	EASTL_ASSERT(!mReadOnly);

	if (mLines.empty())
		return;

	if (AnyCursorHasSelection())
		Delete(aWordMode);
	else
	{
		if (!aWordMode && mState.mCurrentCursor == 0 && closeCharCoords != Coordinates::Invalid() &&
			GetActualCursorCoordinates(0) == closeCharCoords)
		{
			closeCharCoords = Coordinates::Invalid();
			MoveRight(false, false);
			Backspace(false);
			Backspace(false);
			return;
		}

		EditorState stateBeforeDeleting = mState;
		MoveLeft(true, aWordMode);
		if (!AllCursorsHaveSelection()) // can't do backspace if any cursor at {0,0}
		{
			if (AnyCursorHasSelection())
				MoveRight();
			return;
		}

		OnCursorPositionChanged(); // might combine cursors
		Delete(aWordMode, &stateBeforeDeleting);
	}
}

void TextEditor::Delete(bool aWordMode, const EditorState* aEditorState)
{
	EASTL_ASSERT(!mReadOnly);

	if (mLines.empty())
		return;

	if (AnyCursorHasSelection())
	{
		UndoRecord u;
		u.mBefore = aEditorState == nullptr ? mState : *aEditorState;
		for (int c = mState.mCurrentCursor; c > -1; c--)
		{
			if (!mState.mCursors[c].HasSelection())
				continue;
			u.mOperations.push_back({ GetSelectedText(c), mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
		u.mAfter = mState;
		AddUndo(u);
	}
	else
	{
		EditorState stateBeforeDeleting = mState;
		MoveRight(true, aWordMode);
		if (!AllCursorsHaveSelection()) // can't do delete if any cursor at end of last line
		{
			if (AnyCursorHasSelection())
				MoveLeft();
			return;
		}

		OnCursorPositionChanged(); // might combine cursors
		Delete(aWordMode, &stateBeforeDeleting);
	}
}

void TextEditor::HighlightSelectedText()
{
	eastl::string sel = GetSelectedText(0);
	bool spacesOnly = true;

	for (auto c : sel)
	{
		if (c != ' ' && c != '\t')
			spacesOnly = false;

		if (c == '\n')
			return;
	}

	if (spacesOnly)
		return;

	int highlightedCount = 0;
	int highlightAroundLine = mState.mCursors[0].mInteractiveStart.mLine;

	for (int pass = 0; pass < 2; pass++)
	{
		for (int k = 0; k < Min((int)mLines.size(), HIGHLIGHT_MAX_FILE_LINES); k++)
		{
			bool insideRange = abs(k - highlightAroundLine) < HIGHLIGHT_FIRST_PASS_LINES_DISTANCE;
			if ((pass == 0) != (insideRange))
				continue;

			auto & line = mLines[k];
			for (int i = 0; i <= (int)line.size() - (int)sel.length(); i++)
			{
				if (line[i].mChar == sel[0])
				{
					bool found = true;
					for (int j = 1; j < (int)sel.length(); j++)
					{
						if (line[i + j].mChar != sel[j])
						{
							found = false;
							break;
						}
					}

					if (found)
					{
						AddHighlight(k, i, i + (int)sel.length());
						i += (int)sel.length() - 1;
						highlightedCount++;
						if (highlightedCount > MAX_HIGHLIGHTS_PER_FILE)
							return;
					}
				}
			}
		}
	}
}

void TextEditor::SetSelection(Coordinates aStart, Coordinates aEnd, int aCursor)
{
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;

	Coordinates minCoords = Coordinates(0, 0);
	int maxLine = (int)mLines.size() - 1;
	Coordinates maxCoords = Coordinates(maxLine, GetLineMaxColumn(maxLine));
	if (aStart < minCoords)
		aStart = minCoords;
	else if (aStart > maxCoords)
		aStart = maxCoords;
	if (aEnd < minCoords)
		aEnd = minCoords;
	else if (aEnd > maxCoords)
		aEnd = maxCoords;

	mState.mCursors[aCursor].mInteractiveStart = aStart;
	SetCursorPosition(aEnd, aCursor, false);

	ClearHighlights();
	HighlightSelectedText();
}

void TextEditor::SetSelection(int aStartLine, int aStartChar, int aEndLine, int aEndChar, int aCursor)
{
	Coordinates startCoords = { aStartLine, GetCharacterColumn(aStartLine, aStartChar) };
	Coordinates endCoords = { aEndLine, GetCharacterColumn(aEndLine, aEndChar) };
	SetSelection(startCoords, endCoords, aCursor);
}

bool TextEditor::SelectNextOccurrenceOf(const char* aText, int aTextSize, int aCursor, bool aCaseSensitive, bool aWholeWords)
{
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;
	Coordinates nextStart, nextEnd;
	if (!FindNextOccurrence(aText, aTextSize, mState.mCursors[aCursor].mInteractiveEnd, nextStart, nextEnd, aCaseSensitive, aWholeWords))
		return false;
	SetSelection(nextStart, nextEnd, aCursor);
	EnsureCursorVisible(aCursor, true);
	return true;
}


bool TextEditor::Replace(const char* find, int findLength, const char * replaceWith, int replaceLength, bool aCaseSensitive, bool aWholeWords, bool all)
{
	(void)replaceLength;
	bool replaceOccurred = false;

	do
	{
		int aCursor = mState.mCurrentCursor;
		Coordinates nextStart, nextEnd;

		if (!FindNextOccurrence(find, findLength, mState.mCursors[aCursor].mInteractiveEnd, nextStart, nextEnd, aCaseSensitive, aWholeWords))
			break;

		SetSelection(nextStart, nextEnd, aCursor);
		PasteText(replaceWith);
		replaceOccurred = true;

		if (!all && nextStart != mState.mCursors[aCursor].mInteractiveEnd)
			SetSelection(nextStart, mState.mCursors[aCursor].mInteractiveEnd, aCursor);

		EnsureCursorVisible(aCursor, true);
	}
	while (all);

	return replaceOccurred;
}


void TextEditor::AddCursorForNextOccurrence(bool aCaseSensitive, bool aWholeWords)
{
	const Cursor& currentCursor = mState.mCursors[mState.GetLastAddedCursorIndex()];
	if (currentCursor.GetSelectionStart() == currentCursor.GetSelectionEnd())
		return;

	eastl::string selectionText = GetText(currentCursor.GetSelectionStart(), currentCursor.GetSelectionEnd());
	Coordinates nextStart, nextEnd;
	if (!FindNextOccurrence(selectionText.c_str(), selectionText.length(), currentCursor.GetSelectionEnd(), nextStart, nextEnd, aCaseSensitive))
		return;

	mState.AddCursor();
	SetSelection(nextStart, nextEnd, mState.mCurrentCursor);
	mState.SortCursorsFromTopToBottom();
	MergeCursorsIfPossible();
	EnsureCursorVisible(-1, true);
}

bool TextEditor::FindNextOccurrence(const char* aText, int aTextSize, const Coordinates& aFrom, Coordinates& outStart, Coordinates& outEnd, bool aCaseSensitive, bool aWholeWords)
{
	EASTL_ASSERT(aTextSize > 0);
	bool fmatches = false;
	int fline, ifline;
	int findex, ifindex;

	ifline = fline = aFrom.mLine;
	ifindex = findex = GetCharacterIndexR(aFrom);

	while (true)
	{
		bool matches;
		{ // match function
			int lineOffset = 0;
			int currentCharIndex = findex;
			int i = 0;
			for (; i < aTextSize; i++)
			{
				if (currentCharIndex == mLines[fline + lineOffset].size())
				{
					if (aText[i] == '\n' && fline + lineOffset + 1 < mLines.size())
					{
						currentCharIndex = 0;
						lineOffset++;
					}
					else
						break;
				}
				else
				{
					char toCompareA = mLines[fline + lineOffset][currentCharIndex].mChar;
					char toCompareB = aText[i];
					toCompareA = (!aCaseSensitive && toCompareA >= 'A' && toCompareA <= 'Z') ? toCompareA - 'A' + 'a' : toCompareA;
					toCompareB = (!aCaseSensitive && toCompareB >= 'A' && toCompareB <= 'Z') ? toCompareB - 'A' + 'a' : toCompareB;
					if (toCompareA != toCompareB)
						break;
					else
						currentCharIndex++;
				}
			}
			matches = i == aTextSize;
			if (matches)
			{
				if (aWholeWords)
				{
					if (findex > 0)
					{
						char charBefore = mLines[fline][findex - 1].mChar;
						if (CharIsWordChar(charBefore))
							matches = false;
					}
					if (matches && findex + aTextSize < mLines[fline].size())
					{
						char charAfter = mLines[fline][findex + aTextSize].mChar;
						if (CharIsWordChar(charAfter))
							matches = false;
					}
				}

				if (matches)
				{
					outStart = { fline, GetCharacterColumn(fline, findex) };
					outEnd = { fline + lineOffset, GetCharacterColumn(fline + lineOffset, currentCharIndex) };
					return true;
				}
			}
		}

		// move forward
		if (findex == mLines[fline].size()) // need to consider line breaks
		{
			if (fline == mLines.size() - 1)
			{
				fline = 0;
				findex = 0;
			}
			else
			{
				fline++;
				findex = 0;
			}
		}
		else
			findex++;

		// detect complete scan
		if (findex == ifindex && fline == ifline)
			return false;
	}

	return false;
}

bool TextEditor::FindMatchingBracket(int aLine, int aCharIndex, Coordinates& out)
{
	if (aLine > mLines.size() - 1)
		return false;
	int maxCharIndex = mLines[aLine].size() - 1;
	if (aCharIndex > maxCharIndex)
		return false;

	int currentLine = aLine;
	int currentCharIndex = aCharIndex;
	int counter = 1;
	if (CLOSE_TO_OPEN_CHAR.find(mLines[aLine][aCharIndex].mChar) != CLOSE_TO_OPEN_CHAR.end())
	{
		char closeChar = mLines[aLine][aCharIndex].mChar;
		char openChar = CLOSE_TO_OPEN_CHAR.at(closeChar);
		while (Move(currentLine, currentCharIndex, true))
		{
			if (currentCharIndex < mLines[currentLine].size())
			{
				char currentChar = mLines[currentLine][currentCharIndex].mChar;
				if (currentChar == openChar)
				{
					counter--;
					if (counter == 0)
					{
						out = { currentLine, GetCharacterColumn(currentLine, currentCharIndex) };
						return true;
					}
				}
				else if (currentChar == closeChar)
					counter++;
			}
		}
	}
	else if (OPEN_TO_CLOSE_CHAR.find(mLines[aLine][aCharIndex].mChar) != OPEN_TO_CLOSE_CHAR.end())
	{
		char openChar = mLines[aLine][aCharIndex].mChar;
		char closeChar = OPEN_TO_CLOSE_CHAR.at(openChar);
		while (Move(currentLine, currentCharIndex))
		{
			if (currentCharIndex < mLines[currentLine].size())
			{
				char currentChar = mLines[currentLine][currentCharIndex].mChar;
				if (currentChar == closeChar)
				{
					counter--;
					if (counter == 0)
					{
						out = { currentLine, GetCharacterColumn(currentLine, currentCharIndex) };
						return true;
					}
				}
				else if (currentChar == openChar)
					counter++;
			}
		}
	}
	return false;
}


void TextEditor::ChangeSingleLineIndentation(UndoRecord &u, int line, bool aIncrease)
{
	if (aIncrease)
	{
		if (mLines[line].size() > 0)
		{
			Coordinates lineStart = { line, 0 };
			Coordinates insertionEnd = lineStart;
			InsertTextAt(insertionEnd, tabString.c_str()); // sets insertion end
			u.mOperations.push_back({ tabString.c_str(), lineStart, insertionEnd, UndoOperationType::Add });
			ColorizeLine(lineStart.mLine);
		}
	}
	else
	{
		Coordinates start = { line, 0 };
		Coordinates end = { line, mTabSize };
		int charIndex = GetCharacterIndexL(end) - 1;
		while (charIndex > -1 && (mLines[line][charIndex].mChar == ' ' || mLines[line][charIndex].mChar == '\t')) charIndex--;
		bool onlySpaceCharactersFound = charIndex == -1;
		if (onlySpaceCharactersFound)
		{
			u.mOperations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });
			DeleteRange(start, end);
			ColorizeLine(line);
		}
	}
}


void TextEditor::ChangeCurrentLinesIndentation(bool aIncrease)
{
	EASTL_ASSERT(!mReadOnly);

	UndoRecord u;
	u.mBefore = mState;

	for (int c = mState.mCurrentCursor; c > -1; c--)
	{
		if (!aIncrease && mState.mCursors[c].GetSelectionEnd() == mState.mCursors[c].GetSelectionStart())
		{
			ChangeSingleLineIndentation(u, mState.mCursors[c].mInteractiveEnd.mLine, false);
			continue;
		}

		for (int currentLine = mState.mCursors[c].GetSelectionEnd().mLine; currentLine >= mState.mCursors[c].GetSelectionStart().mLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == mState.mCursors[c].GetSelectionEnd() && mState.mCursors[c].GetSelectionEnd() != mState.mCursors[c].GetSelectionStart()) // when selection ends at line start
				continue;

			ChangeSingleLineIndentation(u, currentLine, aIncrease);
		}
	}

	if (u.mOperations.size() > 0)
		AddUndo(u);
}

void TextEditor::MoveUpCurrentLines()
{
	EASTL_ASSERT(!mReadOnly);

	UndoRecord u;
	u.mBefore = mState;

	eastl::set<int> affectedLines;
	int minLine = -1;
	int maxLine = -1;
	for (int c = mState.mCurrentCursor; c > -1; c--) // cursors are expected to be sorted from top to bottom
	{
		for (int currentLine = mState.mCursors[c].GetSelectionEnd().mLine; currentLine >= mState.mCursors[c].GetSelectionStart().mLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == mState.mCursors[c].GetSelectionEnd() && mState.mCursors[c].GetSelectionEnd() != mState.mCursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			minLine = minLine == -1 ? currentLine : (currentLine < minLine ? currentLine : minLine);
			maxLine = maxLine == -1 ? currentLine : (currentLine > maxLine ? currentLine : maxLine);
		}
	}
	if (minLine == 0) // can't move up anymore
		return;

	Coordinates start = { minLine - 1, 0 };
	Coordinates end = { maxLine, GetLineMaxColumn(maxLine) };
	u.mOperations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });

	for (int line : affectedLines) // lines should be sorted here
		eastl::swap(mLines[line - 1], mLines[line]);
	for (int c = mState.mCurrentCursor; c > -1; c--)
	{
		mState.mCursors[c].mInteractiveStart.mLine -= 1;
		mState.mCursors[c].mInteractiveEnd.mLine -= 1;
		// no need to set mCursorPositionChanged as cursors will remain sorted
	}

	end = { maxLine, GetLineMaxColumn(maxLine) }; // this line is swapped with line above, need to find new max column
	u.mOperations.push_back({ GetText(start, end), start, end, UndoOperationType::Add });
	u.mAfter = mState;
	AddUndo(u);
}

void TextEditor::MoveDownCurrentLines()
{
	EASTL_ASSERT(!mReadOnly);

	UndoRecord u;
	u.mBefore = mState;

	eastl::set<int> affectedLines;
	int minLine = -1;
	int maxLine = -1;
	int maxSelectionLine = -1;
	for (int c = 0; c <= mState.mCurrentCursor; c++) // cursors are expected to be sorted from top to bottom
	{
		for (int currentLine = mState.mCursors[c].GetSelectionEnd().mLine; currentLine >= mState.mCursors[c].GetSelectionStart().mLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == mState.mCursors[c].GetSelectionEnd() && mState.mCursors[c].GetSelectionEnd() != mState.mCursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			minLine = minLine == -1 ? currentLine : (currentLine < minLine ? currentLine : minLine);
			maxLine = maxLine == -1 ? currentLine : (currentLine > maxLine ? currentLine : maxLine);
			maxSelectionLine = Max(maxSelectionLine, mState.mCursors[c].mInteractiveStart.mLine);
			maxSelectionLine = Max(maxSelectionLine, mState.mCursors[c].mInteractiveEnd.mLine);
		}
	}
	if (maxLine >= mLines.size() - 1 || maxSelectionLine >= mLines.size() - 1) // can't move down anymore
		return;

	Coordinates start = { minLine, 0 };
	Coordinates end = { maxLine + 1, GetLineMaxColumn(maxLine + 1)};
	u.mOperations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });

	eastl::set<int>::reverse_iterator rit;
	for (rit = affectedLines.rbegin(); rit != affectedLines.rend(); rit++) // lines should be sorted here
		eastl::swap(mLines[*rit + 1], mLines[*rit]);
	for (int c = mState.mCurrentCursor; c > -1; c--)
	{
		mState.mCursors[c].mInteractiveStart.mLine += 1;
		mState.mCursors[c].mInteractiveEnd.mLine += 1;
		// no need to set mCursorPositionChanged as cursors will remain sorted
	}

	end = { maxLine + 1, GetLineMaxColumn(maxLine + 1) }; // this line is swapped with line below, need to find new max column
	u.mOperations.push_back({ GetText(start, end), start, end, UndoOperationType::Add });
	u.mAfter = mState;
	AddUndo(u);
}

void TextEditor::ToggleLineComment()
{
	EASTL_ASSERT(!mReadOnly);
	if (mLanguageDefinition == nullptr)
		return;
	const eastl::string& commentString = mLanguageDefinition->mSingleLineComment;

	UndoRecord u;
	u.mBefore = mState;

	bool shouldAddComment = false;
	eastl::set<int> affectedLines;
	for (int c = mState.mCurrentCursor; c > -1; c--)
	{
		for (int currentLine = mState.mCursors[c].GetSelectionEnd().mLine; currentLine >= mState.mCursors[c].GetSelectionStart().mLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == mState.mCursors[c].GetSelectionEnd() && mState.mCursors[c].GetSelectionEnd() != mState.mCursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			int currentIndex = 0;
			while (currentIndex < mLines[currentLine].size() && (mLines[currentLine][currentIndex].mChar == ' ' || mLines[currentLine][currentIndex].mChar == '\t')) currentIndex++;
			if (currentIndex == mLines[currentLine].size())
				continue;
			int i = 0;
			while (i < commentString.length() && currentIndex + i < mLines[currentLine].size() && mLines[currentLine][currentIndex + i].mChar == commentString[i]) i++;
			bool matched = i == commentString.length();
			shouldAddComment |= !matched;
		}
	}

	if (shouldAddComment)
	{
		for (int currentLine : affectedLines) // order doesn't matter as changes are not multiline
		{
			Coordinates lineStart = { currentLine, 0 };
			Coordinates insertionEnd = lineStart;
			InsertTextAt(insertionEnd, (commentString + ' ').c_str()); // sets insertion end
			u.mOperations.push_back({ (commentString + ' ') , lineStart, insertionEnd, UndoOperationType::Add });
			ColorizeLine(lineStart.mLine);
		}
	}
	else
	{
		for (int currentLine : affectedLines) // order doesn't matter as changes are not multiline
		{
			int currentIndex = 0;
			while (currentIndex < mLines[currentLine].size() && (mLines[currentLine][currentIndex].mChar == ' ' || mLines[currentLine][currentIndex].mChar == '\t')) currentIndex++;
			if (currentIndex == mLines[currentLine].size())
				continue;
			int i = 0;
			while (i < commentString.length() && currentIndex + i < mLines[currentLine].size() && mLines[currentLine][currentIndex + i].mChar == commentString[i]) i++;
			bool matched = i == commentString.length();
			EASTL_ASSERT(matched);
			if (currentIndex + i < mLines[currentLine].size() && mLines[currentLine][currentIndex + i].mChar == ' ')
				i++;

			Coordinates start = { currentLine, GetCharacterColumn(currentLine, currentIndex) };
			Coordinates end = { currentLine, GetCharacterColumn(currentLine, currentIndex + i) };
			u.mOperations.push_back({ GetText(start, end) , start, end, UndoOperationType::Delete});
			DeleteRange(start, end);
			ColorizeLine(currentLine);
		}
	}

	u.mAfter = mState;
	AddUndo(u);

	CancelCloseChars();
}

void TextEditor::RemoveCurrentLines()
{
	UndoRecord u;
	u.mBefore = mState;

	if (AnyCursorHasSelection())
	{
		for (int c = mState.mCurrentCursor; c > -1; c--)
		{
			if (!mState.mCursors[c].HasSelection())
				continue;
			u.mOperations.push_back({ GetSelectedText(c), mState.mCursors[c].GetSelectionStart(), mState.mCursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
	}
	MoveHome();
	OnCursorPositionChanged(); // might combine cursors

	for (int c = mState.mCurrentCursor; c > -1; c--)
	{
		int currentLine = mState.mCursors[c].mInteractiveEnd.mLine;
		int nextLine = currentLine + 1;
		int prevLine = currentLine - 1;

		Coordinates toDeleteStart, toDeleteEnd;
		if (mLines.size() > nextLine) // next line exists
		{
			toDeleteStart = Coordinates(currentLine, 0);
			toDeleteEnd = Coordinates(nextLine, 0);
			SetCursorPosition({ mState.mCursors[c].mInteractiveEnd.mLine, 0 }, c);
		}
		else if (prevLine > -1) // previous line exists
		{
			toDeleteStart = Coordinates(prevLine, GetLineMaxColumn(prevLine));
			toDeleteEnd = Coordinates(currentLine, GetLineMaxColumn(currentLine));
			SetCursorPosition({ prevLine, 0 }, c);
		}
		else
		{
			toDeleteStart = Coordinates(currentLine, 0);
			toDeleteEnd = Coordinates(currentLine, GetLineMaxColumn(currentLine));
			SetCursorPosition({ currentLine, 0 }, c);
		}

		u.mOperations.push_back({ GetText(toDeleteStart, toDeleteEnd), toDeleteStart, toDeleteEnd, UndoOperationType::Delete });

		eastl::set<int> handledCursors = { c };
		if (toDeleteStart.mLine != toDeleteEnd.mLine)
			RemoveLine(currentLine, &handledCursors);
		else
			DeleteRange(toDeleteStart, toDeleteEnd);
	}

	u.mAfter = mState;
	AddUndo(u);
}

float TextEditor::TextDistanceToLineStart(const Coordinates& aFrom, bool aSanitizeCoords) const
{
	if (aSanitizeCoords)
		return SanitizeCoordinates(aFrom).mColumn * mCharAdvance.x;
	else
		return aFrom.mColumn * mCharAdvance.x;
}

void TextEditor::EnsureCursorVisible(int aCursor, bool aStartToo)
{
	if (aCursor == -1)
		aCursor = mState.GetLastAddedCursorIndex();

	mEnsureCursorVisible = aCursor;
	mEnsureCursorVisibleStartToo = aStartToo;
	return;
}

TextEditor::Coordinates TextEditor::SanitizeCoordinates(const Coordinates& aValue) const
{
	auto line = aValue.mLine;
	auto column = aValue.mColumn;
	if (line >= (int) mLines.size())
	{
		if (mLines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = (int) mLines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = mLines.empty() ? 0 : GetLineMaxColumn(line, column);
		return Coordinates(line, column);
	}
}

TextEditor::Coordinates TextEditor::GetActualCursorCoordinates(int aCursor, bool aStart) const
{
	if (aCursor == -1)
		return SanitizeCoordinates(aStart ? mState.mCursors[mState.mCurrentCursor].mInteractiveStart : mState.mCursors[mState.mCurrentCursor].mInteractiveEnd);
	else
		return SanitizeCoordinates(aStart ? mState.mCursors[aCursor].mInteractiveStart : mState.mCursors[aCursor].mInteractiveEnd);
}

TextEditor::Coordinates TextEditor::ScreenPosToCoordinates(const ImVec2& aPosition, bool aInsertionMode, bool* isOverLineNumber) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x + 3.0f, aPosition.y - origin.y);

	if (isOverLineNumber != nullptr)
		*isOverLineNumber = local.x < mTextStart;

	Coordinates out = {
		Max(0, (int)floor(local.y / mCharAdvance.y)),
		Max(0, (int)floor((local.x - mTextStart) / mCharAdvance.x))
	};
	int charIndex = GetCharacterIndexL(out);
	if (charIndex > -1 && charIndex < mLines[out.mLine].size() && mLines[out.mLine][charIndex].mChar == '\t')
	{
		int columnToLeft = GetCharacterColumn(out.mLine, charIndex);
		int columnToRight = GetCharacterColumn(out.mLine, GetCharacterIndexR(out));
		if (out.mColumn - columnToLeft < columnToRight - out.mColumn)
			out.mColumn = columnToLeft;
		else
			out.mColumn = columnToRight;
	}
	else
		out.mColumn = Max(0, (int)floor((local.x - mTextStart + POS_TO_COORDS_COLUMN_OFFSET * mCharAdvance.x) / mCharAdvance.x));
	return SanitizeCoordinates(out);
}

TextEditor::Coordinates TextEditor::FindWordStart(const Coordinates& aFrom) const
{
	if (aFrom.mLine >= (int)mLines.size())
		return aFrom;

	int lineIndex = aFrom.mLine;
	auto& line = mLines[lineIndex];
	int charIndex = GetCharacterIndexL(aFrom);

	if (charIndex > (int)line.size() || line.size() == 0)
		return aFrom;
	if (charIndex == (int)line.size())
		charIndex--;

	bool initialIsWordChar = CharIsWordChar(line[charIndex].mChar);
	bool initialIsSpace = isspace(line[charIndex].mChar);
	char initialChar = line[charIndex].mChar;
	while (Move(lineIndex, charIndex, true, true))
	{
		bool isWordChar = CharIsWordChar(line[charIndex].mChar);
		bool isSpace = isspace(line[charIndex].mChar);
		if (initialIsSpace && !isSpace ||
			initialIsWordChar && !isWordChar ||
			!initialIsWordChar && !initialIsSpace && initialChar != line[charIndex].mChar)
		{
			Move(lineIndex, charIndex, false, true); // one step to the right
			break;
		}
	}
	return { aFrom.mLine, GetCharacterColumn(aFrom.mLine, charIndex) };
}

TextEditor::Coordinates TextEditor::FindWordEnd(const Coordinates& aFrom) const
{
	if (aFrom.mLine >= (int)mLines.size())
		return aFrom;

	int lineIndex = aFrom.mLine;
	auto& line = mLines[lineIndex];
	auto charIndex = GetCharacterIndexL(aFrom);

	if (charIndex >= (int)line.size())
		return aFrom;

	bool initialIsWordChar = CharIsWordChar(line[charIndex].mChar);
	bool initialIsSpace = isspace(line[charIndex].mChar);
	char initialChar = line[charIndex].mChar;
	while (Move(lineIndex, charIndex, false, true))
	{
		if (charIndex == line.size())
			break;
		bool isWordChar = CharIsWordChar(line[charIndex].mChar);
		bool isSpace = isspace(line[charIndex].mChar);
		if (initialIsSpace && !isSpace ||
			initialIsWordChar && !isWordChar ||
			!initialIsWordChar && !initialIsSpace && initialChar != line[charIndex].mChar)
			break;
	}
	return { lineIndex, GetCharacterColumn(aFrom.mLine, charIndex) };
}

int TextEditor::GetCharacterIndexL(const Coordinates& aCoords) const
{
	if (aCoords.mLine >= mLines.size())
		return -1;

	auto& line = mLines[aCoords.mLine];
	int c = 0;
	int i = 0;
	int tabCoordsLeft = 0;

	for (; i < line.size() && c < aCoords.mColumn;)
	{
		if (line[i].mChar == '\t')
		{
			if (tabCoordsLeft == 0)
				tabCoordsLeft = TabSizeAtColumn(c);
			if (tabCoordsLeft > 0)
				tabCoordsLeft--;
			c++;
		}
		else
			++c;
		if (tabCoordsLeft == 0)
			i += UTF8CharLength(line[i].mChar);
	}
	return i;
}

int TextEditor::GetCharacterIndexR(const Coordinates& aCoords) const
{
	if (aCoords.mLine >= mLines.size())
		return -1;
	int c = 0;
	int i = 0;
	for (; i < mLines[aCoords.mLine].size() && c < aCoords.mColumn;)
		MoveCharIndexAndColumn(aCoords.mLine, i, c);
	return i;
}

int TextEditor::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= mLines.size())
		return 0;
	int c = 0;
	int i = 0;
	while (i < aIndex && i < mLines[aLine].size())
		MoveCharIndexAndColumn(aLine, i, c);
	return c;
}

int TextEditor::GetFirstVisibleCharacterIndex(int aLine) const
{
	if (aLine >= mLines.size())
		return 0;
	int c = 0;
	int i = 0;
	while (c < mFirstVisibleColumn && i < mLines[aLine].size())
		MoveCharIndexAndColumn(aLine, i, c);
	if (c > mFirstVisibleColumn)
		i--;
	return i;
}

int TextEditor::GetLineMaxColumn(int aLine, int aLimit) const
{
	if (aLine >= mLines.size())
		return 0;
	int c = 0;
	if (aLimit == -1)
	{
		for (int i = 0; i < mLines[aLine].size(); )
			MoveCharIndexAndColumn(aLine, i, c);
	}
	else
	{
		for (int i = 0; i < mLines[aLine].size(); )
		{
			MoveCharIndexAndColumn(aLine, i, c);
			if (c > aLimit)
				return aLimit;
		}
	}
	return c;
}

TextEditor::Line& TextEditor::InsertLine(int aIndex)
{
	EASTL_ASSERT(!mReadOnly);

	int atTheEOL = !mLines.empty() && (mLines[max(aIndex - 1, 0)].size() == mState.mCursors[0].mInteractiveEnd.mColumn) ? 1 : 0;

	for (auto&& err : mErrorMarkers)
		if (err.line >= aIndex + atTheEOL)
			err.line++;

	for (int& bp : mBreakpoints)
		if (bp >= aIndex + atTheEOL)
			bp++;

	auto& result = *mLines.insert(mLines.begin() + aIndex, Line());

	for (int c = 0; c <= mState.mCurrentCursor; c++) // handle multiple cursors
	{
		if (mState.mCursors[c].mInteractiveEnd.mLine >= aIndex)
			SetCursorPosition({ mState.mCursors[c].mInteractiveEnd.mLine + 1, mState.mCursors[c].mInteractiveEnd.mColumn }, c);
	}

	return result;
}

void TextEditor::RemoveLine(int aIndex, const eastl::set<int>* aHandledCursors)
{
	EASTL_ASSERT(!mReadOnly);
	EASTL_ASSERT(mLines.size() > 1);

	mLines.erase(mLines.begin() + aIndex);
	EASTL_ASSERT(!mLines.empty());

	// handle multiple cursors
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		if (mState.mCursors[c].mInteractiveEnd.mLine >= aIndex)
		{
			if (aHandledCursors == nullptr || aHandledCursors->find(c) == aHandledCursors->end()) // move up if has not been handled already
				SetCursorPosition({ mState.mCursors[c].mInteractiveEnd.mLine - 1, mState.mCursors[c].mInteractiveEnd.mColumn }, c);
		}
	}

	mErrorMarkers.erase(eastl::remove_if(mErrorMarkers.begin(), mErrorMarkers.end(),
		[aIndex](const ErrorMarker& aMarker) { return aMarker.line == aIndex + 1; }), mErrorMarkers.end());

	for (auto && err : mErrorMarkers)
		if (err.line >= aIndex)
			err.line--;

	mBreakpoints.erase(eastl::remove_if(mBreakpoints.begin(), mBreakpoints.end(),
		[aIndex](int aMarker) { return aMarker == aIndex + 1; }), mBreakpoints.end());

	for (int & bp : mBreakpoints)
		if (bp >= aIndex)
			bp--;
}

void TextEditor::RemoveLines(int aStart, int aEnd)
{
	EASTL_ASSERT(!mReadOnly);
	EASTL_ASSERT(aEnd >= aStart);
	EASTL_ASSERT(mLines.size() > (size_t)(aEnd - aStart));

	mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);
	EASTL_ASSERT(!mLines.empty());

	// handle multiple cursors
	for (int c = 0; c <= mState.mCurrentCursor; c++)
	{
		if (mState.mCursors[c].mInteractiveEnd.mLine >= aStart)
		{
			int targetLine = mState.mCursors[c].mInteractiveEnd.mLine - (aEnd - aStart);
			targetLine = targetLine < 0 ? 0 : targetLine;
			SetCursorPosition({ targetLine , mState.mCursors[c].mInteractiveEnd.mColumn }, c);
		}
	}

	mErrorMarkers.erase(eastl::remove_if(mErrorMarkers.begin(), mErrorMarkers.end(),
		[aStart, aEnd](const ErrorMarker& aMarker) { return aMarker.line > aStart - 1 && aMarker.line < aEnd; }), mErrorMarkers.end());

	for (auto && err : mErrorMarkers)
		if (err.line >= aEnd)
			err.line -= (aEnd - aStart);

	mBreakpoints.erase(eastl::remove_if(mBreakpoints.begin(), mBreakpoints.end(),
		[aStart, aEnd](int aMarker) { return aMarker > aStart - 1 && aMarker < aEnd; }), mBreakpoints.end());

	for (int & bp : mBreakpoints)
		if (bp >= aEnd)
			bp -= (aEnd - aStart);

}

void TextEditor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
	EASTL_ASSERT(aEnd >= aStart);
	EASTL_ASSERT(!mReadOnly);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndexL(aStart);
	auto end = GetCharacterIndexR(aEnd);

	if (aStart.mLine == aEnd.mLine)
	{
		auto n = GetLineMaxColumn(aStart.mLine);
		if (aEnd.mColumn >= n)
			RemoveGlyphsFromLine(aStart.mLine, start); // from start to end of line
		else
			RemoveGlyphsFromLine(aStart.mLine, start, end);
	}
	else
	{
		RemoveGlyphsFromLine(aStart.mLine, start); // from start to end of line
		RemoveGlyphsFromLine(aEnd.mLine, 0, end);
		auto& firstLine = mLines[aStart.mLine];
		auto& lastLine = mLines[aEnd.mLine];

		if (aStart.mLine < aEnd.mLine)
		{
			AddGlyphsToLine(aStart.mLine, firstLine.size(), lastLine.begin(), lastLine.end());
			for (int c = 0; c <= mState.mCurrentCursor; c++) // move up cursors in line that is being moved up
			{
				if (mState.mCursors[c].mInteractiveEnd.mLine > aEnd.mLine)
					break;
				else if (mState.mCursors[c].mInteractiveEnd.mLine != aEnd.mLine)
					continue;

				if (mState.mCursors[c].mInteractiveStart == aStart && mState.mCursors[c].mInteractiveEnd == aEnd)
				{
					SetCursorPosition(aStart, c, false);
					continue;
				}

				int otherCursorEndCharIndex = GetCharacterIndexR(mState.mCursors[c].mInteractiveEnd);
				int otherCursorStartCharIndex = GetCharacterIndexR(mState.mCursors[c].mInteractiveStart);
				int otherCursorNewEndCharIndex = GetCharacterIndexR(aStart) + otherCursorEndCharIndex;
				int otherCursorNewStartCharIndex = GetCharacterIndexR(aStart) + otherCursorStartCharIndex;
				auto targetEndCoords = Coordinates(aStart.mLine, GetCharacterColumn(aStart.mLine, otherCursorNewEndCharIndex));
				auto targetStartCoords = Coordinates(aStart.mLine, GetCharacterColumn(aStart.mLine, otherCursorNewStartCharIndex));
				SetCursorPosition(targetStartCoords, c, true);
				SetCursorPosition(targetEndCoords, c, false);
			}
			RemoveLines(aStart.mLine + 1, aEnd.mLine + 1);
		}
	}
}

void TextEditor::DeleteSelection(int aCursor)
{
	if (aCursor == -1)
		aCursor = mState.mCurrentCursor;

	if (mState.mCursors[aCursor].GetSelectionEnd() == mState.mCursors[aCursor].GetSelectionStart())
		return;

	DeleteRange(mState.mCursors[aCursor].GetSelectionStart(), mState.mCursors[aCursor].GetSelectionEnd());
	SetCursorPosition(mState.mCursors[aCursor].GetSelectionStart(), aCursor);
	ColorizeLine(mState.mCursors[aCursor].GetSelectionStart().mLine);
}

void TextEditor::RemoveGlyphsFromLine(int aLine, int aStartChar, int aEndChar)
{
	int column = GetCharacterColumn(aLine, aStartChar);
	auto& line = mLines[aLine];
	OnLineChanged(true, aLine, column, aEndChar - aStartChar, true);
	line.erase(line.begin() + aStartChar, aEndChar == -1 ? line.end() : line.begin() + aEndChar);
	OnLineChanged(false, aLine, column, aEndChar - aStartChar, true);
}

void TextEditor::AddGlyphsToLine(int aLine, int aTargetIndex, Line::iterator aSourceStart, Line::iterator aSourceEnd)
{
	int targetColumn = GetCharacterColumn(aLine, aTargetIndex);
	int charsInserted = eastl::distance(aSourceStart, aSourceEnd);
	auto& line = mLines[aLine];
	OnLineChanged(true, aLine, targetColumn, charsInserted, false);
	line.insert(line.begin() + aTargetIndex, aSourceStart, aSourceEnd);
	OnLineChanged(false, aLine, targetColumn, charsInserted, false);
}

void TextEditor::AddGlyphToLine(int aLine, int aTargetIndex, Glyph aGlyph)
{
	int targetColumn = GetCharacterColumn(aLine, aTargetIndex);
	auto& line = mLines[aLine];
	OnLineChanged(true, aLine, targetColumn, 1, false);
	line.insert(line.begin() + aTargetIndex, aGlyph);
	OnLineChanged(false, aLine, targetColumn, 1, false);
}

ImU32 TextEditor::GetGlyphColor(const Glyph& aGlyph) const
{
	if (mLanguageDefinition == nullptr)
		return mPalette[(int)PaletteIndex::Default];

	return mPalette[(int)aGlyph.mColorIndex];
}

bool TextEditor::LineContainsOnlySpaces(int aLine) const
{
	auto& line = mLines[aLine];
	if (line.empty())
		return false;

	for (auto& glyph : line)
	{
		if (glyph.mChar != ' ' && glyph.mChar != '\t')
			return false;
	}
	return true;
}

void TextEditor::HandleKeyboardInputs(bool aParentIsFocused)
{
	if (ImGui::IsWindowFocused() || aParentIsFocused)
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

		ImGuiIO& io = ImGui::GetIO();
		auto isOSX = io.ConfigMacOSXBehaviors;
		auto alt = io.KeyAlt;
		auto ctrl = io.KeyCtrl;
		auto shift = io.KeyShift;
		auto super = io.KeySuper;

		auto isShortcut = (isOSX ? (super && !ctrl) : (ctrl && !super)) && !alt && !shift;
		auto isShiftShortcut = (isOSX ? (super && !ctrl) : (ctrl && !super)) && shift && !alt;
		auto isWordmoveKey = isOSX ? alt : ctrl;
		auto isAltOnly = alt && !ctrl && !shift && !super;
		auto isCtrlOnly = ctrl && !alt && !shift && !super;
		auto isShiftOnly = shift && !alt && !ctrl && !super;

		bool homePressed = ImGui::IsKeyPressed(ImGuiKey_Home) || (ImGui::IsKeyPressed(ImGuiKey_Keypad7) && !io.InputQueueCharacters.Size);
		bool endPressed = ImGui::IsKeyPressed(ImGuiKey_End) || (ImGui::IsKeyPressed(ImGuiKey_Keypad1) && !io.InputQueueCharacters.Size);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!mReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_Z))
			Undo(true);
		else if (!mReadOnly && isAltOnly && ImGui::IsKeyPressed(ImGuiKey_Backspace))
			Undo(true);
		else if (!mReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_Y))
			Redo(true);
		else if (!mReadOnly && isShiftShortcut && ImGui::IsKeyPressed(ImGuiKey_Z))
			Redo(true);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUp(1, shift);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDown(1, shift);
		else if ((isOSX ? !ctrl : !alt) && !super && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			MoveLeft(shift, isWordmoveKey);
		else if ((isOSX ? !ctrl : !alt) && !super && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			MoveRight(shift, isWordmoveKey);
		else if (!alt && !ctrl && !super && (ImGui::IsKeyPressed(ImGuiKey_PageUp) || (ImGui::IsKeyPressed(ImGuiKey_Keypad9) && !io.InputQueueCharacters.Size)))
			MoveUp(mVisibleLineCount - 2, shift);
		else if (!alt && !ctrl && !super && (ImGui::IsKeyPressed(ImGuiKey_PageDown) || (ImGui::IsKeyPressed(ImGuiKey_Keypad3) && !io.InputQueueCharacters.Size)))
			MoveDown(mVisibleLineCount - 2, shift);
		else if (ctrl && !alt && !super && homePressed)
			MoveTop(shift);
		else if (ctrl && !alt && !super && endPressed)
			MoveBottom(shift);
		else if (!alt && !ctrl && !super && homePressed)
			MoveHome(shift);
		else if (!alt && !ctrl && !super && endPressed)
			MoveEnd(shift);
		else if (!mReadOnly && !alt && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Delete))
			Delete(ctrl);
		else if (!mReadOnly && !alt && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Backspace))
		{
			if (!ctrl && mState.mCursors.size() == 1 && LineContainsOnlySpaces(mState.mCursors[0].mInteractiveStart.mLine))
				ChangeCurrentLinesIndentation(false);
			else
				Backspace(ctrl);
		}
		else if (!mReadOnly && !alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_K))
			RemoveCurrentLines();
		else if (!mReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_LeftBracket))
			ChangeCurrentLinesIndentation(false);
		else if (!mReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_RightBracket))
			ChangeCurrentLinesIndentation(true);
		else if (!alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUpCurrentLines();
		else if (!alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDownCurrentLines();
		else if (!mReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Slash))
			ToggleLineComment();
		else if (!alt && !ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Insert))
			mOverwrite ^= true;
		else if (isCtrlOnly && ImGui::IsKeyPressed(ImGuiKey_Insert))
			Copy();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_C))
			Copy();
		else if (!mReadOnly && isShiftOnly && ImGui::IsKeyPressed(ImGuiKey_Insert))
			Paste();
		else if (!mReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_V))
			Paste();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_X))
			Cut();
		else if (isShiftOnly && ImGui::IsKeyPressed(ImGuiKey_Delete))
			Cut();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_A))
			SelectAll();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_D))
			AddCursorForNextOccurrence();
		else if (!mReadOnly && !alt && !ctrl && !shift && !super && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
			EnterCharacter('\n', false);
		else if (!mReadOnly && !alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_Tab))
			EnterCharacter('\t', shift);
		if (!mReadOnly && !io.InputQueueCharacters.empty() && ctrl == alt && !super)
		{
			for (int i = 0; i < io.InputQueueCharacters.Size; i++)
			{
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					EnterCharacter(c, shift);
			}
			io.InputQueueCharacters.resize(0);
		}


#if ENABLE_EDITOR_FUZZING
		if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Keypad5))
			fuzzer.enabled = !fuzzer.enabled;

		if (fuzzer.enabled)
			for (int t = 0; t < 500; t++)
			{
				fuzzer.newIteration();

				if (fuzzer.prob(30))
					Undo(true);
				if (fuzzer.prob(30))
					Redo(true);
				if (fuzzer.prob(30))
					Delete(fuzzer.prob(2));
				if (fuzzer.prob(30))
					ChangeCurrentLinesIndentation(fuzzer.prob(2));
				if (fuzzer.prob(30))
					Backspace(fuzzer.prob(2));
				if (fuzzer.prob(30))
					RemoveCurrentLines();
				if (fuzzer.prob(30))
					MoveUpCurrentLines();
				if (fuzzer.prob(30))
					MoveDownCurrentLines();
				if (fuzzer.prob(30))
					ToggleLineComment();
				if (fuzzer.prob(30))
					mOverwrite ^= true;
				if (fuzzer.prob(30))
					Copy();
				if (fuzzer.prob(30))
					Paste();
				if (fuzzer.prob(30))
					Cut();
				if (fuzzer.prob(30))
					SelectAll();
				if (fuzzer.prob(30))
					EnterCharacter('\n', false);
				if (fuzzer.prob(8))
					MoveUp(1, fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveDown(1, fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveLeft(fuzzer.prob(2), fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveRight(fuzzer.prob(2), fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveTop(fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveBottom(fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveHome(fuzzer.prob(2));
				if (fuzzer.prob(8))
					MoveEnd(fuzzer.prob(2));
				if (fuzzer.prob(10))
					EnterCharacter('\t', fuzzer.prob(2));

				for (int i = 0; i < 5; i++)
				{
					if (fuzzer.prob(2))
						break;
					EnterCharacter(fuzzer.prob(10) ? ' ' : fuzzer.getKey(), fuzzer.prob(2));
				}
			}
#endif

	}
}

void TextEditor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	/*
	Pan with middle mouse button
	*/
	mPanning &= ImGui::IsMouseDown(2);
	if (mPanning && ImGui::IsMouseDragging(2))
	{
		ImVec2 scroll = { ImGui::GetScrollX(), ImGui::GetScrollY() };
		ImVec2 currentMousePos = ImGui::GetMouseDragDelta(2);
		ImVec2 mouseDelta = {
			currentMousePos.x - mLastMousePos.x,
			currentMousePos.y - mLastMousePos.y
		};
		ImGui::SetScrollY(scroll.y - mouseDelta.y);
		ImGui::SetScrollX(scroll.x - mouseDelta.x);
		mLastMousePos = currentMousePos;
	}

	// Mouse left button dragging (=> update selection)
	mDraggingSelection &= ImGui::IsMouseDown(0);
	if (mDraggingSelection && ImGui::IsMouseDragging(0))
	{
		io.WantCaptureMouse = true;
		Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos(), !mOverwrite);
		SetCursorPosition(cursorCoords, mState.GetLastAddedCursorIndex(), false);
	}

	if (ImGui::IsWindowHovered())
	{
		auto click = ImGui::IsMouseClicked(0);
		if (!shift && !alt)
		{
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick &&
				(mLastClickTime != -1.0f && (t - mLastClickTime) < io.MouseDoubleClickTime &&
					Distance(io.MousePos, mLastClickPos) < 0.01f);

			if (click)
				mDraggingSelection = true;

			/*
			Pan with middle mouse button
			*/

			if (ImGui::IsMouseClicked(2))
			{
				mPanning = true;
				mLastMousePos = ImGui::GetMouseDragDelta(2);
			}

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (ctrl)
					mState.AddCursor();
				else
					mState.mCurrentCursor = 0;

				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos());
				Coordinates targetCursorPos = cursorCoords.mLine < mLines.size() - 1 ?
					Coordinates{ cursorCoords.mLine + 1, 0 } :
					Coordinates{ cursorCoords.mLine, GetLineMaxColumn(cursorCoords.mLine) };
				SetSelection({ cursorCoords.mLine, 0 }, targetCursorPos, mState.mCurrentCursor);

				mLastClickTime = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (ctrl)
					mState.AddCursor();
				else
					mState.mCurrentCursor = 0;

				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(FindWordStart(cursorCoords), FindWordEnd(cursorCoords), mState.mCurrentCursor);

				mLastClickTime = (float)ImGui::GetTime();
				mLastClickPos = io.MousePos;
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				if (ctrl)
					mState.AddCursor();
				else
					mState.mCurrentCursor = 0;

				bool isOverLineNumber;
				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos(), !mOverwrite, &isOverLineNumber);
				if (isOverLineNumber)
				{
					Coordinates targetCursorPos = cursorCoords.mLine < mLines.size() - 1 ?
						Coordinates{ cursorCoords.mLine + 1, 0 } :
						Coordinates{ cursorCoords.mLine, GetLineMaxColumn(cursorCoords.mLine) };
					SetSelection({ cursorCoords.mLine, 0 }, targetCursorPos, mState.mCurrentCursor);
				}
				else
					SetCursorPosition(cursorCoords, mState.GetLastAddedCursorIndex());

				mLastClickTime = (float)ImGui::GetTime();
				mLastClickPos = io.MousePos;
			}
			else if (ImGui::IsMouseReleased(0))
			{
				mState.SortCursorsFromTopToBottom();
				MergeCursorsIfPossible();
			}
		}
		else if (shift)
		{
			if (click)
			{
				Coordinates newSelection = ScreenPosToCoordinates(ImGui::GetMousePos(), !mOverwrite);
				SetCursorPosition(newSelection, mState.mCurrentCursor, false);
			}
		}
	}
}

void TextEditor::UpdateViewVariables(float aScrollX, float aScrollY)
{
	mContentHeight = ImGui::GetWindowHeight() - (IsHorizontalScrollbarVisible() ? IMGUI_SCROLLBAR_WIDTH : 0.0f);
	mContentWidth = ImGui::GetWindowWidth() - (IsVerticalScrollbarVisible() ? IMGUI_SCROLLBAR_WIDTH : 0.0f);

	mVisibleLineCount = Max((int)ceil(mContentHeight / mCharAdvance.y), 0);
	mFirstVisibleLine = Max((int)(aScrollY / mCharAdvance.y), 0);
	mLastVisibleLine = Max((int)((mContentHeight + aScrollY) / mCharAdvance.y), 0);

	mVisibleColumnCount = Max((int)ceil((mContentWidth - Max(mTextStart - aScrollX, 0.0f)) / mCharAdvance.x), 0);
	mFirstVisibleColumn = Max((int)(Max(aScrollX - mTextStart, 0.0f) / mCharAdvance.x), 0);
	mLastVisibleColumn = Max((int)((mContentWidth + aScrollX - mTextStart) / mCharAdvance.x), 0);
}

void TextEditor::Render(bool aParentIsFocused)
{
	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	const float fontHeight = ImGui::GetTextLineHeightWithSpacing();
	mCharAdvance = ImVec2(fontWidth, fontHeight * mLineSpacing);

	// Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
	mTextStart = mLeftMargin;
	static char lineNumberBuffer[16];
	if (mShowLineNumbers)
	{
		snprintf(lineNumberBuffer, 16, " %d ", int(mLines.size()));
		mTextStart += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineNumberBuffer, nullptr, nullptr).x;
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	mScrollX = ImGui::GetScrollX();
	mScrollY = ImGui::GetScrollY();
	UpdateViewVariables(mScrollX, mScrollY);

	int maxColumnLimited = 0;
	if (!mLines.empty())
	{
		auto drawList = ImGui::GetWindowDrawList();
		float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		for (int lineNo = mFirstVisibleLine; lineNo <= mLastVisibleLine && lineNo < mLines.size(); lineNo++)
		{
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

			auto& line = mLines[lineNo];
			maxColumnLimited = Max(GetLineMaxColumn(lineNo, mLastVisibleColumn), maxColumnLimited);

			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, maxColumnLimited);

			// Draw highlights for the current line
			if (lineNo < highlights.size() && highlights[lineNo])
			{
				HighlightRanges &ranges = *highlights[lineNo];
				for (auto && range : ranges)
				{
					if (range.first < line.size() && range.second <= line.size())
					{
						float x1 = TextDistanceToLineStart(Coordinates(lineNo, range.first));
						float x2 = TextDistanceToLineStart(Coordinates(lineNo, range.second));
						drawList->AddRectFilled(ImVec2{ lineStartScreenPos.x + mTextStart + x1, lineStartScreenPos.y },
							ImVec2{ lineStartScreenPos.x + mTextStart + x2, lineStartScreenPos.y + mCharAdvance.y },
							mPalette[(int)PaletteIndex::HighlightedTextFill]);
					}
				}
			}

			// Draw selection for the current line
			for (int c = 0; c <= mState.mCurrentCursor; c++)
			{
				float rectStart = -1.0f;
				float rectEnd = -1.0f;
				Coordinates cursorSelectionStart = mState.mCursors[c].GetSelectionStart();
				Coordinates cursorSelectionEnd = mState.mCursors[c].GetSelectionEnd();
				EASTL_ASSERT(cursorSelectionStart <= cursorSelectionEnd);

				if (cursorSelectionStart <= lineEndCoord)
					rectStart = cursorSelectionStart > lineStartCoord ? TextDistanceToLineStart(cursorSelectionStart) : 0.0f;
				if (cursorSelectionEnd > lineStartCoord)
					rectEnd = TextDistanceToLineStart(cursorSelectionEnd < lineEndCoord ? cursorSelectionEnd : lineEndCoord);
				if (cursorSelectionEnd.mLine > lineNo || cursorSelectionEnd.mLine == lineNo && cursorSelectionEnd > lineEndCoord)
					rectEnd += mCharAdvance.x;

				if (rectStart != -1 && rectEnd != -1 && rectStart < rectEnd)
					drawList->AddRectFilled(
						ImVec2{ lineStartScreenPos.x + mTextStart + rectStart, lineStartScreenPos.y },
						ImVec2{ lineStartScreenPos.x + mTextStart + rectEnd, lineStartScreenPos.y + mCharAdvance.y },
						mPalette[(int)PaletteIndex::Selection]);
			}

			// Draw breakpoints
			auto start = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

			auto breakpointIt = eastl::find(mBreakpoints.begin(), mBreakpoints.end(), lineNo + 1);
			if (breakpointIt != mBreakpoints.end())
			{
				auto end = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::Breakpoint]);
			}

			// Draw error markers
			auto errorIt = eastl::find_if(mErrorMarkers.begin(), mErrorMarkers.end(),
				[lineNo](const ErrorMarker& aMarker) { return aMarker.line == lineNo + 1; });

			if (errorIt != mErrorMarkers.end())
			{
				auto end = ImVec2(lineStartScreenPos.x + mTextStart + 4000, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::ErrorMarker]);

				if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
				{
					ImGui::BeginTooltip();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("Error at line %d:", errorIt->line);
					ImGui::PopStyleColor();
					ImGui::Separator();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
					ImGui::Text("%s", errorIt->text.c_str());
					ImGui::PopStyleColor();
					ImGui::EndTooltip();
				}
			}

			// Draw line number (right aligned)
			if (mShowLineNumbers)
			{
				snprintf(lineNumberBuffer, 16, "%d  ", lineNo + 1);
				float lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineNumberBuffer, nullptr, nullptr).x;
				drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth, lineStartScreenPos.y), mPalette[(int)PaletteIndex::LineNumber], lineNumberBuffer);
			}

			eastl::vector<Coordinates> cursorCoordsInThisLine;
			for (int c = 0; c <= mState.mCurrentCursor; c++)
			{
				if (mState.mCursors[c].mInteractiveEnd.mLine == lineNo)
					cursorCoordsInThisLine.push_back(mState.mCursors[c].mInteractiveEnd);
			}
			if (cursorCoordsInThisLine.size() > 0)
			{
				bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || aParentIsFocused;

				// Render the cursors
				if (focused)
				{
					for (const auto& cursorCoords : cursorCoordsInThisLine)
					{
						float width = 1.0f;
						auto cindex = GetCharacterIndexR(cursorCoords);
						float cx = TextDistanceToLineStart(cursorCoords);

						if (mOverwrite && cindex < (int)line.size())
						{
							if (line[cindex].mChar == '\t')
							{
								auto x = (1.0f + floorf((1.0f + cx) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
								width = x - cx;
							}
							else
								width = mCharAdvance.x;
						}
						ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + mCharAdvance.y);
						drawList->AddRectFilled(cstart, cend, mPalette[(int)PaletteIndex::Cursor]);
						if (mCursorOnBracket)
						{
							ImVec2 topLeft = { cstart.x, lineStartScreenPos.y + fontHeight + 1.0f };
							ImVec2 bottomRight = { topLeft.x + mCharAdvance.x, topLeft.y + 1.0f };
							drawList->AddRectFilled(topLeft, bottomRight, mPalette[(int)PaletteIndex::Cursor]);
						}
					}
				}
			}

			// Render colorized text
			static eastl::string glyphBuffer;
			int charIndex = GetFirstVisibleCharacterIndex(lineNo);
			int column = mFirstVisibleColumn; // can be in the middle of tab character
			while (charIndex < mLines[lineNo].size() && column <= mLastVisibleColumn)
			{
				auto& glyph = line[charIndex];
				auto color = GetGlyphColor(glyph);
				ImVec2 targetGlyphPos = { lineStartScreenPos.x + mTextStart + TextDistanceToLineStart({lineNo, column}, false), lineStartScreenPos.y };

				if (glyph.mChar == '\t')
				{
					if (mShowWhitespaces)
					{
						ImVec2 p1, p2, p3, p4;

						const auto s = ImGui::GetFontSize();
						const auto x1 = targetGlyphPos.x + mCharAdvance.x * 0.3f;
						const auto y = targetGlyphPos.y + fontHeight * 0.5f;

						if (mShortTabs)
						{
							const auto x2 = targetGlyphPos.x + mCharAdvance.x;
							p1 = ImVec2(x1, y);
							p2 = ImVec2(x2, y);
							p3 = ImVec2(x2 - s * 0.16f, y - s * 0.16f);
							p4 = ImVec2(x2 - s * 0.16f, y + s * 0.16f);
						}
						else
						{
							const auto x2 = targetGlyphPos.x + TabSizeAtColumn(column) * mCharAdvance.x - mCharAdvance.x * 0.3f;
							p1 = ImVec2(x1, y);
							p2 = ImVec2(x2, y);
							p3 = ImVec2(x2 - s * 0.2f, y - s * 0.2f);
							p4 = ImVec2(x2 - s * 0.2f, y + s * 0.2f);
						}

						drawList->AddLine(p1, p2, mPalette[(int)PaletteIndex::ControlCharacter]);
						drawList->AddLine(p2, p3, mPalette[(int)PaletteIndex::ControlCharacter]);
						drawList->AddLine(p2, p4, mPalette[(int)PaletteIndex::ControlCharacter]);
					}
				}
				else if (glyph.mChar == ' ')
				{
					if (mShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x = targetGlyphPos.x + spaceSize * 0.5f;
						const auto y = targetGlyphPos.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, mPalette[(int)PaletteIndex::ControlCharacter], 4);
					}
				}
				else
				{
					int seqLength = UTF8CharLength(glyph.mChar);
					if (mCursorOnBracket && seqLength == 1 && mMatchingBracketCoords == Coordinates{ lineNo, column })
					{
						ImVec2 topLeft = { targetGlyphPos.x, targetGlyphPos.y + fontHeight + 1.0f };
						ImVec2 bottomRight = { topLeft.x + mCharAdvance.x, topLeft.y + 1.0f };
						drawList->AddRectFilled(topLeft, bottomRight, mPalette[(int)PaletteIndex::Cursor]);
					}
					glyphBuffer.clear();
					for (int i = 0; i < seqLength; i++)
						glyphBuffer.push_back(line[charIndex + i].mChar);
					drawList->AddText(targetGlyphPos, color, glyphBuffer.c_str());
				}

				MoveCharIndexAndColumn(lineNo, charIndex, column);
			}
		}
	}
	mCurrentSpaceHeight = (mLines.size() + Min(mVisibleLineCount - 1, (int)mLines.size())) * mCharAdvance.y;
	mCurrentSpaceWidth = Max((maxColumnLimited + Min(mVisibleColumnCount - 1, maxColumnLimited)) * mCharAdvance.x, mCurrentSpaceWidth);

	ImGui::SetCursorPos(ImVec2(0, 0));
	ImGui::Dummy(ImVec2(mCurrentSpaceWidth, mCurrentSpaceHeight));

	if (mEnsureCursorVisible > -1)
	{
		for (int i = 0; i < (mEnsureCursorVisibleStartToo ? 2 : 1); i++) // first pass for interactive end and second pass for interactive start
		{
			if (i) UpdateViewVariables(mScrollX, mScrollY); // second pass depends on changes made in first pass
			Coordinates targetCoords = GetActualCursorCoordinates(mEnsureCursorVisible, i); // cursor selection end or start
			if (targetCoords.mLine <= mFirstVisibleLine)
			{
				float targetScroll = eastl::max(0.0f, (targetCoords.mLine - 0.5f) * mCharAdvance.y);
				if (targetScroll < mScrollY)
					ImGui::SetScrollY(targetScroll);
			}
			if (targetCoords.mLine >= mLastVisibleLine)
			{
				float targetScroll = eastl::max(0.0f, (targetCoords.mLine + 1.5f) * mCharAdvance.y - mContentHeight);
				if (targetScroll > mScrollY)
					ImGui::SetScrollY(targetScroll);
			}
			if (targetCoords.mColumn <= mFirstVisibleColumn)
			{
				float targetScroll = eastl::max(0.0f, mTextStart + (targetCoords.mColumn - 0.5f) * mCharAdvance.x);
				if (targetScroll < mScrollX)
					ImGui::SetScrollX(mScrollX = targetScroll);
			}
			if (targetCoords.mColumn >= mLastVisibleColumn)
			{
				float targetScroll = eastl::max(0.0f, mTextStart + (targetCoords.mColumn + 0.5f) * mCharAdvance.x - mContentWidth);
				if (targetScroll > mScrollX)
					ImGui::SetScrollX(mScrollX = targetScroll);
			}
		}
		mEnsureCursorVisible = -1;
	}
	if (mScrollToTop)
	{
		ImGui::SetScrollY(0.0f);
		mScrollToTop = false;
	}
	if (mSetViewAtLine > -1)
	{
		float targetScroll;
		switch (mSetViewAtLineMode)
		{
		default:
		case SetViewAtLineMode::FirstVisibleLine:
			targetScroll = eastl::max(0.0f, (float)mSetViewAtLine * mCharAdvance.y);
			break;
		case SetViewAtLineMode::LastVisibleLine:
			targetScroll = eastl::max(0.0f, (float)(mSetViewAtLine - (mLastVisibleLine - mFirstVisibleLine)) * mCharAdvance.y);
			break;
		case SetViewAtLineMode::Centered:
			targetScroll = eastl::max(0.0f, ((float)mSetViewAtLine - (float)(mLastVisibleLine - mFirstVisibleLine) * 0.5f) * mCharAdvance.y);
			break;
		}
		ImGui::SetScrollY(targetScroll);
		mSetViewAtLine = -1;
	}
}

void TextEditor::OnCursorPositionChanged()
{
	if (mState.mCurrentCursor == 0 && !mState.mCursors[0].HasSelection()) // only one cursor without selection
		mCursorOnBracket = FindMatchingBracket(mState.mCursors[0].mInteractiveEnd.mLine,
			GetCharacterIndexR(mState.mCursors[0].mInteractiveEnd), mMatchingBracketCoords);
	else
		mCursorOnBracket = false;

	if (!mDraggingSelection)
	{
		mState.SortCursorsFromTopToBottom();
		MergeCursorsIfPossible();
	}
}

void TextEditor::OnLineChanged(bool aBeforeChange, int aLine, int aColumn, int aCharCount, bool aDeleted) // adjusts cursor position when other cursor writes/deletes in the same line
{
	static eastl::unordered_map<int, int> cursorCharIndices;
	if (aBeforeChange)
	{
		cursorCharIndices.clear();
		for (int c = 0; c <= mState.mCurrentCursor; c++)
		{
			if (mState.mCursors[c].mInteractiveEnd.mLine == aLine && // cursor is at the line
				mState.mCursors[c].mInteractiveEnd.mColumn > aColumn && // cursor is to the right of changing part
				mState.mCursors[c].GetSelectionEnd() == mState.mCursors[c].GetSelectionStart()) // cursor does not have a selection
			{
				cursorCharIndices[c] = GetCharacterIndexR({ aLine, mState.mCursors[c].mInteractiveEnd.mColumn });
				cursorCharIndices[c] += aDeleted ? -aCharCount : aCharCount;
			}
		}
	}
	else
	{
		for (auto& item : cursorCharIndices)
			SetCursorPosition({ aLine, GetCharacterColumn(aLine, item.second) }, item.first);
	}
}

void TextEditor::MergeCursorsIfPossible()
{
	// requires the cursors to be sorted from top to bottom
	eastl::set<int> cursorsToDelete;
	if (AnyCursorHasSelection())
	{
		// merge cursors if they overlap
		for (int c = mState.mCurrentCursor; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1; // pc for previous cursor

			bool pcContainsC = mState.mCursors[pc].GetSelectionEnd() >= mState.mCursors[c].GetSelectionEnd();
			bool pcContainsStartOfC = mState.mCursors[pc].GetSelectionEnd() > mState.mCursors[c].GetSelectionStart();

			if (pcContainsC)
			{
				cursorsToDelete.insert(c);
			}
			else if (pcContainsStartOfC)
			{
				Coordinates pcStart = mState.mCursors[pc].GetSelectionStart();
				Coordinates cEnd = mState.mCursors[c].GetSelectionEnd();
				mState.mCursors[pc].mInteractiveEnd = cEnd;
				mState.mCursors[pc].mInteractiveStart = pcStart;
				cursorsToDelete.insert(c);
			}
		}
	}
	else
	{
		// merge cursors if they are at the same position
		for (int c = mState.mCurrentCursor; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1;
			if (mState.mCursors[pc].mInteractiveEnd == mState.mCursors[c].mInteractiveEnd)
				cursorsToDelete.insert(c);
		}
	}
	for (int c = mState.mCurrentCursor; c > -1; c--)// iterate backwards through each of them
	{
		if (cursorsToDelete.find(c) != cursorsToDelete.end())
			mState.mCursors.erase(mState.mCursors.begin() + c);
	}
	mState.mCurrentCursor -= cursorsToDelete.size();
}

void TextEditor::AddUndo(UndoRecord& aValue)
{
	EASTL_ASSERT(!mReadOnly);
	mUndoBuffer.resize((size_t)(mUndoIndex + 1));
	mUndoBuffer.back() = aValue;
	++mUndoIndex;
}


void TextEditor::ColorizeAll()
{
	ColorizeRange(0, INT_MAX, true);
	colorizeTime = 0.0;
}

void TextEditor::ColorizeLine(int line)
{
	ColorizeRange(line - 1, line + 1, false);
	colorizeTime = ImGui::GetTime() + 0.4;
}

static bool equal_token(const char * str, const char * token)
{
	while (*token)
	{
		if (*str != *token)
			return false;
		token++;
		str++;
	}
	return true;
}


void TextEditor::ColorizeRange(int from, int to, bool multiline_tokens)
{
	if (!mLanguageDefinition)
		return;

	from = eastl::max(from, 0);
	to = eastl::min((int)mLines.size() - 1, to);
	bool limitedRange = from != 0 || to != (int)mLines.size() - 1;

	eastl::string commentStart = mLanguageDefinition->mCommentStart;
	eastl::string commentEnd = mLanguageDefinition->mCommentEnd;
	eastl::string commentLine = mLanguageDefinition->mSingleLineComment;
	eastl::string preprocessorPrefix = mLanguageDefinition->mPreprocessorPrefix;
	bool multilineStrings = mLanguageDefinition->mMultilineStrings;
	bool nestedComments = mLanguageDefinition->mNestedComments;

	eastl::string buffer;

	int commentDepth = 0;
	bool inSingleLineComment = false;
	int nonSpaceChars = 0;
	char stringOpenChar = 0;

	commentLineDepths.resize(mLines.size());

	for (int index = from; index <= to; index++)
	{
		auto& line = mLines[index];

		if (line.empty())
			continue;

		int lineLen = min(2000, (int)line.size());

		if (line.size() + 1 > buffer.size())
			buffer.resize(line.size() + 1);

		for (size_t j = 0; j < line.size(); ++j)
		{
			auto& col = line[j];
			buffer[j] = col.mChar;
			col.mColorIndex = PaletteIndex::Default;
		}
		buffer[line.size()] = 0;

		if (!mLanguageDefinition->mTokenize)
			continue;

		if (limitedRange)
			commentDepth = commentLineDepths[index];
		else
			commentLineDepths[index] = eastl::min(commentDepth, 255);

		for (int i = 0; i < lineLen; ++i)
		{
			if (nonSpaceChars < 2 && buffer[i] != ' ' && buffer[i] != '\t')
				nonSpaceChars++;

			if (stringOpenChar)
			{
				line[i].mColorIndex = PaletteIndex::String;

				if (buffer[i] == '\\')
				{
					i++;
					if (i < line.size())
						line[i].mColorIndex = PaletteIndex::String;
					continue;
				}

				if (buffer[i] == stringOpenChar)
					stringOpenChar = 0;
			}
			else
			{
				if (commentDepth == 0 && (buffer[i] == '\"' || buffer[i] == '\''))
				{
					stringOpenChar = buffer[i];
					line[i].mColorIndex = PaletteIndex::String;
					continue;
				}

				if (!commentLine.empty() && buffer[i] == commentLine[0] && equal_token(&buffer[i], commentLine.c_str()))
				{
					for (int j = i; j < line.size(); j++)
						line[j].mColorIndex = PaletteIndex::Comment;
					break;
				}

				if (nonSpaceChars == 1 && !preprocessorPrefix.empty() && buffer[i] == preprocessorPrefix[0] && equal_token(&buffer[i], preprocessorPrefix.c_str()))
				{
					for (int j = i; j < line.size(); j++)
						line[j].mColorIndex = PaletteIndex::Preprocessor;
					break;
				}

				if (!commentStart.empty() && buffer[i] == commentStart[0] && equal_token(&buffer[i], commentStart.c_str()))
				{
					commentDepth = nestedComments ? commentDepth + 1 : 1;
					int j = i;
					for (; j < i + commentStart.length(); j++)
						line[j].mColorIndex = PaletteIndex::Comment;
					i = j - 1;
					continue;
				}

				if (commentDepth > 0 && buffer[i] == commentEnd[0] && equal_token(&buffer[i], commentEnd.c_str()))
				{
					commentDepth--;

					int j = i;
					for (; j < i + commentStart.length(); j++)
						line[j].mColorIndex = PaletteIndex::Comment;
					i = j - 1;
					continue;
				}

				if (commentDepth > 0)
				{
					line[i].mColorIndex = PaletteIndex::Comment;
				}
			}

			const char * bufferBegin = &buffer.front();
			const char * bufferEnd = bufferBegin + line.size();
			const char * last = bufferEnd;

			for (auto first = bufferBegin; first != last; )
			{
				const char* token_begin = nullptr;
				const char* token_end = nullptr;
				PaletteIndex token_color = PaletteIndex::Default;

				if (line[first - bufferBegin].mColorIndex != PaletteIndex::Default)
				{
					first++;
					continue;
				}

				if (!mLanguageDefinition->mTokenize(first, last, token_begin, token_end, token_color))
				{
					first++;
				}
				else
				{
					const size_t token_length = token_end - token_begin;

					if (token_color == PaletteIndex::Identifier)
					{
						eastl::string id(token_begin, token_length);

						if (!mLanguageDefinition->mCaseSensitive)
							eastl::transform(id.begin(), id.end(), id.begin(), ::toupper);

						if (mLanguageDefinition->mKeywords.count(id) != 0)
							token_color = PaletteIndex::Keyword;
						else if (mLanguageDefinition->mIdentifiers.count(id) != 0)
							token_color = PaletteIndex::KnownIdentifier;
					}

					for (size_t j = 0; j < token_length; ++j)
						line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

					first = token_end;
				}
			}

			if (!multilineStrings || stringOpenChar == '\'')
				stringOpenChar = 0;
		}
	}
}


const TextEditor::Palette& TextEditor::GetDarkPalette()
{
	const static Palette p = { {
			0xdcdfe4ff,	// Default
			0xe06c75ff,	// Keyword
			0xe5c07bff,	// Number
			0x98c379ff,	// String
			0xe0a070ff, // Char literal
			0x6a93A4ff, // Punctuation
			0x808040ff,	// Preprocessor
			0xdcdfe4ff, // Identifier
			0x61afefff, // Known identifier
			0xc678ddff, // Preproc identifier
			0x3696a2ff, // Comment (single line)
			0x3696a2ff, // Comment (multi line)
			0x282c34ff, // Background
			0xe0e0e0ff, // Cursor
			0x2060a080, // Selection
			0xff200080, // ErrorMarker
			0xffffff15, // ControlCharacter
			0x0080f040, // Breakpoint
			0x7a8394ff, // Line number
			0x00000040, // Current line fill
			0x80808040, // Current line fill (inactive)
			0xa0a0a040, // Current line edge
			0xff406080, // Highlight fill
		} };
	return p;
}

const TextEditor::Palette& TextEditor::GetMarianaPalette()
{
	const static Palette p = { {
			0xffffffff,	// Default
			0xc695c6ff,	// Keyword
			0xf9ae58ff,	// Number
			0x99c794ff,	// String
			0xe0a070ff, // Char literal
			0x7fc4ccff, // Punctuation
			0x808040ff,	// Preprocessor
			0xffffffff, // Identifier
			0x4dc69bff, // Known identifier
			0xe0a0ffff, // Preproc identifier
			0xa6acb9ff, // Comment (single line)
			0xa6acb9ff, // Comment (multi line)
			0x303841ff, // Background
			0xe0e0e0ff, // Cursor
			0x6e7a8580, // Selection
			0xec5f6680, // ErrorMarker
			0xffffff30, // ControlCharacter
			0x0080f040, // Breakpoint
			0xffffffb0, // Line number
			0x4e5a6580, // Current line fill
			0x4e5a6530, // Current line fill (inactive)
			0x4e5a65b0, // Current line edge
			0xff406080, // Highlight fill
		} };
	return p;
}

const TextEditor::Palette& TextEditor::GetLightPalette()
{
	const static Palette p = { {
			0x404040ff,	// None
			0x060cffff,	// Keyword
			0x008000ff,	// Number
			0xa02020ff,	// String
			0x704030ff, // Char literal
			0x000000ff, // Punctuation
			0x606040ff,	// Preprocessor
			0x404040ff, // Identifier
			0x106060ff, // Known identifier
			0xa040c0ff, // Preproc identifier
			0x205020ff, // Comment (single line)
			0x205040ff, // Comment (multi line)
			0xffffffff, // Background
			0x000000ff, // Cursor
			0x00006040, // Selection
			0xff1000a0, // ErrorMarker
			0x90909090, // ControlCharacter
			0x0080f080, // Breakpoint
			0x005050ff, // Line number
			0x00000040, // Current line fill
			0x80808040, // Current line fill (inactive)
			0x00000040, // Current line edge
			0xffA0A0A0, // Highlight fill
		} };
	return p;
}

const TextEditor::Palette& TextEditor::GetRetroBluePalette()
{
	const static Palette p = { {
			0xffff00ff,	// None
			0x00ffffff,	// Keyword
			0x00ff00ff,	// Number
			0x008080ff,	// String
			0x008080ff, // Char literal
			0xffffffff, // Punctuation
			0x008000ff,	// Preprocessor
			0xffff00ff, // Identifier
			0xffffffff, // Known identifier
			0xff00ffff, // Preproc identifier
			0x808080ff, // Comment (single line)
			0x404040ff, // Comment (multi line)
			0x000080ff, // Background
			0xff8000ff, // Cursor
			0x00ffff80, // Selection
			0xff0000a0, // ErrorMarker
			0x0080ff80, // Breakpoint
			0xe0e0e0ff, // Line number
			0x00000040, // Current line fill
			0x80808040, // Current line fill (inactive)
			0xe0e0e040, // Current line edge
			0xff404080, // Highlight fill
		} };
	return p;
}

const eastl::unordered_map<char, char> TextEditor::OPEN_TO_CLOSE_CHAR = {
	{'{', '}'},
	{'(' , ')'},
	{'[' , ']'}
};
const eastl::unordered_map<char, char> TextEditor::CLOSE_TO_OPEN_CHAR = {
	{'}', '{'},
	{')' , '('},
	{']' , '['}
};

TextEditor::PaletteId TextEditor::defaultPalette = TextEditor::PaletteId::Dark;