/*
    File: services/IniDiffService.h
    Purpose:
      - Declares raw text summary diff and semantic key/value diff operations.

    How it fits in the codebase:
      - MainWindow uses this service in runDiffAsync().
      - Diff output is rendered by DiffTableModel and also feeds merge preview generation.
*/

#pragma once

#include "domain/IniDocument.h"
#include "domain/IniTypes.h"

#include <QVector>

class IniDiffService {
public:
    // Positional line summary for quick "added/removed/changed" counters.
    TextDiffSummary buildTextSummary(const IniDocument& baseDoc, const IniDocument& compareDoc) const;
    // Semantic diff over (section,key), optionally case-insensitive.
    QVector<SemanticDiffItem> buildSemanticDiff(const IniDocument& baseDoc,
                                                const IniDocument& compareDoc,
                                                bool caseInsensitiveKeys) const;
};
