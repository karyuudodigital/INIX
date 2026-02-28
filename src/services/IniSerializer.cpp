#include "services/IniSerializer.h"

#include <QFile>

QByteArray IniSerializer::serialize(const IniDocument& document) const {
    QByteArray bytes;
    if (document.encoding() == IniEncoding::Utf8Bom) {
        bytes.append("\xEF\xBB\xBF");
    }

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
    if (document.encoding() == IniEncoding::Local8Bit) {
        bytes.append(merged.toLocal8Bit());
    } else {
        bytes.append(merged.toUtf8());
    }
    return bytes;
}

bool IniSerializer::writeToFile(const IniDocument& document, const QString& filePath, QString& error) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("Unable to open file for writing: %1").arg(filePath);
        return false;
    }
    const QByteArray serialized = serialize(document);
    if (file.write(serialized) != serialized.size()) {
        error = QStringLiteral("Failed to write all bytes to: %1").arg(filePath);
        return false;
    }
    return true;
}
