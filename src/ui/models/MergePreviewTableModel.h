#pragma once

#include "domain/IniTypes.h"

#include <QAbstractTableModel>

class MergePreviewTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit MergePreviewTableModel(QObject* parent = nullptr);

    void setItems(QVector<MergePreviewItem> items);
    const QVector<MergePreviewItem>& items() const;
    QVector<MergePreviewItem>& itemsMutable();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void selectByStatus(SemanticDiffStatus status);
    void selectAll(bool selected);

private:
    QVector<MergePreviewItem> items_;
};

