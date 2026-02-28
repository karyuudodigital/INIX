/*
    File: services/IniSearchService.cpp
    Purpose:
      - Implements matching logic used by table filtering.

    How it fits in the codebase:
      - IniFilterProxyModel constructs row entries and delegates matching here.
      - Keeps regex/case behavior independent from Qt view code.
*/

#include "services/IniSearchService.h"

bool IniSearchService::matches(const IniSettingEntry& entry, const SearchOptions& options, bool* regexValid) const {
    // Empty query means "show everything".
    if (options.query.isEmpty()) {
        if (regexValid) {
            *regexValid = true;
        }
        return true;
    }

    // Search across a single combined text buffer for simple matching semantics.
    const QString haystack = entry.section + " " + entry.key + " " + entry.value;

    // Plain substring path.
    if (!options.regex) {
        const Qt::CaseSensitivity cs = options.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        if (regexValid) {
            *regexValid = true;
        }
        return haystack.contains(options.query, cs);
    }

    // Regex path with dynamic case option.
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;
    if (!options.caseSensitive) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }
    const QRegularExpression expr(options.query, patternOptions);
    if (regexValid) {
        *regexValid = expr.isValid();
    }
    // Invalid regex is treated as non-blocking (keep rows visible) while reporting validity.
    if (!expr.isValid()) {
        return true;
    }
    return expr.match(haystack).hasMatch();
}
