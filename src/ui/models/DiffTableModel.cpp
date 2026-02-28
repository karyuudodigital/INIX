#include "ui/models/DiffTableModel.h"

#include <QBrush>
#include <QColor>

namespace {
QString statusToString(SemanticDiffStatus status) {
    switch (status) {
    case SemanticDiffStatus::Added:
        return "Added";
    case SemanticDiffStatus::Removed:
        return "Removed";
    case SemanticDiffStatus::Changed:
        return "Changed";
    case SemanticDiffStatus::Unchanged:
        return "Unchanged";
    }
    return "Unknown";
}
} // namespace

DiffTableModel::DiffTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void DiffTableModel::setItems(QVector<SemanticDiffItem> items) {
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
        return "Status";
    case 1:
        return "Section";
    case 2:
        return "Key";
    case 3:
        return "Base Value";
    case 4:
        return "Compare Value";
    default:
        return {};
    }
}

