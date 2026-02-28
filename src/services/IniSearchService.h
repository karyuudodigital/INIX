/*
    File: services/IniSearchService.h
    Purpose:
      - Declares row-level search matching logic used by filtering UI models.

    How it fits in the codebase:
      - IniFilterProxyModel delegates all match decisions here.
      - Keeps regex/case behavior centralized and testable outside UI classes.
*/

#pragma once

#include "domain/IniTypes.h"

#include <QRegularExpression>

struct SearchOptions {
    QString query;
    bool caseSensitive = false;
    bool regex = false;
};

class IniSearchService {
public:
    // Returns true when the entry should remain visible.
    // regexValid is optional output for UI feedback on invalid regex input.
    bool matches(const IniSettingEntry& entry, const SearchOptions& options, bool* regexValid = nullptr) const;
};
