#pragma once

#include "domain/IniDocument.h"

#include <QByteArray>

class IniSerializer {
public:
    QByteArray serialize(const IniDocument& document) const;
    bool writeToFile(const IniDocument& document, const QString& filePath, QString& error) const;
};

