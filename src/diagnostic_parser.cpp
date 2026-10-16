#include "cinderde/diagnostic_parser.h"
#include <QRegularExpression>
#include <QDir>
#include <QFileInfo>

namespace CinderDE {

QString DiagnosticParser::resolvePath(const QString &pathStr, const QString &workingDir) {
    if (pathStr.isEmpty()) return QString();

    QFileInfo info(pathStr);
    if (info.isAbsolute()) {
        return QDir::cleanPath(info.filePath());
    }

    if (!workingDir.isEmpty()) {
        QDir dir(workingDir);
        QString combined = dir.filePath(pathStr);
        return QDir::cleanPath(combined);
    }

    return QDir::cleanPath(pathStr);
}

std::optional<DiagnosticItem> DiagnosticParser::parseLine(const QString &line, const QString &workingDir) {
    if (line.trimmed().isEmpty()) return std::nullopt;

    // Matches GCC/Clang format: file:line:[col:] [fatal error|error|warning|note|info]: message
    static const QRegularExpression regex(
        R"(^([^:\n]+):(\d+):(?:(\d+):)?\s*(fatal error|error|warning|note|info):\s*(.*)$)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = regex.match(line.trimmed());
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    DiagnosticItem item;
    item.rawText = line;
    QString rawFile = match.captured(1);
    item.line = match.captured(2).toInt();

    if (!match.captured(3).isEmpty()) {
        item.column = match.captured(3).toInt();
    } else {
        item.column = -1;
    }

    QString levelStr = match.captured(4).toLower();
    if (levelStr == "fatal error" || levelStr == "error") {
        item.level = DiagnosticItem::Error;
    } else if (levelStr == "warning") {
        item.level = DiagnosticItem::Warning;
    } else if (levelStr == "note") {
        item.level = DiagnosticItem::Note;
    } else {
        item.level = DiagnosticItem::Info;
    }

    item.message = match.captured(5);
    item.filePath = resolvePath(rawFile, workingDir);

    return item;
}

QList<DiagnosticItem> DiagnosticParser::parseOutput(const QString &output, const QString &workingDir) {
    QList<DiagnosticItem> items;
    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        auto item = parseLine(line, workingDir);
        if (item.has_value() && item->isValid()) {
            items.append(*item);
        }
    }
    return items;
}

} // namespace CinderDE
