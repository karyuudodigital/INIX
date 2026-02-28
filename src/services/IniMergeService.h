#pragma once

#include "domain/IniDocument.h"
#include "domain/IniTypes.h"

#include <functional>

class IniMergeService {
public:
    QVector<MergePreviewItem> buildPreview(const QVector<SemanticDiffItem>& semanticDiff) const;

    MergeApplySummary applyPreview(IniDocument& target,
                                   const QVector<MergePreviewItem>& previewItems,
                                   MergeConflictPolicy policy,
                                   const std::function<bool(const MergePreviewItem&)>& promptCallback = {}) const;
};

