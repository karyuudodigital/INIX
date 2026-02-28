/*
    File: services/IniMergeService.h
    Purpose:
      - Declares merge preview generation and selected-row merge application.

    How it fits in the codebase:
      - Consumes semantic diff rows from IniDiffService.
      - Produces merge rows shown by MergePreviewTableModel.
      - Applies accepted rows back into IniDocument through upsertSetting().
*/

#pragma once

#include "domain/IniDocument.h"
#include "domain/IniTypes.h"

#include <functional>

class IniMergeService {
public:
    // Convert semantic diff rows into user-selectable merge preview items.
    QVector<MergePreviewItem> buildPreview(const QVector<SemanticDiffItem>& semanticDiff) const;

    // Apply selected preview rows to target document under a conflict policy.
    // promptCallback is only used for PromptPerConflict mode.
    MergeApplySummary applyPreview(IniDocument& target,
                                   const QVector<MergePreviewItem>& previewItems,
                                   MergeConflictPolicy policy,
                                   const std::function<bool(const MergePreviewItem&)>& promptCallback = {}) const;
};
