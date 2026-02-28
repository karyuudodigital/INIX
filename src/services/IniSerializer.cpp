/*
    File: services/IniSerializer.cpp
    Purpose:
      - Implements INI document serialization and file output.

    How it fits in the codebase:
      - MainWindow save actions call writeToFile().
      - Uses IniDocument metadata (encoding, line ending) for stable output.
*/

#include "services/IniSerializer.h"

#include <QFile>

QByteArray IniSerializer::serialize(const IniDocument& document) const {
    QByteArray bytes;

    // Emit UTF-8 BOM only when explicitly requested by document metadata.
    if (document.encoding() == IniEncoding::Utf8Bom) {
        bytes.append("\xEF\xBB\xBF");
    }

    // Rebuild textual lines from structured data.
    QStringList outputLines;
    outputLines.reserve(document.lines().size());
    for (const auto& line : document.lines()) {
        if (line.type == IniLineType::Section) {
            outputLines.push_back(QStringLiteral("[%1]").arg(line.section));
            continue;
        }
        if (line.type == IniLineType::KeyValue) {
            outputLines.push_back(QStringLiteral("%1=%2").arg(line.key, line.value));
            continue;
        }
        outputLines.push_back(line.rawText);
    }

    const QString merged = outputLines.join(document.lineEnding());

    // Encode with either local code page or UTF-8 mode.
    if (document.encoding() == IniEncoding::Local8Bit) {
        bytes.append(merged.toLocal8Bit());
    } else {
        bytes.append(merged.toUtf8());
    }
    return bytes;
}

bool IniSerializer::writeToFile(const IniDocument& document, const QString& filePath, QString& error) const {
    // Truncate mode ensures full overwrite of previous file contents.
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("Unable to open file for writing: %1").arg(filePath);
        return false;
    }
    const QByteArray serialized = serialize(document);
    // Ensure all bytes are written; partial writes are treated as failure.
    if (file.write(serialized) != serialized.size()) {
        error = QStringLiteral("Failed to write all bytes to: %1").arg(filePath);
        return false;
    }
    return true;
}
