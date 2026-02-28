/*
    File: ui/models/IniFilterProxyModel.cpp
    Purpose:
      - Implements proxy filtering/highlighting over settings table rows.

    How it fits in the codebase:
      - Wraps IniSettingsTableModel and is assigned directly to the main table view.
      - Emits match stats used by MainWindow status labels.
*/

#include "ui/models/IniFilterProxyModel.h"

#include "ui/models/IniSettingsTableModel.h"

#include <QBrush>
#include <QColor>

IniFilterProxyModel::IniFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void IniFilterProxyModel::setSearchOptions(const SearchOptions& options) {
    // Store new options, re-run filter, then emit updated visible row stats.
    options_ = options;
    invalidateFilter();
    emitFilterStats();
}

SearchOptions IniFilterProxyModel::searchOptions() const { return options_; }

bool IniFilterProxyModel::isRegexValid() const { return regexValid_; }

bool IniFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    // Build a lightweight entry from source model columns.
    const auto modelIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString section = sourceModel()->data(modelIndex, Qt::DisplayRole).toString();
    const QString key = sourceModel()->data(sourceModel()->index(sourceRow, 1, sourceParent), Qt::DisplayRole).toString();
    const QString value =
        sourceModel()->data(sourceModel()->index(sourceRow, 2, sourceParent), Qt::DisplayRole).toString();
    IniSettingEntry entry{
        .lineIndex = sourceRow,
        .sourceLine = sourceRow + 1,
        .section = section,
        .key = key,
        .value = value,
    };

    // Delegate decision to reusable search service.
    bool regexValid = true;
    const bool match = searchService_.matches(entry, options_, &regexValid);
    regexValid_ = regexValid;
    return match;
}

QVariant IniFilterProxyModel::data(const QModelIndex& index, int role) const {
    // Optional row highlight whenever query is non-empty.
    if (role == Qt::BackgroundRole && !options_.query.isEmpty()) {
        return QBrush(QColor(255, 247, 204));
    }
    return QSortFilterProxyModel::data(index, role);
}

void IniFilterProxyModel::emitFilterStats() { emit filterApplied(rowCount(), regexValid_); }
