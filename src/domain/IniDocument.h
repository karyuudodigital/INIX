#pragma once

#include "domain/IniTypes.h"

#include <QObject>
#include <QString>

struct IniDocumentSnapshot {
    QVector<IniLine> lines;
    IniEncoding encoding = IniEncoding::Utf8;
    QString lineEnding = "\n";
    QString path;
};

class IniDocument : public QObject {
    Q_OBJECT

public:
    explicit IniDocument(QObject* parent = nullptr);

    void clear();
    bool isEmpty() const;
    bool isDirty() const;
    void setDirty(bool dirty);

    QString path() const;
    void setPath(const QString& path);

    IniEncoding encoding() const;
    void setEncoding(IniEncoding encoding);

    QString lineEnding() const;
    void setLineEnding(const QString& lineEnding);

    const QVector<IniLine>& lines() const;
    void setLines(QVector<IniLine> lines);

    QVector<IniSettingEntry> keyValueEntries() const;

    bool updateSettingValue(int lineIndex, const QString& value);
    bool updateSettingKey(int lineIndex, const QString& key);
    bool addSetting(const QString& section, const QString& key, const QString& value);
    bool deleteSetting(int lineIndex);
    bool addSection(const QString& section);
    bool deleteSection(const QString& section);
    bool upsertSetting(const QString& section, const QString& key, const QString& value);
    bool findSetting(const QString& section, const QString& key, int& lineIndex) const;
    QString getSettingValue(const QString& section, const QString& key) const;

    IniDocumentSnapshot snapshot() const;
    void restore(const IniDocumentSnapshot& snapshot);

signals:
    void changed();

private:
    QVector<IniLine> lines_;
    IniEncoding encoding_ = IniEncoding::Utf8;
    QString lineEnding_ = "\n";
    QString path_;
    bool dirty_ = false;
};

