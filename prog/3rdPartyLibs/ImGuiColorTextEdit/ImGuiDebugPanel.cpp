#include "TextEditor.h"

void TextEditor::ImGuiDebugPanel(const eastl::string& panelName)
{
	ImGui::Begin(panelName.c_str());

	if (ImGui::CollapsingHeader("Editor state info"))
	{
		ImGui::Checkbox("Panning", &mPanning);
		ImGui::Checkbox("Dragging selection", &mDraggingSelection);
		ImGui::DragInt("Cursor count", &mState.mCurrentCursor);
		for (int i = 0; i <= mState.mCurrentCursor; i++)
		{
			ImGui::DragInt2("Interactive start", &mState.mCursors[i].mInteractiveStart.mLine);
			ImGui::DragInt2("Interactive end", &mState.mCursors[i].mInteractiveEnd.mLine);
		}
	}
	if (ImGui::CollapsingHeader("Lines"))
	{
		for (int i = 0; i < mLines.size(); i++)
		{
			ImGui::Text("%d", int(mLines[i].size()));
		}
	}
	if (ImGui::CollapsingHeader("Undo"))
	{
		static eastl::string numberOfRecordsText;
		numberOfRecordsText = "Number of records: " + eastl::to_string(mUndoBuffer.size());
		ImGui::Text("%s", numberOfRecordsText.c_str());
		ImGui::DragInt("Undo index", &mState.mCurrentCursor);
		for (int i = 0; i < mUndoBuffer.size(); i++)
		{
			if (ImGui::CollapsingHeader(eastl::to_string(i).c_str()))
			{

				ImGui::Text("Operations");
				for (int j = 0; j < mUndoBuffer[i].mOperations.size(); j++)
				{
					ImGui::Text("%s", mUndoBuffer[i].mOperations[j].mText.c_str());
					ImGui::Text(mUndoBuffer[i].mOperations[j].mType == UndoOperationType::Add ? "Add" : "Delete");
					ImGui::DragInt2("Start", &mUndoBuffer[i].mOperations[j].mStart.mLine);
					ImGui::DragInt2("End", &mUndoBuffer[i].mOperations[j].mEnd.mLine);
					ImGui::Separator();
				}
			}
		}
	}
	ImGui::End();
}