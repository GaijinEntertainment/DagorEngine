#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QVector>

class BlkSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

public:
  explicit BlkSyntaxHighlighter(QTextDocument *parent = nullptr);

  void setKeywordList(const QStringList &keywords);
  void setDirectiveList(const QStringList &directives);

protected:
  void highlightBlock(const QString &text) override;

private:
  struct HighlightRule
  {
    QRegularExpression pattern;
    QTextCharFormat format;
  };

  void rebuildRules();

  QVector<HighlightRule> rules;
  QStringList keywordList;
  QStringList directiveList;

  QTextCharFormat keywordFormat;
  QTextCharFormat directiveFormat;
  QTextCharFormat typeFormat;
  QTextCharFormat valueFormat;
  QTextCharFormat commentFormat;
  QTextCharFormat includeFormat;
  QTextCharFormat blockFormat;
};
