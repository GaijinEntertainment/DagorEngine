#include "BlkCodeEditor.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>

BlkCodeEditor::BlkCodeEditor(QWidget *parent) : QPlainTextEdit(parent) {}

void BlkCodeEditor::setCompleter(QCompleter *completer)
{
  if (completionHelper)
    disconnect(completionHelper, nullptr, this, nullptr);

  completionHelper = completer;

  if (!completionHelper)
    return;

  completionHelper->setWidget(this);
  completionHelper->setCompletionMode(QCompleter::PopupCompletion);
  completionHelper->setCaseSensitivity(Qt::CaseInsensitive);
  connect(completionHelper, QOverload<const QString &>::of(&QCompleter::activated), this, &BlkCodeEditor::insertCompletion);
}

void BlkCodeEditor::focusInEvent(QFocusEvent *event)
{
  if (completionHelper)
    completionHelper->setWidget(this);
  QPlainTextEdit::focusInEvent(event);
}

void BlkCodeEditor::keyPressEvent(QKeyEvent *event)
{
  if (completionHelper && completionHelper->popup()->isVisible())
  {
    switch (event->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
      event->ignore();
      return;
    default:
      break;
    }
  }

  const bool ctrlOrShift = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
  const bool shortcutOverride = ctrlOrShift && event->key() == Qt::Key_Space;

  QPlainTextEdit::keyPressEvent(event);

  if (!completionHelper)
    return;

  const bool hasModifier = event->modifiers() & (Qt::ControlModifier | Qt::AltModifier);
  const QString completionPrefix = textUnderCursor();

  if (!shortcutOverride && (hasModifier || event->text().isEmpty() || completionPrefix.length() < 2))
  {
    completionHelper->popup()->hide();
    return;
  }

  if (completionPrefix != completionHelper->completionPrefix())
  {
    completionHelper->setCompletionPrefix(completionPrefix);
    completionHelper->popup()->setCurrentIndex(completionHelper->completionModel()->index(0, 0));
  }

  QRect cursorRect = this->cursorRect();
  cursorRect.setWidth(completionHelper->popup()->sizeHintForColumn(0) + completionHelper->popup()->verticalScrollBar()->sizeHint().width());
  completionHelper->complete(cursorRect);
}

void BlkCodeEditor::insertCompletion(const QString &completion)
{
  if (!completionHelper || completionHelper->widget() != this)
    return;

  QTextCursor cursor = textCursor();
  const int prefixLength = completionHelper->completionPrefix().length();
  cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, prefixLength);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefixLength);
  cursor.removeSelectedText();
  cursor.insertText(completion);
  setTextCursor(cursor);
}

QString BlkCodeEditor::textUnderCursor() const
{
  QTextCursor cursor = textCursor();
  const QTextBlock block = cursor.block();
  const int relativePos = cursor.position() - block.position();
  const QString text = block.text();
  if (text.isEmpty())
    return QString();

  int start = relativePos;
  while (start > 0)
  {
    const QChar ch = text.at(start - 1);
    if (!(ch.isLetterOrNumber() || ch == '_' || ch == '#' || ch == '@' || ch == '/' || ch == '.' || ch == '-'))
      break;
    --start;
  }

  int end = relativePos;
  while (end < text.size())
  {
    const QChar ch = text.at(end);
    if (!(ch.isLetterOrNumber() || ch == '_' || ch == '#' || ch == '@' || ch == '/' || ch == '.' || ch == '-'))
      break;
    ++end;
  }

  return text.mid(start, end - start);
}
