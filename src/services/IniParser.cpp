#include "services/IniParser.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>

namespace {
QString detectLineEnding(const QString& text) {
    if (text.contains("\r\n")) {
        return "\r\n";
    }
    if (text.contains("\r")) {
        return "\r";
    }
    return "\n";
}
} // namespace

IniParseResult IniParser::parseFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {.ok = false, .error = QStringLiteral("Unable to open file: %1").arg(filePath)};
    }
    return parseText(file.readAll(), filePath);
}

IniParseResult IniParser::parseText(const QByteArray& bytes, const QString& sourcePath) const {
    IniEncoding encoding = IniEncoding::Utf8;
    const QString text = decodeBytes(bytes, encoding);
    const QString lineEnding = detectLineEnding(text);
    const QStringList lines = text.split(QRegularExpression("\r\n|\n|\r"), Qt::KeepEmptyParts);

    QVector<IniLine> parsedLines;
    parsedLines.reserve(lines.size());

    QString currentSection;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        const QString trimmed = line.trimmed();
        IniLine parsedLine;
        parsedLine.rawText = line;
        parsedLine.sourceLine = i + 1;

        if (trimmed.isEmpty()) {
            parsedLine.type = IniLineType::Blank;
        } else if (trimmed.startsWith(';') || trimmed.startsWith('#')) {
            parsedLine.type = IniLineType::Comment;
        } else if (trimmed.startsWith('[') && trimmed.endsWith(']') && trimmed.size() > 2) {
            parsedLine.type = IniLineType::Section;
            currentSection = trimmed.mid(1, trimmed.size() - 2).trimmed();
            parsedLine.section = currentSection;
        } else {
            const int equalsPos = line.indexOf('=');
            if (equalsPos > 0) {
                parsedLine.type = IniLineType::KeyValue;
                parsedLine.section = currentSection;
                parsedLine.key = line.left(equalsPos).trimmed();
                parsedLine.value = line.mid(equalsPos + 1);
            } else {
                parsedLine.type = IniLineType::Malformed;
            }
        }

        parsedLines.push_back(std::move(parsedLine));
    }

    IniDocumentSnapshot snapshot;
    snapshot.lines = std::move(parsedLines);
    snapshot.encoding = encoding;
    snapshot.lineEnding = lineEnding;
    snapshot.path = sourcePath;
    return {.ok = true, .snapshot = std::move(snapshot)};
}

QString IniParser::decodeBytes(const QByteArray& bytes, IniEncoding& encodingOut) const {
    if (bytes.startsWith("\xEF\xBB\xBF")) {
        encodingOut = IniEncoding::Utf8Bom;
        return QString::fromUtf8(bytes.mid(3));
    }

    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    const QString utf8Decoded = utf8Decoder.decode(bytes);
    if (!utf8Decoder.hasError()) {
        encodingOut = IniEncoding::Utf8;
        return utf8Decoded;
    }

    encodingOut = IniEncoding::Local8Bit;
    return QString::fromLocal8Bit(bytes);
}
