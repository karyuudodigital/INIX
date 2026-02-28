/*
    File: services/IniMergeService.cpp
    Purpose:
      - Implements merge preview creation and merge application policies.

    How it fits in the codebase:
      - Input comes from semantic diff output.
      - Output feeds MergePreviewTableModel and then writes into IniDocument.
*/

#include "services/IniMergeService.h"

QVector<MergePreviewItem> IniMergeService::buildPreview(const QVector<SemanticDiffItem>& semanticDiff) const {
    // Remove rows that do not represent actionable changes for merge.
    QVector<MergePreviewItem> preview;
    preview.reserve(semanticDiff.size());
    for (const auto& item : semanticDiff) {
        if (item.status == SemanticDiffStatus::Removed || item.status == SemanticDiffStatus::Unchanged) {
            continue;
        }
        preview.push_back(MergePreviewItem{
            .selected = (item.status == SemanticDiffStatus::Added || item.status == SemanticDiffStatus::Changed),
            .section = item.section,
            .key = item.key,
            .sourceValue = item.compareValue,
            .targetValue = item.baseValue,
            .status = item.status,
        });
    }
    return preview;
}

MergeApplySummary IniMergeService::applyPreview(IniDocument& target,
                                                const QVector<MergePreviewItem>& previewItems,
                                                MergeConflictPolicy policy,
                                                const std::function<bool(const MergePreviewItem&)>& promptCallback) const {
    MergeApplySummary summary;
    for (const auto& item : previewItems) {
        // Unchecked rows are counted as skipped and never applied.
        if (!item.selected) {
            ++summary.skipped;
            continue;
        }

        // "Changed with different values" is treated as a conflict candidate.
        const bool conflict = item.status == SemanticDiffStatus::Changed && item.targetValue != item.sourceValue;
        bool shouldReplace = false;
        if (item.status == SemanticDiffStatus::Added) {
            shouldReplace = true;
            ++summary.added;
        } else if (!conflict) {
            shouldReplace = true;
            ++summary.replaced;
        } else {
            switch (policy) {
            case MergeConflictPolicy::ReplaceTarget:
                shouldReplace = true;
                ++summary.replaced;
                break;
            case MergeConflictPolicy::KeepTarget:
                shouldReplace = false;
                ++summary.kept;
                break;
            case MergeConflictPolicy::PromptPerConflict:
                shouldReplace = promptCallback ? promptCallback(item) : false;
                if (shouldReplace) {
                    ++summary.replaced;
                } else {
                    ++summary.kept;
                }
                break;
            }
        }
        if (shouldReplace) {
            // upsertSetting performs update-if-exists or add-if-missing.
            target.upsertSetting(item.section, item.key, item.sourceValue);
        }
    }
    return summary;
}
