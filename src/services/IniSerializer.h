/*
    File: services/IniSerializer.h
    Purpose:
      - Declares document serialization and save-to-disk helpers.

    How it fits in the codebase:
      - Called by MainWindow save/save-as actions.
      - Uses IniDocument encoding + line ending metadata to preserve file style.
*/

#pragma once

#include "domain/IniDocument.h"

#include <QByteArray>

class IniSerializer {
public:
    // Convert the document to bytes ready for file output.
    QByteArray serialize(const IniDocument& document) const;
    // Write serialized bytes to disk, reporting user-facing error text.
    bool writeToFile(const IniDocument& document, const QString& filePath, QString& error) const;
};
