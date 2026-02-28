#include "ui/models/IniSettingsTableModel.h"

IniSettingsTableModel::IniSettingsTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void IniSettingsTableModel::setDocument(IniDocument* document) {
    if (document_ == document) {
        return;
    }
    if (document_) {
        disconnect(document_, nullptr, this, nullptr);
    }
    document_ = document;
    if (document_) {
        connect(document_, &IniDocument::changed, this, &IniSettingsTableModel::refresh);
    }
    refresh();
}

IniDocument* IniSettingsTableModel::document() const { return document_; }

int IniSettingsTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return entries_.size();
}

int IniSettingsTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant IniSettingsTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= entries_.size()) {
        return {};
    }
    const auto& entry = entries_[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case Section:
            return entry.section;
        case Key:
            return entry.key;
        case Value:
            return entry.value;
        case SourceLine:
            return entry.sourceLine;
        default:
            return {};
        }
    }
    return {};
}

QVariant IniSettingsTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case Section:
        return "Section";
    case Key:
        return "Key";
    case Value:
        return "Value";
    case SourceLine:
        return "Source line";
    default:
        return {};
    }
}

Qt::ItemFlags IniSettingsTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == Key || index.column() == Value) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}

bool IniSettingsTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!document_ || !index.isValid() || role != Qt::EditRole || index.row() < 0 || index.row() >= entries_.size()) {
        return false;
    }
    const auto& entry = entries_[index.row()];
    if (index.column() == Key) {
        return document_->updateSettingKey(entry.lineIndex, value.toString());
    }
    if (index.column() == Value) {
        return document_->updateSettingValue(entry.lineIndex, value.toString());
    }
    return false;
}

bool IniSettingsTableModel::addSetting(const QString& section, const QString& key, const QString& value) {
    return document_ ? document_->addSetting(section, key, value) : false;
}

bool IniSettingsTableModel::deleteSettingAtRow(int row) {
    if (!document_ || row < 0 || row >= entries_.size()) {
        return false;
    }
    return document_->deleteSetting(entries_[row].lineIndex);
}

bool IniSettingsTableModel::addSection(const QString& section) { return document_ ? document_->addSection(section) : false; }

bool IniSettingsTableModel::deleteSection(const QString& section) {
    return document_ ? document_->deleteSection(section) : false;
}

IniSettingEntry IniSettingsTableModel::entryAtRow(int row) const {
    if (row < 0 || row >= entries_.size()) {
        return {};
    }
    return entries_[row];
}

void IniSettingsTableModel::refresh() {
    beginResetModel();
    entries_ = document_ ? document_->keyValueEntries() : QVector<IniSettingEntry>{};
    endResetModel();
}

