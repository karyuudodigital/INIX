#include "services/IniDiffService.h"

#include <algorithm>
#include <QHash>
#include <QSet>

namespace {
QString canonicalToken(const QString& section, const QString& key, bool caseInsensitive) {
    const QString s = caseInsensitive ? section.toLower() : section;
    const QString k = caseInsensitive ? key.toLower() : key;
    return s + "||" + k;
}
} // namespace

TextDiffSummary IniDiffService::buildTextSummary(const IniDocument& baseDoc, const IniDocument& compareDoc) const {
    const auto& a = baseDoc.lines();
    const auto& b = compareDoc.lines();
    TextDiffSummary summary;
    const int shared = std::min(a.size(), b.size());
    for (int i = 0; i < shared; ++i) {
        if (a[i].rawText != b[i].rawText) {
            ++summary.changedLines;
        }
    }
    if (a.size() > b.size()) {
        summary.removedLines = a.size() - b.size();
    } else if (b.size() > a.size()) {
        summary.addedLines = b.size() - a.size();
    }
    return summary;
}

QVector<SemanticDiffItem> IniDiffService::buildSemanticDiff(const IniDocument& baseDoc,
                                                            const IniDocument& compareDoc,
                                                            bool caseInsensitiveKeys) const {
    struct Accumulator {
        SemanticDiffItem item;
        bool hasBase = false;
        bool hasCompare = false;
    };

    QHash<QString, Accumulator> table;
    QSet<QString> keys;

    for (const auto& entry : baseDoc.keyValueEntries()) {
        const QString token = canonicalToken(entry.section, entry.key, caseInsensitiveKeys);
        keys.insert(token);
        auto& acc = table[token];
        acc.item.section = entry.section;
        acc.item.key = entry.key;
        acc.item.baseValue = entry.value;
        acc.hasBase = true;
    }
    for (const auto& entry : compareDoc.keyValueEntries()) {
        const QString token = canonicalToken(entry.section, entry.key, caseInsensitiveKeys);
        keys.insert(token);
        auto& acc = table[token];
        acc.item.section = entry.section;
        acc.item.key = entry.key;
        acc.item.compareValue = entry.value;
        acc.hasCompare = true;
    }

    QVector<SemanticDiffItem> output;
    output.reserve(keys.size());
    for (const auto& token : keys) {
        const auto acc = table.value(token);
        auto item = acc.item;
        if (!acc.hasBase && acc.hasCompare) {
            item.status = SemanticDiffStatus::Added;
        } else if (acc.hasBase && !acc.hasCompare) {
            item.status = SemanticDiffStatus::Removed;
        } else if (item.baseValue != item.compareValue) {
            item.status = SemanticDiffStatus::Changed;
        } else {
            item.status = SemanticDiffStatus::Unchanged;
        }
        output.push_back(std::move(item));
    }

    std::sort(output.begin(), output.end(), [](const SemanticDiffItem& lhs, const SemanticDiffItem& rhs) {
        if (lhs.section.compare(rhs.section, Qt::CaseInsensitive) == 0) {
            return lhs.key.compare(rhs.key, Qt::CaseInsensitive) < 0;
        }
        return lhs.section.compare(rhs.section, Qt::CaseInsensitive) < 0;
    });
    return output;
}
