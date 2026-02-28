#pragma once

#include <QString>
#include <QVector>

enum class IniLineType {
    Section,
    KeyValue,
    Comment,
    Blank,
    Malformed
};

enum class IniEncoding {
    Utf8Bom,
    Utf8,
    Local8Bit
};

struct IniLine {
    IniLineType type = IniLineType::Blank;
    QString rawText;
    QString section;
    QString key;
    QString value;
    int sourceLine = 0;
};

struct IniSettingEntry {
    int lineIndex = -1;
    int sourceLine = 0;
    QString section;
    QString key;
    QString value;
};

struct TextDiffSummary {
    int addedLines = 0;
    int removedLines = 0;
    int changedLines = 0;
};

enum class SemanticDiffStatus {
    Added,
    Removed,
    Changed,
    Unchanged
};

struct SemanticDiffItem {
    QString section;
    QString key;
    QString baseValue;
    QString compareValue;
    SemanticDiffStatus status = SemanticDiffStatus::Unchanged;
};

enum class MergeConflictPolicy {
    ReplaceTarget,
    KeepTarget,
    PromptPerConflict
};

struct MergePreviewItem {
    bool selected = false;
    QString section;
    QString key;
    QString sourceValue;
    QString targetValue;
    SemanticDiffStatus status = SemanticDiffStatus::Unchanged;
};

struct MergeApplySummary {
    int replaced = 0;
    int kept = 0;
    int added = 0;
    int skipped = 0;
};

