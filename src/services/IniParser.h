#pragma once

#include "domain/IniDocument.h"

#include <QString>

struct IniParseResult {
    bool ok = false;
    QString error;
    IniDocumentSnapshot snapshot;
};

class IniParser {
public:
    IniParseResult parseFile(const QString& filePath) const;
    IniParseResult parseText(const QByteArray& bytes, const QString& sourcePath = QString()) const;

private:
    QString decodeBytes(const QByteArray& bytes, IniEncoding& encodingOut) const;
};

