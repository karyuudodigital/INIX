#include "services/IniSearchService.h"

bool IniSearchService::matches(const IniSettingEntry& entry, const SearchOptions& options, bool* regexValid) const {
    if (options.query.isEmpty()) {
        if (regexValid) {
            *regexValid = true;
        }
        return true;
    }

    const QString haystack = entry.section + " " + entry.key + " " + entry.value;

    if (!options.regex) {
        const Qt::CaseSensitivity cs = options.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        if (regexValid) {
            *regexValid = true;
        }
        return haystack.contains(options.query, cs);
    }

    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;
    if (!options.caseSensitive) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }
    const QRegularExpression expr(options.query, patternOptions);
    if (regexValid) {
        *regexValid = expr.isValid();
    }
    if (!expr.isValid()) {
        return true;
    }
    return expr.match(haystack).hasMatch();
}

