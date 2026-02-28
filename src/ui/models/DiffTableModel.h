/*
    File: ui/models/DiffTableModel.h
    Purpose:
      - Read-only table model for semantic diff output.

    How it fits in the codebase:
      - Populated by MainWindow when async diff completes.
      - Rendered in the Diff tab table view.
*/

#pragma once

#include "domain/IniTypes.h"

#include <QAbstractTableModel>

class DiffTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit DiffTableModel(QObject* parent = nullptr);

    // Replace all rows with newly computed diff output.
    void setItems(QVector<SemanticDiffItem> items);
    const QVector<SemanticDiffItem>& items() const;

    // QAbstractTableModel contract
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<SemanticDiffItem> items_;
};
