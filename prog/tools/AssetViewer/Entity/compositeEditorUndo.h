#include <libTools/util/undo.h>
#include <EASTL/unique_ptr.h>

class DataBlock;

class CompositeEditorUndoParams : public UndoRedoObject
{
public:
  void saveUndo();

private:
  virtual void restore(bool save_redo) override;
  virtual void redo() override;
  virtual size_t size() override { return bufferSize; }
  virtual void accepted() override {}
  virtual void get_description(String &s) override { s = "CompositeEditorUndoParams"; }

  void loadUndo() const;
  void saveUndoFromDataBlock(const DataBlock &dataBlock);

  eastl::unique_ptr<uint8_t[]> buffer;
  size_t bufferSize = 0;
};