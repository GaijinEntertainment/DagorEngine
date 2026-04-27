#include "BlkSyntaxHighlighter.h"

#include <QColor>
#include <QFont>
#include <QRegularExpression>

namespace
{
const QColor kKeywordColor(0, 122, 204);
const QColor kDirectiveColor(160, 64, 0);
const QColor kTypeColor(120, 120, 120);
const QColor kValueColor(42, 161, 152);
const QColor kCommentColor(128, 128, 128);
const QColor kIncludeColor(0, 153, 102);
const QColor kBlockColor(193, 132, 1);
}

BlkSyntaxHighlighter::BlkSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
  keywordFormat.setForeground(kKeywordColor);
  keywordFormat.setFontWeight(QFont::Bold);

  directiveFormat.setForeground(kDirectiveColor);
  directiveFormat.setFontWeight(QFont::Bold);

  typeFormat.setForeground(kTypeColor);
  typeFormat.setFontItalic(true);

  valueFormat.setForeground(kValueColor);

  commentFormat.setForeground(kCommentColor);
  commentFormat.setFontItalic(true);

  includeFormat.setForeground(kIncludeColor);
  includeFormat.setFontWeight(QFont::Bold);

  blockFormat.setForeground(kBlockColor);
  blockFormat.setFontWeight(QFont::Bold);

  rebuildRules();
}

void BlkSyntaxHighlighter::setKeywordList(const QStringList &keywords)
{
  keywordList = keywords;
  rebuildRules();
}

void BlkSyntaxHighlighter::setDirectiveList(const QStringList &directives)
{
  directiveList = directives;
  rebuildRules();
}

void BlkSyntaxHighlighter::highlightBlock(const QString &text)
{
  for (const HighlightRule &rule : rules)
  {
    QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
    while (it.hasNext())
    {
      const QRegularExpressionMatch match = it.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }

  // Strings
  QRegularExpression stringPattern(R"("([^"\\]|\\.)*")");
  QRegularExpressionMatchIterator stringIter = stringPattern.globalMatch(text);
  while (stringIter.hasNext())
  {
    const QRegularExpressionMatch match = stringIter.next();
    setFormat(match.capturedStart(), match.capturedLength(), valueFormat);
  }

  // Single-line comments
  QRegularExpression singleLineComment(QStringLiteral("//[^\n]*"));
  QRegularExpressionMatchIterator singleCommentIter = singleLineComment.globalMatch(text);
  while (singleCommentIter.hasNext())
  {
    const QRegularExpressionMatch match = singleCommentIter.next();
    setFormat(match.capturedStart(), match.capturedLength(), commentFormat);
  }

  // Multi-line comments
  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = text.indexOf(QStringLiteral("/*"));
  else
    startIndex = 0;

  while (startIndex >= 0)
  {
    int endIndex = text.indexOf(QStringLiteral("*/"), startIndex + 2);
    int commentLength;
    if (endIndex == -1)
    {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    }
    else
    {
      commentLength = endIndex - startIndex + 2;
      setCurrentBlockState(0);
    }
    setFormat(startIndex, commentLength, commentFormat);
    startIndex = text.indexOf(QStringLiteral("/*"), startIndex + commentLength);
  }

  if (startIndex < 0)
    setCurrentBlockState(0);
}

void BlkSyntaxHighlighter::rebuildRules()
{
  rules.clear();

  // Block headers
  HighlightRule blockRule;
  blockRule.pattern = QRegularExpression(R"(^\s*([A-Za-z_][\w]*)\s*\{)");
  blockRule.format = blockFormat;
  rules.push_back(blockRule);

  // Parameter names (name:type)
  HighlightRule paramRule;
  paramRule.pattern = QRegularExpression(R"(^\s*([A-Za-z_][\w]*)\s*:)" );
  paramRule.format = keywordFormat;
  rules.push_back(paramRule);

  // Types after colon
  HighlightRule typeRule;
  typeRule.pattern = QRegularExpression(R"(:\s*([A-Za-z_][\w]*)\b)");
  typeRule.format = typeFormat;
  rules.push_back(typeRule);

  // Includes
  HighlightRule includeRule;
  includeRule.pattern = QRegularExpression((QStringLiteral(R"(@?include\s+[<#["']?([^>"'\s]+))")));
  includeRule.format = includeFormat;
  rules.push_back(includeRule);

  for (const QString &keyword : keywordList)
  {
    HighlightRule rule;
    rule.pattern = QRegularExpression(QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(keyword)));
    rule.format = keywordFormat;
    rules.push_back(rule);
  }

  for (const QString &directive : directiveList)
  {
    HighlightRule rule;
    rule.pattern = QRegularExpression(QStringLiteral("@%1").arg(QRegularExpression::escape(directive)));
    rule.format = directiveFormat;
    rules.push_back(rule);
  }
}
