/*
    File: domain/IniDocument.cpp
    Purpose:
      - Implements mutable INI document operations and change notification.

    How it fits in the codebase:
      - Acts as the canonical state store for editor data.
      - UI models read from this object.
      - Services and MainWindow mutate it through explicit methods.

    Qt notes:
      - IniDocument inherits QObject and emits changed() whenever observable state changes.
      - Views refresh by listening to that signal through model adapters.
*/

#include "domain/IniDocument.h"

IniDocument::IniDocument(QObject* parent) : QObject(parent) {}

void IniDocument::clear() {
    // Return to a clean default state equivalent to a brand-new document.
    lines_.clear();
    path_.clear();
    encoding_ = IniEncoding::Utf8;
    lineEnding_ = "\n";
    dirty_ = false;
    emit changed();
}

bool IniDocument::isEmpty() const { return lines_.isEmpty(); }

bool IniDocument::isDirty() const { return dirty_; }

void IniDocument::setDirty(bool dirty) {
    // Dirty flag is part of observable state, so we notify listeners.
    dirty_ = dirty;
    emit changed();
}

QString IniDocument::path() const { return path_; }

void IniDocument::setPath(const QString& path) { path_ = path; }

IniEncoding IniDocument::encoding() const { return encoding_; }

void IniDocument::setEncoding(IniEncoding encoding) { encoding_ = encoding; }

QString IniDocument::lineEnding() const { return lineEnding_; }

void IniDocument::setLineEnding(const QString& lineEnding) { lineEnding_ = lineEnding; }

const QVector<IniLine>& IniDocument::lines() const { return lines_; }

void IniDocument::setLines(QVector<IniLine> lines) {
    // This method is used after parse/restore. Incoming lines become authoritative.
    lines_ = std::move(lines);
    dirty_ = false;
    emit changed();
}

QVector<IniSettingEntry> IniDocument::keyValueEntries() const {
    // Flatten parsed lines into table rows used by UI models.
    QVector<IniSettingEntry> entries;
    entries.reserve(lines_.size());
    for (int i = 0; i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        if (line.type != IniLineType::KeyValue) {
            continue;
        }
        entries.push_back(IniSettingEntry{
            .lineIndex = i,
            .sourceLine = line.sourceLine,
            .section = line.section,
            .key = line.key,
            .value = line.value,
        });
    }
    return entries;
}

bool IniDocument::updateSettingValue(int lineIndex, const QString& value) {
    // Guard invalid indices and non-key/value lines.
    if (lineIndex < 0 || lineIndex >= lines_.size()) {
        return false;
    }
    auto& line = lines_[lineIndex];
    if (line.type != IniLineType::KeyValue) {
        return false;
    }
    // Keep rawText synchronized so serializer can preserve edited values.
    line.value = value;
    line.rawText = QStringLiteral("%1=%2").arg(line.key, line.value);
    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::updateSettingKey(int lineIndex, const QString& key) {
    if (lineIndex < 0 || lineIndex >= lines_.size() || key.trimmed().isEmpty()) {
        return false;
    }
    auto& line = lines_[lineIndex];
    if (line.type != IniLineType::KeyValue) {
        return false;
    }
    // Key edits preserve value and regenerate canonical "key=value" text.
    line.key = key.trimmed();
    line.rawText = QStringLiteral("%1=%2").arg(line.key, line.value);
    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::addSection(const QString& section) {
    const QString normalized = section.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    // Insert a blank spacer line followed by the section header line.
    // sourceLine is recomputed based on current insertion position.
    lines_.push_back(IniLine{
        .type = IniLineType::Blank,
        .rawText = QString(),
        .section = QString(),
        .key = QString(),
        .value = QString(),
        .sourceLine = static_cast<int>(lines_.size()) + 1,
    });
    lines_.push_back(IniLine{
        .type = IniLineType::Section,
        .rawText = QStringLiteral("[%1]").arg(normalized),
        .section = normalized,
        .key = QString(),
        .value = QString(),
        .sourceLine = static_cast<int>(lines_.size()) + 1,
    });

    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::addSetting(const QString& section, const QString& key, const QString& value) {
    const QString normalizedSection = section.trimmed();
    const QString normalizedKey = key.trimmed();
    if (normalizedSection.isEmpty() || normalizedKey.isEmpty()) {
        return false;
    }

    // Find insertion point:
    // 1) Inside existing section (after last line in that section), or
    // 2) End of file after creating section if missing.
    int insertIndex = lines_.size();
    bool seenSection = false;
    for (int i = 0; i < lines_.size(); ++i) {
        if (lines_[i].type == IniLineType::Section) {
            if (seenSection) {
                insertIndex = i;
                break;
            }
            seenSection = lines_[i].section.compare(normalizedSection, Qt::CaseInsensitive) == 0;
            continue;
        }
        if (seenSection) {
            insertIndex = i + 1;
        }
    }

    if (!seenSection) {
        // Creates section + blank separator and marks document dirty.
        addSection(normalizedSection);
        insertIndex = lines_.size();
    }

    // Insert new key/value line and normalize sourceLine metadata.
    lines_.insert(insertIndex, IniLine{
                                  .type = IniLineType::KeyValue,
                                  .rawText = QStringLiteral("%1=%2").arg(normalizedKey, value),
                                  .section = normalizedSection,
                                  .key = normalizedKey,
                                  .value = value,
                                  .sourceLine = insertIndex + 1,
                              });

    for (int i = 0; i < lines_.size(); ++i) {
        lines_[i].sourceLine = i + 1;
    }

    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::deleteSetting(int lineIndex) {
    if (lineIndex < 0 || lineIndex >= lines_.size() || lines_[lineIndex].type != IniLineType::KeyValue) {
        return false;
    }
    // Remove the row and renumber sourceLine so UI reflects contiguous line numbers.
    lines_.removeAt(lineIndex);
    for (int i = 0; i < lines_.size(); ++i) {
        lines_[i].sourceLine = i + 1;
    }
    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::deleteSection(const QString& section) {
    const QString normalized = section.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    // Identify the section range [startIndex, endIndex) to remove.
    int startIndex = -1;
    int endIndex = lines_.size();
    for (int i = 0; i < lines_.size(); ++i) {
        if (lines_[i].type != IniLineType::Section) {
            continue;
        }
        if (startIndex < 0 && lines_[i].section.compare(normalized, Qt::CaseInsensitive) == 0) {
            startIndex = i;
            continue;
        }
        if (startIndex >= 0) {
            endIndex = i;
            break;
        }
    }
    if (startIndex < 0) {
        return false;
    }

    // Remove section header and all lines until next section header or EOF.
    lines_.remove(startIndex, endIndex - startIndex);
    for (int i = 0; i < lines_.size(); ++i) {
        lines_[i].sourceLine = i + 1;
    }
    dirty_ = true;
    emit changed();
    return true;
}

bool IniDocument::findSetting(const QString& section, const QString& key, int& lineIndex) const {
    lineIndex = -1;
    // Search from bottom to top so "last occurrence wins" for duplicate keys.
    for (int i = lines_.size() - 1; i >= 0; --i) {
        const auto& line = lines_[i];
        if (line.type != IniLineType::KeyValue) {
            continue;
        }
        if (line.section.compare(section, Qt::CaseInsensitive) == 0 &&
            line.key.compare(key, Qt::CaseInsensitive) == 0) {
            lineIndex = i;
            return true;
        }
    }
    return false;
}

QString IniDocument::getSettingValue(const QString& section, const QString& key) const {
    int lineIndex = -1;
    if (!findSetting(section, key, lineIndex)) {
        return {};
    }
    return lines_[lineIndex].value;
}

bool IniDocument::upsertSetting(const QString& section, const QString& key, const QString& value) {
    // Update existing key when present, otherwise append a new setting.
    int existingLine = -1;
    if (findSetting(section, key, existingLine)) {
        return updateSettingValue(existingLine, value);
    }
    return addSetting(section, key, value);
}

IniDocumentSnapshot IniDocument::snapshot() const {
    // Cheap value-type snapshot used by parser handoff and undo/redo command.
    return IniDocumentSnapshot{
        .lines = lines_,
        .encoding = encoding_,
        .lineEnding = lineEnding_,
        .path = path_,
    };
}

void IniDocument::restore(const IniDocumentSnapshot& snapshot) {
    // Restore all persisted fields in one operation.
    lines_ = snapshot.lines;
    encoding_ = snapshot.encoding;
    lineEnding_ = snapshot.lineEnding;
    path_ = snapshot.path;
    dirty_ = true;
    emit changed();
}
