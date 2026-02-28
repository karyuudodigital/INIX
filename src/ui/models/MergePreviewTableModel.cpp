/*
    File: ui/models/MergePreviewTableModel.cpp
    Purpose:
      - Implements merge preview rows with checkbox-based selection.

    How it fits in the codebase:
      - Populated from IniMergeService preview output.
      - Edited by user selection controls in MainWindow.
      - Consumed by apply merge operation.
*/

#include "ui/models/MergePreviewTableModel.h"

#include <QCoreApplication>

namespace {
// Convert enum status to user-facing table text.
QString statusToString(SemanticDiffStatus status) {
    switch (status) {
    case SemanticDiffStatus::Added:
        return QCoreApplication::translate("MergePreviewTableModel", "Added");
    case SemanticDiffStatus::Removed:
        return QCoreApplication::translate("MergePreviewTableModel", "Removed");
    case SemanticDiffStatus::Changed:
        return QCoreApplication::translate("MergePreviewTableModel", "Changed");
    case SemanticDiffStatus::Unchanged:
        return QCoreApplication::translate("MergePreviewTableModel", "Unchanged");
    }
    return QCoreApplication::translate("MergePreviewTableModel", "Unknown");
}
} // namespace

MergePreviewTableModel::MergePreviewTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void MergePreviewTableModel::setItems(QVector<MergePreviewItem> items) {
    // Reset model after replacing row vector.
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}

const QVector<MergePreviewItem>& MergePreviewTableModel::items() const { return items_; }

QVector<MergePreviewItem>& MergePreviewTableModel::itemsMutable() { return items_; }

int MergePreviewTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

int MergePreviewTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 6;
}

QVariant MergePreviewTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }
    const auto& item = items_[index.row()];
    // First column is checkbox state.
    if (role == Qt::CheckStateRole && index.column() == 0) {
        return item.selected ? Qt::Checked : Qt::Unchecked;
    }
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 1:
            return statusToString(item.status);
        case 2:
            return item.section;
        case 3:
            return item.key;
        case 4:
            return item.targetValue;
        case 5:
            return item.sourceValue;
        default:
            return {};
        }
    }
    return {};
}

QVariant MergePreviewTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case 0:
        return tr("Apply");
    case 1:
        return tr("Status");
    case 2:
        return tr("Section");
    case 3:
        return tr("Key");
    case 4:
        return tr("Target Value");
    case 5:
        return tr("Source Value");
    default:
        return {};
    }
}

Qt::ItemFlags MergePreviewTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsUserCheckable;
    }
    return f;
}

bool MergePreviewTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return false;
    }
    // Only checkbox column is editable.
    if (index.column() == 0 && role == Qt::CheckStateRole) {
        items_[index.row()].selected = value.toInt() == Qt::Checked;
        emit dataChanged(index, index, {Qt::CheckStateRole});
        return true;
    }
    return false;
}

void MergePreviewTableModel::selectByStatus(SemanticDiffStatus status) {
    // Batch-select only rows with matching semantic status.
    for (auto& item : items_) {
        item.selected = item.status == status;
    }
    if (!items_.isEmpty()) {
        emit dataChanged(index(0, 0), index(items_.size() - 1, 0), {Qt::CheckStateRole});
    }
}

void MergePreviewTableModel::selectAll(bool selected) {
    // Batch-select or clear all rows.
    for (auto& item : items_) {
        item.selected = selected;
    }
    if (!items_.isEmpty()) {
        emit dataChanged(index(0, 0), index(items_.size() - 1, 0), {Qt::CheckStateRole});
    }
}
