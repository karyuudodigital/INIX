#pragma once

#include "services/IniSearchService.h"

#include <QSortFilterProxyModel>

class IniFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit IniFilterProxyModel(QObject* parent = nullptr);

    void setSearchOptions(const SearchOptions& options);
    SearchOptions searchOptions() const;
    bool isRegexValid() const;

signals:
    void filterApplied(int visibleCount, bool regexValid);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    void emitFilterStats();

    SearchOptions options_;
    mutable bool regexValid_ = true;
    IniSearchService searchService_;
};
