#pragma once

#include "domain/IniTypes.h"

#include <QAbstractTableModel>

class DiffTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit DiffTableModel(QObject* parent = nullptr);

    void setItems(QVector<SemanticDiffItem> items);
    const QVector<SemanticDiffItem>& items() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<SemanticDiffItem> items_;
};

