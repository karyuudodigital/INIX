/*
    File: services/IniParser.h
    Purpose:
      - Declares parsing from bytes/files into IniDocumentSnapshot.

    How it fits in the codebase:
      - Used by MainWindow async open flows (target and compare files).
      - Converts text into line-typed structures consumed by domain/services/models.
*/

#pragma once

#include "domain/IniDocument.h"

#include <QString>

// Result object keeps error text + parsed snapshot in one return value.
struct IniParseResult {
    bool ok = false;
    QString error;
    IniDocumentSnapshot snapshot;
};

class IniParser {
public:
    // Parse from a filesystem path.
    IniParseResult parseFile(const QString& filePath) const;
    // Parse from raw bytes, useful for tests and non-file sources.
    IniParseResult parseText(const QByteArray& bytes, const QString& sourcePath = QString()) const;

private:
    // Decode bytes to QString while reporting selected encoding mode.
    QString decodeBytes(const QByteArray& bytes, IniEncoding& encodingOut) const;
};
