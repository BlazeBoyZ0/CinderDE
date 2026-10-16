#ifndef CINDERDE_FILE_TYPE_DETECTOR_H
#define CINDERDE_FILE_TYPE_DETECTOR_H

#include <QString>

namespace CinderDE {

struct FileTypeInfo {
    QString name;
    QString extension;
    bool isBinary;
};

class FileTypeDetector {
public:
    static FileTypeInfo detectType(const QString &filePath);
    static bool isLikelyBinary(const QString &filePath, int bytesToRead = 1024);
};

} // namespace CinderDE

#endif // CINDERDE_FILE_TYPE_DETECTOR_H
