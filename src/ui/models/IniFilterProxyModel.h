/*
    File: ui/models/IniFilterProxyModel.h
    Purpose:
      - QSortFilterProxyModel that applies search/filter rules to settings rows.

    How it fits in the codebase:
      - Sits between IniSettingsTableModel and the main settings QTableView.
      - Uses IniSearchService to evaluate matches and exposes filter stats to MainWindow.
*/

#pragma once

#include "services/IniSearchService.h"

#include <QSortFilterProxyModel>

class IniFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit IniFilterProxyModel(QObject* parent = nullptr);

    // Replaces current filter options and re-runs filtering.
    void setSearchOptions(const SearchOptions& options);
    SearchOptions searchOptions() const;
    bool isRegexValid() const;

signals:
    // Emitted after a filter run with visible row count and regex validity state.
    void filterApplied(int visibleCount, bool regexValid);

protected:
    // QSortFilterProxyModel hooks
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    void emitFilterStats();

    SearchOptions options_;
    mutable bool regexValid_ = true;
    IniSearchService searchService_;
};
