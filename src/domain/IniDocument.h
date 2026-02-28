/*
    File: domain/IniDocument.h
    Purpose:
      - Declares the in-memory editable INI document abstraction.

    How it fits in the codebase:
      - Central mutable state object used by MainWindow.
      - Services consume snapshots or read-only views from this object.
      - UI models subscribe to its changed() signal and refresh tables.
*/

#pragma once

#include "domain/IniTypes.h"

#include <QObject>
#include <QString>

// Serializable document state used for parse results and undo snapshots.
struct IniDocumentSnapshot {
    QVector<IniLine> lines;
    IniEncoding encoding = IniEncoding::Utf8;
    QString lineEnding = "\n";
    QString path;
};

// IniDocument is a QObject so it can emit changed() and be observed by Qt models/views.
class IniDocument : public QObject {
    Q_OBJECT

public:
    explicit IniDocument(QObject* parent = nullptr);

    // Reset to an empty, clean UTF-8 document.
    void clear();
    bool isEmpty() const;
    bool isDirty() const;
    void setDirty(bool dirty);

    // Source path of the document currently loaded in memory.
    QString path() const;
    void setPath(const QString& path);

    // File encoding and line-ending policy used by IniSerializer.
    IniEncoding encoding() const;
    void setEncoding(IniEncoding encoding);

    QString lineEnding() const;
    void setLineEnding(const QString& lineEnding);

    // Full parsed line list for low-level operations.
    const QVector<IniLine>& lines() const;
    void setLines(QVector<IniLine> lines);

    // Flattened editable rows for table-oriented UI.
    QVector<IniSettingEntry> keyValueEntries() const;

    // Point updates and structural edits.
    bool updateSettingValue(int lineIndex, const QString& value);
    bool updateSettingKey(int lineIndex, const QString& key);
    bool addSetting(const QString& section, const QString& key, const QString& value);
    bool deleteSetting(int lineIndex);
    bool addSection(const QString& section);
    bool deleteSection(const QString& section);
    bool upsertSetting(const QString& section, const QString& key, const QString& value);
    bool findSetting(const QString& section, const QString& key, int& lineIndex) const;
    QString getSettingValue(const QString& section, const QString& key) const;

    // Snapshot/restore is used by parser handoff and undo command integration.
    IniDocumentSnapshot snapshot() const;
    void restore(const IniDocumentSnapshot& snapshot);

signals:
    // Emitted whenever observable document state changes.
    void changed();

private:
    QVector<IniLine> lines_;
    IniEncoding encoding_ = IniEncoding::Utf8;
    QString lineEnding_ = "\n";
    QString path_;
    bool dirty_ = false;
};
