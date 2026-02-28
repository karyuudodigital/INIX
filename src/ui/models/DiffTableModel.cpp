/*
    File: ui/models/DiffTableModel.cpp
    Purpose:
      - Implements a read-only model for semantic diff rows.

    How it fits in the codebase:
      - MainWindow sets rows after async diff finishes.
      - Diff tab table reads display text and row background color from this model.
*/

#include "ui/models/DiffTableModel.h"

#include <QBrush>
#include <QColor>
#include <QCoreApplication>

namespace {
// Convert enum status to user-facing table text.
QString statusToString(SemanticDiffStatus status) {
    switch (status) {
    case SemanticDiffStatus::Added:
        return QCoreApplication::translate("DiffTableModel", "Added");
    case SemanticDiffStatus::Removed:
        return QCoreApplication::translate("DiffTableModel", "Removed");
    case SemanticDiffStatus::Changed:
        return QCoreApplication::translate("DiffTableModel", "Changed");
    case SemanticDiffStatus::Unchanged:
        return QCoreApplication::translate("DiffTableModel", "Unchanged");
    }
    return QCoreApplication::translate("DiffTableModel", "Unknown");
}
} // namespace

DiffTableModel::DiffTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void DiffTableModel::setItems(QVector<SemanticDiffItem> items) {
    // Reset model to publish wholesale row replacement to connected views.
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}

const QVector<SemanticDiffItem>& DiffTableModel::items() const { return items_; }

int DiffTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

int DiffTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 5;
}

QVariant DiffTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const auto& item = items_[index.row()];
    // DisplayRole defines cell text by column.
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return statusToString(item.status);
        case 1:
            return item.section;
        case 2:
            return item.key;
        case 3:
            return item.baseValue;
        case 4:
            return item.compareValue;
        default:
            return {};
        }
    }
    // BackgroundRole adds semantic coloring for quick visual scanning.
    if (role == Qt::BackgroundRole) {
        if (item.status == SemanticDiffStatus::Added) {
            return QBrush(QColor(220, 255, 220));
        }
        if (item.status == SemanticDiffStatus::Removed) {
            return QBrush(QColor(255, 220, 220));
        }
        if (item.status == SemanticDiffStatus::Changed) {
            return QBrush(QColor(255, 242, 204));
        }
    }
    return {};
}

QVariant DiffTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case 0:
        return tr("Status");
    case 1:
        return tr("Section");
    case 2:
        return tr("Key");
    case 3:
        return tr("Base Value");
    case 4:
        return tr("Compare Value");
    default:
        return {};
    }
}
