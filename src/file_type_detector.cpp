#include "cinderde/file_type_detector.h"
#include <QFileInfo>
#include <QFile>
#include <QMap>

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

    static const QMap<QString, QString> extensionMap = {
        {"cpp", "C++ Source"},
        {"cc", "C++ Source"},
        {"cxx", "C++ Source"},
        {"c", "C Source"},
        {"h", "Header File"},
        {"hpp", "Header File"},
        {"hxx", "Header File"},
        {"py", "Python Source"},
        {"txt", "Plain Text"},
        {"md", "Markdown Document"},
        {"markdown", "Markdown Document"},
        {"json", "JSON File"},
        {"xml", "XML File"},
        {"html", "HTML Document"},
        {"htm", "HTML Document"},
        {"css", "CSS Stylesheet"},
        {"js", "JavaScript File"},
        {"ts", "TypeScript File"},
        {"java", "Java Source"},
        {"kt", "Kotlin Source"},
        {"kts", "Kotlin Script"},
        {"cmake", "CMake Script"},
        {"sh", "Shell Script"},
        {"bash", "Bash Script"},
        {"yaml", "YAML File"},
        {"yml", "YAML File"},
        {"ini", "INI Configuration"},
        {"conf", "Configuration File"},
        {"qrc", "Qt Resource File"},
        {"ui", "Qt Designer UI File"}
    };

    if (fileInfo.fileName().toLower() == "cmakelists.txt") {
        info.name = "CMake Build Script";
        return info;
    }

    if (extensionMap.contains(ext)) {
        info.name = extensionMap.value(ext);
    } else if (!ext.isEmpty()) {
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
