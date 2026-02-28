/*
    File: ui/models/IniSettingsTableModel.h
    Purpose:
      - Primary editable table model over IniDocument key/value entries.

    How it fits in the codebase:
      - Backing model for the central settings table.
      - Reads from IniDocument and writes edits back to IniDocument methods.
*/

#pragma once

#include "domain/IniDocument.h"

#include <QAbstractTableModel>

class IniSettingsTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    // Stable column indices used by views/proxy models.
    enum Column {
        Section = 0,
        Key = 1,
        Value = 2,
        SourceLine = 3,
        ColumnCount = 4
    };

    explicit IniSettingsTableModel(QObject* parent = nullptr);

    // Attach/detach a document. Model subscribes to document changed() and refreshes.
    void setDocument(IniDocument* document);
    IniDocument* document() const;

    // QAbstractTableModel contract
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    // Convenience methods used by MainWindow commands.
    bool addSetting(const QString& section, const QString& key, const QString& value);
    bool deleteSettingAtRow(int row);
    bool addSection(const QString& section);
    bool deleteSection(const QString& section);
    IniSettingEntry entryAtRow(int row) const;

private:
    void refresh();

    IniDocument* document_ = nullptr;
    QVector<IniSettingEntry> entries_;
};
