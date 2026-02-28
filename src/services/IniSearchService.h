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
    bool matches(const IniSettingEntry& entry, const SearchOptions& options, bool* regexValid = nullptr) const;
};

