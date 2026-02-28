#include "services/IniMergeService.h"

QVector<MergePreviewItem> IniMergeService::buildPreview(const QVector<SemanticDiffItem>& semanticDiff) const {
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
        if (!item.selected) {
            ++summary.skipped;
            continue;
        }
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
            target.upsertSetting(item.section, item.key, item.sourceValue);
        }
    }
    return summary;
}
