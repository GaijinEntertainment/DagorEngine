#pragma once

#include <QPlainTextEdit>

class QCompleter;

class BlkCodeEditor : public QPlainTextEdit
{
  Q_OBJECT

public:
  explicit BlkCodeEditor(QWidget *parent = nullptr);

  void setCompleter(QCompleter *completer);
  QCompleter *completer() const { return completionHelper; }

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;

private slots:
  void insertCompletion(const QString &completion);

private:
  QString textUnderCursor() const;

  QCompleter *completionHelper = nullptr;
};
