#include "cinderde/file_type_detector.h"
#include "cinderde/language_pack.h"
#include <QFileInfo>
#include <QFile>

namespace CinderDE {

FileTypeInfo FileTypeDetector::detectType(const QString &filePath) {
    FileTypeInfo info;
    info.isBinary = isLikelyBinary(filePath);

    if (filePath.isEmpty()) {
        info.name = "Plain Text";
        info.extension = "";
        return info;
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    info.extension = ext;

    if (info.isBinary) {
        info.name = "Binary File";
        return info;
    }

    LanguageDefinition langDef = LanguageRegistry::instance().definitionForExtension(ext);
    if (langDef.isValid()) {
        info.name = langDef.name;
    } else if (!ext.isEmpty()) {
        // Without an installed pack, identify only the file extension, not a language.
        info.name = ext.toUpper() + " File";
    } else {
        info.name = "Plain Text";
    }

    return info;
}

bool FileTypeDetector::isLikelyBinary(const QString &filePath, int bytesToRead) {
    if (filePath.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.read(bytesToRead);
    file.close();

    if (data.isEmpty()) {
        return false;
    }

    int nullCount = 0;
    int controlCharCount = 0;

    for (int i = 0; i < data.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(data.at(i));
        if (ch == 0) {
            nullCount++;
        } else if ((ch < 7 || (ch > 13 && ch < 32)) && ch != 27) { // standard control chars excluding \t, \n, \r, ESC
            controlCharCount++;
        }
    }

    // If there's any null byte or > 10% non-printable control characters, treat as binary
    if (nullCount > 0) {
        return true;
    }

    if (data.size() > 0 && (double)controlCharCount / data.size() > 0.10) {
        return true;
    }

    return false;
}

} // namespace CinderDE
