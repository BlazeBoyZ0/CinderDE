#ifndef CINDERDE_DIAGNOSTIC_PARSER_H
#define CINDERDE_DIAGNOSTIC_PARSER_H

#include <QString>
#include <QList>
#include <optional>

namespace CinderDE {

struct DiagnosticItem {
    enum Level {
        Error,
        Warning,
        Note,
        Info
    };

    QString filePath;
    int line = -1;     // 1-based, -1 if not specified
    int column = -1;   // 1-based, -1 if not specified
    Level level = Error;
    QString message;
    QString rawText;

    bool isValid() const { return !filePath.isEmpty() && line > 0; }
    QString levelString() const {
        switch (level) {
            case Error: return "error";
            case Warning: return "warning";
            case Note: return "note";
            case Info: return "info";
        }
        return "error";
    }
};

class DiagnosticParser {
public:
    static std::optional<DiagnosticItem> parseLine(const QString &line, const QString &workingDir = QString());
    static QList<DiagnosticItem> parseOutput(const QString &output, const QString &workingDir = QString());
    static QString resolvePath(const QString &pathStr, const QString &workingDir = QString());
};

} // namespace CinderDE

#endif // CINDERDE_DIAGNOSTIC_PARSER_H
