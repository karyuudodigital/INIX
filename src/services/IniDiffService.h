#pragma once

#include "domain/IniDocument.h"
#include "domain/IniTypes.h"

#include <QVector>

class IniDiffService {
public:
    TextDiffSummary buildTextSummary(const IniDocument& baseDoc, const IniDocument& compareDoc) const;
    QVector<SemanticDiffItem> buildSemanticDiff(const IniDocument& baseDoc,
                                                const IniDocument& compareDoc,
                                                bool caseInsensitiveKeys) const;
};

