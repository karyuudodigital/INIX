/*
    File: domain/IniTypes.h
    Purpose:
      - Defines shared value types used across domain, services, and UI models.

    How it fits in the codebase:
      - Keeps cross-module contracts in one place to avoid circular dependencies.
      - Services return these types; models render these types; MainWindow coordinates them.
*/

#pragma once

#include <QString>
#include <QVector>

// Classifies each original file line after parsing.
enum class IniLineType {
    Section,
    KeyValue,
    Comment,
    Blank,
    Malformed
};

// Encoding mode preserved from input and reused during save.
enum class IniEncoding {
    Utf8Bom,
    Utf8,
    Local8Bit
};

// Full-fidelity parsed line representation.
// rawText keeps original text for comments/malformed lines and formatting preservation.
struct IniLine {
    IniLineType type = IniLineType::Blank;
    QString rawText;
    QString section;
    QString key;
    QString value;
    int sourceLine = 0;
};

// Flat table row used by table models to display editable settings.
struct IniSettingEntry {
    int lineIndex = -1;
    int sourceLine = 0;
    QString section;
    QString key;
    QString value;
};

// Fast positional text-diff summary (not a full patch).
struct TextDiffSummary {
    int addedLines = 0;
    int removedLines = 0;
    int changedLines = 0;
};

// Semantic relationship for one (section,key) pair between base and compare docs.
enum class SemanticDiffStatus {
    Added,
    Removed,
    Changed,
    Unchanged
};

// One semantic diff row shown in the Diff tab.
struct SemanticDiffItem {
    QString section;
    QString key;
    QString baseValue;
    QString compareValue;
    SemanticDiffStatus status = SemanticDiffStatus::Unchanged;
};

// Conflict policy used when applying selected merge rows.
enum class MergeConflictPolicy {
    ReplaceTarget,
    KeepTarget,
    PromptPerConflict
};

// One selectable row in merge preview.
struct MergePreviewItem {
    bool selected = false;
    QString section;
    QString key;
    QString sourceValue;
    QString targetValue;
    SemanticDiffStatus status = SemanticDiffStatus::Unchanged;
};

// Aggregate counters returned after merge apply.
struct MergeApplySummary {
    int replaced = 0;
    int kept = 0;
    int added = 0;
    int skipped = 0;
};
