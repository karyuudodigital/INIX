/*
    File: ui/models/MergePreviewTableModel.h
    Purpose:
      - Editable table model for merge preview rows with per-row checkbox selection.

    How it fits in the codebase:
      - Filled from IniMergeService::buildPreview().
      - User selection state is edited here and consumed by apply merge flow.
*/

#pragma once

#include "domain/IniTypes.h"

#include <QAbstractTableModel>

class MergePreviewTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit MergePreviewTableModel(QObject* parent = nullptr);

    // Replace all preview rows.
    void setItems(QVector<MergePreviewItem> items);
    const QVector<MergePreviewItem>& items() const;
    // Mutable access used by higher-level workflows when needed.
    QVector<MergePreviewItem>& itemsMutable();

    // QAbstractTableModel contract
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    // Batch selection helpers for toolbar/buttons.
    void selectByStatus(SemanticDiffStatus status);
    void selectAll(bool selected);

private:
    QVector<MergePreviewItem> items_;
};
